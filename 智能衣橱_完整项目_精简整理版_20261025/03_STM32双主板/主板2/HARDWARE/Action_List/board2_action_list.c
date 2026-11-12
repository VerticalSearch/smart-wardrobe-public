#include "board2_action_list.h"
#include "board2_action_tuning.h"
#include "control.h"
#include "stepper_multi.h"
#include "timer.h"
#include "usartdma.h"
#include "delay.h"

typedef struct
{
    float x_deg;
    float y_deg;
    float z_deg;
} Board2ActionPosition_t;

typedef struct
{
    const Board2ActionGroupStep_t *steps;
    uint8_t step_count;
} Board2ActionGroup_t;

typedef enum
{
    BOARD2_AXIS_X = 0,
    BOARD2_AXIS_Y,
    BOARD2_AXIS_Z,
    BOARD2_AXIS_TELESCOPE
} Board2Axis_t;

typedef enum
{
    BOARD2_ACTION_MODE_DEFAULT = 0,
    BOARD2_ACTION_MODE_STORE_PREPARE,
    BOARD2_ACTION_MODE_STORE_RETRACT,
    BOARD2_ACTION_MODE_TAKE_C_TO_A,
    BOARD2_ACTION_MODE_FETCH_RESET
} Board2ActionMode_t;

#define BOARD2_ACTION_POINT_ENUM(id, name, x_deg, y_deg, z_deg) BOARD2_ACTION_POINT_##id,

typedef enum
{
    BOARD2_ACTION_POINT_TABLE(BOARD2_ACTION_POINT_ENUM)
    BOARD2_ACTION_POINT_COUNT
} Board2ActionPointId_t;

#undef BOARD2_ACTION_POINT_ENUM

#define BOARD2_ACTION_POINT_HOME            BOARD2_ACTION_POINT_A0  /* 回家点就是A0点 */

#define BOARD2_ARRAY_COUNT(list)             ((uint8_t)(sizeof(list) / sizeof((list)[0])))
#define BOARD2_FLOAT_ABS(x)                  (((x) < 0.0f) ? -(x) : (x))

/* 调参只改 board2_action_tuning.h；本文件保留流程状态机。 */
volatile uint8_t g_board2_action_running = 0U;
volatile uint8_t g_board2_action_current_step = 0U;
volatile uint8_t g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
volatile uint8_t g_board2_action_current_group_step = 0U;
volatile uint8_t g_board2_action_last_point = BOARD2_ACTION_POINT_HOME;
volatile uint8_t g_board2_action_next_c_index = 0U;
volatile uint8_t g_board2_action_target_c = 0U;
volatile uint8_t g_board2_action_result = BOARD2_ACTION_RESULT_IDLE;

static uint8_t s_workflow_state = BOARD2_ACTION_WORKFLOW_HOME_TO_A0;
static uint8_t s_workflow_mode = BOARD2_ACTION_MODE_DEFAULT;
static uint8_t s_current_a_index = 0U;
static uint8_t s_current_c_index = 0U;
static uint8_t s_single_store_item = 0U;
static int s_next_b_servo1_deg = BOARD2_ACTION_SERVO1_BACK_DEG;
static uint16_t s_b_flip_count = 0U;

static uint32_t s_wait_start_ms = 0U;
static uint32_t s_wait_duration_ms = 0U;
static uint8_t s_waiting = 0U;

static uint32_t s_motion_start_ms = 0U;
static uint32_t s_motion_duration_ms = 0U;
static uint8_t s_motion_waiting = 0U;
static uint8_t s_motion_axis_mask = 0U;
static float s_motion_pending_delta[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static uint8_t s_motion_feedback_fault = 0U;

static float get_axis_position(Board2Axis_t axis);
static void set_axis_position(Board2Axis_t axis, float angle_deg);
static void commit_motion_feedback_axes(uint8_t mask);
static void discard_motion_feedback_axes(uint8_t mask);
static uint8_t s_group_active = 0U;
static uint8_t s_group_step_active = 0U;

static uint32_t s_servo_last_update_ms = 0U;
static uint8_t s_servo1_active = 0U;
static uint8_t s_servo2_active = 0U;
static int s_servo1_target_deg = TIM1_SERVO1_INIT_ANGLE_DEG;
static int s_servo2_target_deg = TIM1_SERVO2_INIT_ANGLE_DEG;

static Board2ActionPosition_t s_position =
{
    0.0f,
    0.0f,
    0.0f
};

#define BOARD2_ACTION_POINT_ENTRY(id, name, x_deg, y_deg, z_deg) { name, x_deg, y_deg, z_deg },
static const Board2ActionPoint_t s_points[] =
{
    BOARD2_ACTION_POINT_TABLE(BOARD2_ACTION_POINT_ENTRY)
};
#undef BOARD2_ACTION_POINT_ENTRY

static const uint8_t s_default_a_points[] =
{
    BOARD2_ACTION_POINT_A01,
    BOARD2_ACTION_POINT_A02,
    BOARD2_ACTION_POINT_A03,
    BOARD2_ACTION_POINT_A04,
};

static const uint8_t s_default_a_switches[] =
{
    4U,
    3U,
    2U,
    1U,
};

static const Board2ActionGroupStep_t s_grab_group[] =
{
    /* name,    Z absolute, servo1,                    servo2,                    wait */
    { "open",     BOARD2_ACTION_GRAB_Z_UP_DEG,    BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO2_OPEN_DEG,  BOARD2_ACTION_WAIT_GRAB_OPEN_MS },
    { "down",     BOARD2_ACTION_GRAB_Z_DOWN_DEG,  BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,      BOARD2_ACTION_WAIT_GRAB_DOWN_MS },
    { "close",    BOARD2_ACTION_GRAB_Z_DOWN_DEG,  BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO2_CLOSE_DEG, BOARD2_ACTION_WAIT_GRAB_CLOSE_MS },
    { "up",       BOARD2_ACTION_GRAB_Z_UP_DEG,    BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,      BOARD2_ACTION_WAIT_GRAB_UP_MS },
};

static const Board2ActionGroupStep_t s_place_group[] =
{
    /* name,    Z absolute, servo1,                    servo2,                    wait */
    { "down",     BOARD2_ACTION_PLACE_Z_DOWN_DEG,  BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,     BOARD2_ACTION_WAIT_PLACE_DOWN_MS },
    { "release",  BOARD2_ACTION_PLACE_Z_DOWN_DEG,  BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO2_OPEN_DEG, BOARD2_ACTION_WAIT_PLACE_RELEASE_MS },
    { "up",       BOARD2_ACTION_PLACE_Z_UP_DEG,    BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,     BOARD2_ACTION_WAIT_PLACE_UP_MS },
};

static const Board2ActionGroupStep_t s_c_grab_group[] =
{
    /* C区抓取衣服：C区放置高度是0，所以取衣时也降到0夹紧再抬起。 */
    { "c_open",   BOARD2_ACTION_C_GRAB_Z_UP_DEG,   BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO2_OPEN_DEG,  BOARD2_ACTION_WAIT_C_GRAB_OPEN_MS },
    { "c_down",   BOARD2_ACTION_C_GRAB_Z_DOWN_DEG, BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,      BOARD2_ACTION_WAIT_C_GRAB_DOWN_MS },
    { "c_close",  BOARD2_ACTION_C_GRAB_Z_DOWN_DEG, BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO2_CLOSE_DEG, BOARD2_ACTION_WAIT_C_GRAB_CLOSE_MS },
    { "c_up",     BOARD2_ACTION_C_GRAB_Z_UP_DEG,   BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,      BOARD2_ACTION_WAIT_C_GRAB_UP_MS },
};

static const Board2ActionGroupStep_t s_a_place_group[] =
{
    /* A区放置衣服：A区原抓取高度是-1200，所以放回A区时也降到-1200松开。 */
    { "a_down",    BOARD2_ACTION_A_PLACE_Z_DOWN_DEG, BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,     BOARD2_ACTION_WAIT_A_PLACE_DOWN_MS },
    { "a_release", BOARD2_ACTION_A_RELEASE_Z_DEG,    BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO2_OPEN_DEG, BOARD2_ACTION_WAIT_A_PLACE_RELEASE_MS },
    { "a_up",      BOARD2_ACTION_A_PLACE_Z_UP_DEG,   BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,     BOARD2_ACTION_WAIT_A_PLACE_UP_MS },
};

static const Board2ActionGroup_t s_groups[BOARD2_ACTION_GROUP_COUNT] =
{
    { s_grab_group,  BOARD2_ARRAY_COUNT(s_grab_group)  },
    { s_place_group, BOARD2_ARRAY_COUNT(s_place_group) },
    { s_c_grab_group,  BOARD2_ARRAY_COUNT(s_c_grab_group)  },
    { s_a_place_group, BOARD2_ARRAY_COUNT(s_a_place_group) },
};

static uint16_t limit_speed(uint16_t speed_rpm)
{
    return (speed_rpm > BOARD2_ACTION_MOTOR_MAX_SPEED_RPM) ? BOARD2_ACTION_MOTOR_MAX_SPEED_RPM : speed_rpm;
}

static uint16_t limit_axis_speed(Board2Axis_t axis, uint16_t speed_rpm)
{
    uint16_t speed = limit_speed(speed_rpm);

    if ((axis == BOARD2_AXIS_X) && (speed > BOARD2_ACTION_X_MAX_SPEED_RPM))
    {
        speed = BOARD2_ACTION_X_MAX_SPEED_RPM;
    }

    return speed;
}

static uint8_t limit_accel(uint16_t accel)
{
    if (accel > BOARD2_ACTION_MOTOR_MAX_ACCEL)
    {
        accel = BOARD2_ACTION_MOTOR_MAX_ACCEL;
    }

    return BOARD2_ACCEL_U8(accel);
}

static uint32_t motion_run_time_ms(float angle_deg, uint16_t speed_rpm)
{
    float ms;
    uint32_t result_ms;

    if ((angle_deg <= 0.0f) || (speed_rpm == 0U))
    {
        return 0U;
    }

    ms = (angle_deg * 60000.0f) / ((float)speed_rpm * 360.0f);
    result_ms = (uint32_t)(ms + 0.5f);
    return (result_ms == 0U) ? 1U : result_ms;
}

static uint16_t speed_for_run_time(float angle_deg, uint32_t run_time_ms, uint16_t max_speed_rpm)
{
    float speed;

    if (angle_deg <= 0.0f)
    {
        return 0U;
    }

    if (run_time_ms == 0U)
    {
        return limit_speed(max_speed_rpm);
    }

    /* 按目标运行时间反推转速：速度 = 角度 / 时间。 */
    speed = (angle_deg * 60000.0f) / ((float)run_time_ms * 360.0f);
    if (speed < (float)BOARD2_ACTION_XY_SYNC_MIN_SPEED_RPM)
    {
        speed = (float)BOARD2_ACTION_XY_SYNC_MIN_SPEED_RPM;
    }
    else if (speed > (float)max_speed_rpm)
    {
        speed = (float)max_speed_rpm;
    }

    return limit_speed((uint16_t)(speed + 0.5f));
}

static uint8_t scale_accel_for_speed(uint16_t base_accel, uint16_t base_speed, uint16_t actual_speed)
{
    float accel;

    if ((base_speed == 0U) || (actual_speed >= base_speed))
    {
        return limit_accel(base_accel);
    }

    /* 转速降下来时，加速度也按同一比例降，减少短轴先冲到位的问题。 */
    accel = ((float)base_accel * (float)actual_speed) / (float)base_speed;
    if (accel < (float)BOARD2_ACTION_XY_SYNC_MIN_ACCEL)
    {
        accel = (float)BOARD2_ACTION_XY_SYNC_MIN_ACCEL;
    }
    else if (accel > (float)base_accel)
    {
        accel = (float)base_accel;
    }

    return limit_accel((uint16_t)(accel + 0.5f));
}

static uint32_t motion_time_ms(float angle_deg, uint16_t speed_rpm)
{
    return motion_run_time_ms(angle_deg, speed_rpm) + BOARD2_ACTION_MOTION_TIME_MARGIN_MS;
}

static uint8_t axis_bit(Board2Axis_t axis)
{
    return (uint8_t)(1U << (uint8_t)axis);
}

static void block_for_motion(uint32_t duration_ms)
{
    uint32_t elapsed_ms;
    uint32_t remaining_ms;

    if (duration_ms == 0U)
    {
        return;
    }

    if (s_motion_waiting == 0U)
    {
        s_motion_start_ms = g_board2_tick_ms;
        s_motion_duration_ms = duration_ms;
        s_motion_waiting = 1U;
        return;
    }

    elapsed_ms = (uint32_t)(g_board2_tick_ms - s_motion_start_ms);
    remaining_ms = (elapsed_ms >= s_motion_duration_ms) ? 0U : (s_motion_duration_ms - elapsed_ms);
    if (duration_ms > remaining_ms)
    {
        s_motion_start_ms = g_board2_tick_ms;
        s_motion_duration_ms = duration_ms;
    }
}

static void start_motion_wait(Board2Axis_t axis, uint32_t duration_ms)
{
    block_for_motion(duration_ms);
    s_motion_axis_mask |= axis_bit(axis);
    s_motion_feedback_fault = 0U;
}

static uint8_t motion_feedback_faulted(void)
{
    return s_motion_feedback_fault;
}

static void clear_motion_feedback_fault(void)
{
    s_motion_feedback_fault = 0U;
}

static void fail_motion_feedback(void)
{
    Board2_LinkSetPhotoFillLight(0U);
    discard_motion_feedback_axes(s_motion_axis_mask);
    s_motion_axis_mask = 0U;
    s_motion_waiting = 0U;
    s_waiting = 0U;
    s_group_active = 0U;
    s_group_step_active = 0U;
    g_board2_action_running = 0U;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    g_board2_action_result = BOARD2_ACTION_RESULT_ERROR;
    s_motion_feedback_fault = 1U;
}

static uint8_t wait_one_axis_feedback(Board2Axis_t axis)
{
    uint32_t timeout_ms = s_motion_duration_ms + BOARD2_ACTION_CLOSED_LOOP_TIMEOUT_MARGIN_MS;

    switch (axis)
    {
        case BOARD2_AXIS_X:
            return stepper_multi_wait_reached_dual_opposite_uart2(BOARD2_AXIS_X_ADDR_ID1,
                                                                  BOARD2_AXIS_X_ADDR_ID2,
                                                                  timeout_ms);
        case BOARD2_AXIS_Y:
            return stepper_multi_wait_reached(BOARD2_AXIS_Y_CHANNEL,
                                              BOARD2_AXIS_Y_ADDR,
                                              timeout_ms);
        case BOARD2_AXIS_Z:
            return stepper_multi_wait_reached(BOARD2_AXIS_Z_CHANNEL,
                                              BOARD2_AXIS_Z_ADDR,
                                              timeout_ms);
        case BOARD2_AXIS_TELESCOPE:
            return 1U;
        default:
            return 1U;
    }
}

static uint8_t motion_feedback_done(void)
{
#if (BOARD2_ACTION_CLOSED_LOOP_ENABLE != 0U)
    uint8_t mask = s_motion_axis_mask;

    if (mask == 0U)
    {
        clear_motion_feedback_fault();
        return 1U;
    }

    if (((mask & axis_bit(BOARD2_AXIS_X)) != 0U) && (wait_one_axis_feedback(BOARD2_AXIS_X) == 0U))
    {
        fail_motion_feedback();
        return 1U;
    }

    if (((mask & axis_bit(BOARD2_AXIS_Y)) != 0U) && (wait_one_axis_feedback(BOARD2_AXIS_Y) == 0U))
    {
        fail_motion_feedback();
        return 1U;
    }

    if (((mask & axis_bit(BOARD2_AXIS_Z)) != 0U) && (wait_one_axis_feedback(BOARD2_AXIS_Z) == 0U))
    {
        fail_motion_feedback();
        return 1U;
    }

    if (((mask & axis_bit(BOARD2_AXIS_TELESCOPE)) != 0U) &&
        (wait_one_axis_feedback(BOARD2_AXIS_TELESCOPE) == 0U))
    {
        fail_motion_feedback();
        return 1U;
    }

    commit_motion_feedback_axes(mask);
    s_motion_axis_mask = 0U;
    clear_motion_feedback_fault();
#else
    commit_motion_feedback_axes(s_motion_axis_mask);
    s_motion_axis_mask = 0U;
    clear_motion_feedback_fault();
#endif
    return 1U;
}

static uint8_t motion_done(void)
{
    if (s_motion_waiting == 0U)
    {
        if (s_motion_axis_mask != 0U)
        {
            return motion_feedback_done();
        }

        clear_motion_feedback_fault();
        return 1U;
    }

    if ((uint32_t)(g_board2_tick_ms - s_motion_start_ms) >= s_motion_duration_ms)
    {
        s_motion_waiting = 0U;
        return motion_feedback_done();
    }

    return 0U;
}

static void wait_motion_complete_blocking(void)
{
    while (motion_done() == 0U)
    {
        delay_ms(1U);
    }
}

static uint8_t wait_motion_complete_blocking_ok(void)
{
    wait_motion_complete_blocking();
    if (motion_feedback_faulted() == 0U)
    {
        return 1U;
    }

    g_board2_action_running = 0U;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    return 0U;
}

static void start_wait(uint32_t wait_ms)
{
    s_wait_duration_ms = wait_ms;
    if (wait_ms == 0U)
    {
        s_waiting = 0U;
        return;
    }

    s_wait_start_ms = g_board2_tick_ms;
    s_waiting = 1U;
}

static uint8_t wait_done(void)
{
    if (s_waiting == 0U)
    {
        return 1U;
    }

    if ((uint32_t)(g_board2_tick_ms - s_wait_start_ms) >= s_wait_duration_ms)
    {
        s_waiting = 0U;
        return 1U;
    }

    return 0U;
}

static void track_axis(Board2Axis_t axis, float signed_angle_deg)
{
    volatile float *accum = 0;
    volatile uint32_t *count = 0;

    switch (axis)
    {
        case BOARD2_AXIS_X:
            accum = &g_board2_x_accum_deg;
            count = &g_board2_x_send_count;
            break;
        case BOARD2_AXIS_Y:
            accum = &g_board2_y_accum_deg;
            count = &g_board2_y_send_count;
            break;
        case BOARD2_AXIS_Z:
            accum = &g_board2_z_accum_deg;
            count = &g_board2_z_send_count;
            break;
        case BOARD2_AXIS_TELESCOPE:
            accum = &g_board2_telescope_accum_deg;
            count = &g_board2_telescope_send_count;
            break;
        default:
            break;
    }

    if (accum != 0)
    {
        *accum += signed_angle_deg;
    }

    if (count != 0)
    {
        (*count)++;
    }
}

static void commit_axis_motion(Board2Axis_t axis, float signed_angle_deg)
{
    set_axis_position(axis, get_axis_position(axis) + signed_angle_deg);
    track_axis(axis, signed_angle_deg);
}

static void queue_axis_motion(Board2Axis_t axis, float signed_angle_deg)
{
    if ((uint8_t)axis >= 4U)
    {
        return;
    }

    s_motion_pending_delta[(uint8_t)axis] += signed_angle_deg;
}

static void commit_motion_feedback_axes(uint8_t mask)
{
    uint8_t i;

    for (i = 0U; i < 4U; i++)
    {
        if (((mask & (uint8_t)(1U << i)) != 0U) && (s_motion_pending_delta[i] != 0.0f))
        {
            commit_axis_motion((Board2Axis_t)i, s_motion_pending_delta[i]);
            s_motion_pending_delta[i] = 0.0f;
        }
    }
}

static void discard_motion_feedback_axes(uint8_t mask)
{
    uint8_t i;

    for (i = 0U; i < 4U; i++)
    {
        if ((mask & (uint8_t)(1U << i)) != 0U)
        {
            s_motion_pending_delta[i] = 0.0f;
        }
    }
}

static float get_axis_position(Board2Axis_t axis)
{
    switch (axis)
    {
        case BOARD2_AXIS_X: return s_position.x_deg;
        case BOARD2_AXIS_Y: return s_position.y_deg;
        case BOARD2_AXIS_Z: return s_position.z_deg;
        case BOARD2_AXIS_TELESCOPE: return g_board2_telescope_accum_deg;
        default: return 0.0f;
    }
}

static void set_axis_position(Board2Axis_t axis, float angle_deg)
{
    switch (axis)
    {
        case BOARD2_AXIS_X:
            s_position.x_deg = angle_deg;
            break;
        case BOARD2_AXIS_Y:
            s_position.y_deg = angle_deg;
            break;
        case BOARD2_AXIS_Z:
            s_position.z_deg = angle_deg;
            break;
        default:
            break;
    }
}

static void move_axis_relative(Board2Axis_t axis,
                               float signed_angle_deg,
                               uint16_t speed_rpm,
                               uint16_t accel)
{
    uint8_t dir;
    float angle_deg = BOARD2_FLOAT_ABS(signed_angle_deg);
    uint16_t limited_speed = limit_axis_speed(axis, speed_rpm);
    uint8_t limited_accel = limit_accel(accel);

    if (angle_deg <= 0.0f)
    {
        return;
    }

    switch (axis)
    {
        case BOARD2_AXIS_TELESCOPE:
            s_motion_pending_delta[(uint8_t)axis] = 0.0f;
            dir = (signed_angle_deg < 0.0f) ? BOARD2_AXIS_TELESCOPE_HOME_DIR : BOARD2_AXIS_TELESCOPE_KEY_DIR;
            stepper_multi_enable(BOARD2_AXIS_TELESCOPE_CHANNEL, BOARD2_AXIS_TELESCOPE_ADDR, true);
            stepper_multi_move_angle(BOARD2_AXIS_TELESCOPE_CHANNEL,
                                     BOARD2_AXIS_TELESCOPE_ADDR,
                                     dir,
                                     limited_speed,
                                     limited_accel,
                                     angle_deg);
            commit_axis_motion(axis, signed_angle_deg);
            start_motion_wait(axis, motion_time_ms(angle_deg, limited_speed));
            return;

        case BOARD2_AXIS_X:
            dir = (signed_angle_deg < 0.0f) ? BOARD2_AXIS_X_HOME_DIR : BOARD2_AXIS_X_KEY_DIR;
            stepper_multi_enable_dual_opposite_uart2(BOARD2_AXIS_X_ADDR_ID1, BOARD2_AXIS_X_ADDR_ID2, true);
            stepper_multi_move_angle_dual_opposite_uart2(BOARD2_AXIS_X_ADDR_ID1,
                                                         BOARD2_AXIS_X_ADDR_ID2,
                                                         dir,
                                                         limited_speed,
                                                         limited_accel,
                                                         angle_deg);
            break;

        case BOARD2_AXIS_Y:
            dir = (signed_angle_deg < 0.0f) ? BOARD2_AXIS_Y_HOME_DIR : BOARD2_AXIS_Y_KEY_DIR;
            stepper_multi_enable(BOARD2_AXIS_Y_CHANNEL, BOARD2_AXIS_Y_ADDR, true);
            stepper_multi_move_angle(BOARD2_AXIS_Y_CHANNEL, BOARD2_AXIS_Y_ADDR, dir, limited_speed, limited_accel, angle_deg);
            break;

        case BOARD2_AXIS_Z:
            dir = (signed_angle_deg < 0.0f) ? BOARD2_AXIS_Z_HOME_DIR : BOARD2_AXIS_Z_KEY_DIR;
            stepper_multi_enable(BOARD2_AXIS_Z_CHANNEL, BOARD2_AXIS_Z_ADDR, true);
            stepper_multi_move_angle(BOARD2_AXIS_Z_CHANNEL, BOARD2_AXIS_Z_ADDR, dir, limited_speed, limited_accel, angle_deg);
            break;

        default:
            return;
    }

    queue_axis_motion(axis, signed_angle_deg);
    start_motion_wait(axis, motion_time_ms(angle_deg, limited_speed));
}

static void move_axis_to(Board2Axis_t axis,
                         float target_deg,
                         uint16_t speed_rpm,
                         uint16_t accel)
{
    move_axis_relative(axis, target_deg - get_axis_position(axis), speed_rpm, accel);
}

static void move_xy_sync(float target_x_deg, float target_y_deg)
{
    float x_delta = target_x_deg - s_position.x_deg;
    float y_delta = target_y_deg - s_position.y_deg;
    float x_angle = BOARD2_FLOAT_ABS(x_delta);
    float y_angle = BOARD2_FLOAT_ABS(y_delta);
    uint16_t x_base_speed = limit_axis_speed(BOARD2_AXIS_X, BOARD2_AXIS_X_SPEED_RPM);
    uint16_t y_base_speed = limit_axis_speed(BOARD2_AXIS_Y, BOARD2_AXIS_Y_SPEED_RPM);
    uint16_t x_speed = x_base_speed;
    uint16_t y_speed = y_base_speed;
    uint8_t x_accel = limit_accel(BOARD2_AXIS_X_ACCEL);
    uint8_t y_accel = limit_accel(BOARD2_AXIS_Y_ACCEL);
    uint32_t x_run_ms = 0U;
    uint32_t y_run_ms = 0U;
    uint32_t x_delay_ms = 0U;
    uint32_t y_delay_ms = 0U;
    uint32_t target_run_ms;

    if ((x_angle <= 0.0f) && (y_angle <= 0.0f))
    {
        return;
    }

#if (BOARD2_ACTION_XY_SYNC_ENABLE != 0U)
    /*
     * 新同步算法：
     * 1. 先分别按 X/Y 自己的最高可用速度估算最快运行时间。
     * 2. 取较慢轴的运行时间作为共同目标时间。
     * 3. 反推另一轴转速，而不是简单按距离比例缩放。
     * 这样能消除 X=1000rpm、Y=1500rpm 这种基准速度不同造成的到位时间差。
     */
    x_run_ms = motion_run_time_ms(x_angle, x_base_speed);
    y_run_ms = motion_run_time_ms(y_angle, y_base_speed);
    target_run_ms = (x_run_ms > y_run_ms) ? x_run_ms : y_run_ms;

    if (x_angle > 0.0f)
    {
        x_speed = speed_for_run_time(x_angle, target_run_ms, x_base_speed);
        x_accel = scale_accel_for_speed(BOARD2_AXIS_X_ACCEL, x_base_speed, x_speed);
        x_run_ms = motion_run_time_ms(x_angle, x_speed);
#if (BOARD2_ACTION_XY_SYNC_DELAY_ENABLE != 0U)
        if ((target_run_ms > x_run_ms) &&
            ((target_run_ms - x_run_ms) >= BOARD2_ACTION_XY_SYNC_DELAY_THRESHOLD_MS))
        {
            x_delay_ms = target_run_ms - x_run_ms;
        }
#endif
    }

    if (y_angle > 0.0f)
    {
        y_speed = speed_for_run_time(y_angle, target_run_ms, y_base_speed);
        y_accel = scale_accel_for_speed(BOARD2_AXIS_Y_ACCEL, y_base_speed, y_speed);
        y_run_ms = motion_run_time_ms(y_angle, y_speed);
#if (BOARD2_ACTION_XY_SYNC_DELAY_ENABLE != 0U)
        if ((target_run_ms > y_run_ms) &&
            ((target_run_ms - y_run_ms) >= BOARD2_ACTION_XY_SYNC_DELAY_THRESHOLD_MS))
        {
            y_delay_ms = target_run_ms - y_run_ms;
        }
#endif
    }
#endif

    if ((x_angle > 0.0f) && (y_angle > 0.0f))
    {
        if (x_delay_ms > y_delay_ms)
        {
            move_axis_relative(BOARD2_AXIS_Y, y_delta, y_speed, y_accel);
            delay_ms(x_delay_ms - y_delay_ms);
            move_axis_relative(BOARD2_AXIS_X, x_delta, x_speed, x_accel);
        }
        else
        {
            move_axis_relative(BOARD2_AXIS_X, x_delta, x_speed, x_accel);
            if (y_delay_ms > x_delay_ms)
            {
                delay_ms(y_delay_ms - x_delay_ms);
            }
            move_axis_relative(BOARD2_AXIS_Y, y_delta, y_speed, y_accel);
        }
    }
    else if (x_angle > 0.0f)
    {
        move_axis_relative(BOARD2_AXIS_X, x_delta, x_speed, x_accel);
    }
    else
    {
        move_axis_relative(BOARD2_AXIS_Y, y_delta, y_speed, y_accel);
    }
}

static uint8_t get_point(uint8_t point_id, const Board2ActionPoint_t **point)
{
    if ((point == 0) || (point_id >= BOARD2_ARRAY_COUNT(s_points)))
    {
        return 0U;
    }

    *point = &s_points[point_id];
    return 1U;
}

static void move_to_point(uint8_t point_id)
{
    const Board2ActionPoint_t *point;
    float safe_z_deg;

    if (get_point(point_id, &point) == 0U)
    {
        return;
    }

    /* Z正方向为上升：先抬到目标高度和当前高度中较高的位置，再移动XY，避免低高度横移。 */
    safe_z_deg = (point->z_deg > s_position.z_deg) ? point->z_deg : s_position.z_deg;
    clear_motion_feedback_fault();
    move_axis_to(BOARD2_AXIS_Z, safe_z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_xy_sync(point->x_deg, point->y_deg);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_axis_to(BOARD2_AXIS_Z, point->z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    g_board2_action_last_point = point_id;
}

static void move_to_a_point(uint8_t point_id)
{
    const Board2ActionPoint_t *point;
    float safe_z_deg;
    float first_pre_x_deg;
    float pre_y_deg;

    if (get_point(point_id, &point) == 0U)
    {
        return;
    }

    safe_z_deg = (point->z_deg > s_position.z_deg) ? point->z_deg : s_position.z_deg;
    clear_motion_feedback_fault();
    move_axis_to(BOARD2_AXIS_Z, safe_z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }

    first_pre_x_deg = point->x_deg + BOARD2_ACTION_A_PRE_X_OFFSET_DEG;
    pre_y_deg = point->y_deg + BOARD2_ACTION_A_PRE_Y_OFFSET_DEG;

    move_xy_sync(first_pre_x_deg, pre_y_deg);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_xy_sync(point->x_deg, pre_y_deg);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_xy_sync(point->x_deg, point->y_deg);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_axis_to(BOARD2_AXIS_Z, point->z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    g_board2_action_last_point = point_id;
}

static float get_c_pre_y_offset(uint8_t point_id)
{
    if ((point_id == BOARD2_ACTION_POINT_C01) &&
        ((s_workflow_mode == BOARD2_ACTION_MODE_DEFAULT) ||
         (s_workflow_mode == BOARD2_ACTION_MODE_TAKE_C_TO_A)))
    {
        return -BOARD2_ACTION_C_PRE_Y_OFFSET_DEG;
    }

    return BOARD2_ACTION_C_PRE_Y_OFFSET_DEG;
}

static void move_to_c_pre_point(uint8_t point_id)
{
    const Board2ActionPoint_t *point;
    float safe_z_deg;
    float c_pre_y_offset_deg;
    if (get_point(point_id, &point) == 0U)
    {
        return;
    }

    /* C点预进入：默认存放/取衣到C01时从Y-侧靠近，其余C点仍从Y+侧靠近。 */
    c_pre_y_offset_deg = get_c_pre_y_offset(point_id);
    safe_z_deg = (point->z_deg > s_position.z_deg) ? point->z_deg : s_position.z_deg;
    clear_motion_feedback_fault();
    move_axis_to(BOARD2_AXIS_Z, safe_z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_xy_sync(point->x_deg, point->y_deg + c_pre_y_offset_deg);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_axis_to(BOARD2_AXIS_Z, point->z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    g_board2_action_last_point = point_id;
}

static void move_to_c_x_pre_point(uint8_t point_id)
{
    const Board2ActionPoint_t *point;
    float safe_z_deg;
    float c_pre_y_offset_deg;
    if (get_point(point_id, &point) == 0U)
    {
        return;
    }

    /* C点前置停靠：默认存放/取衣到C01时Y偏置取反，其余C点保持Y+。 */
    c_pre_y_offset_deg = get_c_pre_y_offset(point_id);
    safe_z_deg = (point->z_deg > s_position.z_deg) ? point->z_deg : s_position.z_deg;
    clear_motion_feedback_fault();
    move_axis_to(BOARD2_AXIS_Z, safe_z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_xy_sync(point->x_deg - BOARD2_ACTION_C_PRE_X_OFFSET_DEG,
                 point->y_deg + c_pre_y_offset_deg);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_axis_to(BOARD2_AXIS_Z, point->z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    g_board2_action_last_point = point_id;
}

static void move_to_c_point(uint8_t point_id)
{
    const Board2ActionPoint_t *point;
    float safe_z_deg;
    if (get_point(point_id, &point) == 0U)
    {
        return;
    }

    safe_z_deg = (point->z_deg > s_position.z_deg) ? point->z_deg : s_position.z_deg;
    clear_motion_feedback_fault();
    move_axis_to(BOARD2_AXIS_Z, safe_z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_xy_sync(point->x_deg, point->y_deg);
    if (wait_motion_complete_blocking_ok() == 0U)
    {
        return;
    }
    move_axis_to(BOARD2_AXIS_Z, point->z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    g_board2_action_last_point = point_id;
}

static void move_z_to_point(uint8_t point_id)
{
    const Board2ActionPoint_t *point;

    if (get_point(point_id, &point) == 0U)
    {
        return;
    }

    clear_motion_feedback_fault();
    move_axis_to(BOARD2_AXIS_Z, point->z_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
    g_board2_action_last_point = point_id;
}

static void move_xy_to_point_keep_z(uint8_t point_id)
{
    const Board2ActionPoint_t *point;

    if (get_point(point_id, &point) == 0U)
    {
        return;
    }

    clear_motion_feedback_fault();
    move_xy_sync(point->x_deg, point->y_deg);
    g_board2_action_last_point = point_id;
}

static void move_z_to_take_brush_height(void)
{
    clear_motion_feedback_fault();
    move_axis_to(BOARD2_AXIS_Z,
                 BOARD2_ACTION_TAKE_BRUSH_Z_DEG,
                 BOARD2_AXIS_Z_SPEED_RPM,
                 BOARD2_AXIS_Z_ACCEL);
}

static void move_z_up_after_take_brush(void)
{
    clear_motion_feedback_fault();
    move_axis_to(BOARD2_AXIS_Z,
                 BOARD2_ACTION_TAKE_BRUSH_Z_UP_DEG,
                 BOARD2_AXIS_Z_SPEED_RPM,
                 BOARD2_AXIS_Z_ACCEL);
}

static void move_z_to_b_rotate_range(void)
{
    float z_error = s_position.z_deg - BOARD2_ACTION_SERVO1_ROTATE_Z_DEG;

    if (z_error < 0.0f)
    {
        z_error = -z_error;
    }

    clear_motion_feedback_fault();
    if (z_error <= BOARD2_ACTION_B_ROTATE_Z_TOLERANCE_DEG)
    {
        return;
    }

    move_axis_to(BOARD2_AXIS_Z,
                 BOARD2_ACTION_B_ROTATE_Z_TARGET_DEG,
                 BOARD2_AXIS_Z_SPEED_RPM,
                 BOARD2_AXIS_Z_ACCEL);
}

static uint8_t get_current_a_point(void)
{
    if ((s_workflow_mode == BOARD2_ACTION_MODE_DEFAULT) &&
        (s_current_a_index < BOARD2_ARRAY_COUNT(s_default_a_points)))
    {
        return s_default_a_points[s_current_a_index];
    }

    return (uint8_t)(BOARD2_ACTION_A_FIRST_POINT + s_current_a_index);
}

static uint8_t get_current_c_point(void)
{
    return (uint8_t)(BOARD2_ACTION_C_FIRST_POINT + s_current_c_index);
}

static uint8_t get_next_c_index_for_new_store(void)
{
    return (g_board2_action_next_c_index < BOARD2_ACTION_C_COUNT) ?
           g_board2_action_next_c_index :
           BOARD2_ACTION_C_COUNT;
}

static void advance_next_c_index_after_place(void)
{
    if (s_current_c_index < BOARD2_ACTION_C_COUNT)
    {
        s_current_c_index++;
    }
    g_board2_action_next_c_index = s_current_c_index;
}

static uint8_t default_a_candidate_count(void)
{
    uint8_t count = BOARD2_ARRAY_COUNT(s_default_a_points);

    if (BOARD2_ACTION_A_COUNT < count)
    {
        count = BOARD2_ACTION_A_COUNT;
    }

    if (BOARD2_ARRAY_COUNT(s_default_a_switches) < count)
    {
        count = BOARD2_ARRAY_COUNT(s_default_a_switches);
    }

    return count;
}

static void read_default_a_switches(uint8_t switch_state[])
{
    uint8_t i;

    /*
     * OLED shows Read(1), Read(2), Read(3), Read(4) left to right.
     * Storage mapping is A1<-Read(4), A2<-Read(3), A3<-Read(2), A4<-Read(1).
     * Board2_AdcIo_Read() already inverts the low-level hardware signal; 1 means has clothes.
     */
    for (i = 0U; i < BOARD2_ARRAY_COUNT(s_default_a_switches); i++)
    {
        switch_state[i] = Board2_AdcIo_Read(s_default_a_switches[i]);
    }
}

static uint8_t select_next_default_a_point(void)
{
    uint8_t i;
    uint8_t count = default_a_candidate_count();
    uint8_t switch_state[BOARD2_ARRAY_COUNT(s_default_a_switches)];

    if (s_workflow_mode != BOARD2_ACTION_MODE_DEFAULT)
    {
        return 1U;
    }

    if ((s_current_c_index >= BOARD2_ACTION_C_COUNT) ||
        (get_current_c_point() >= BOARD2_ARRAY_COUNT(s_points)))
    {
        return 0U;
    }

    read_default_a_switches(switch_state);

    for (i = s_current_a_index; i < count; i++)
    {
        if ((s_default_a_points[i] < BOARD2_ARRAY_COUNT(s_points)) &&
            (switch_state[i] == 1U))
        {
            s_current_a_index = i;
            return 1U;
        }
    }

    s_current_a_index = count;
    return 0U;
}

static uint8_t current_default_a_has_clothes(void)
{
    uint8_t count = default_a_candidate_count();
    uint8_t switch_state[BOARD2_ARRAY_COUNT(s_default_a_switches)];

    if (s_workflow_mode != BOARD2_ACTION_MODE_DEFAULT)
    {
        return 1U;
    }

    if (s_current_a_index >= count)
    {
        return 0U;
    }

    read_default_a_switches(switch_state);
    return (switch_state[s_current_a_index] == 1U) ? 1U : 0U;
}

static uint8_t workflow_has_item(void)
{
    uint8_t a_point = get_current_a_point();
    uint8_t c_point = get_current_c_point();

    if (s_workflow_mode == BOARD2_ACTION_MODE_TAKE_C_TO_A)
    {
        return ((get_current_c_point() < BOARD2_ARRAY_COUNT(s_points)) &&
                (BOARD2_ACTION_POINT_B3 < BOARD2_ARRAY_COUNT(s_points)) &&
                (BOARD2_ACTION_POINT_B4 < BOARD2_ARRAY_COUNT(s_points)) &&
                (BOARD2_ACTION_POINT_A01 < BOARD2_ARRAY_COUNT(s_points))) ? 1U : 0U;
    }

    if ((s_workflow_mode == BOARD2_ACTION_MODE_STORE_PREPARE) ||
        (s_workflow_mode == BOARD2_ACTION_MODE_STORE_RETRACT) ||
        (s_workflow_mode == BOARD2_ACTION_MODE_FETCH_RESET))
    {
        return (BOARD2_ACTION_POINT_A0 < BOARD2_ARRAY_COUNT(s_points)) ? 1U : 0U;
    }

    /* 防止调大A/C数量后超过点位表，避免空跑后续抓取、拍照、放置动作 */
    return ((s_current_a_index < default_a_candidate_count()) &&
            (s_current_c_index < BOARD2_ACTION_C_COUNT) &&
            (a_point < BOARD2_ARRAY_COUNT(s_points)) &&
            (c_point < BOARD2_ARRAY_COUNT(s_points))) ? 1U : 0U;
}

static int get_other_servo1_angle(int current_deg)
{
    int front_diff = current_deg - BOARD2_ACTION_SERVO1_FRONT_DEG;
    int back_diff = current_deg - BOARD2_ACTION_SERVO1_BACK_DEG;

    if (front_diff < 0)
    {
        front_diff = -front_diff;
    }
    if (back_diff < 0)
    {
        back_diff = -back_diff;
    }

    return (front_diff <= back_diff) ?
           BOARD2_ACTION_SERVO1_BACK_DEG :
           BOARD2_ACTION_SERVO1_FRONT_DEG;
}

static int clamp_servo_angle(int angle_deg)
{
    if (angle_deg < (int)TIM1_SERVO_MIN_ANGLE_DEG)
    {
        return (int)TIM1_SERVO_MIN_ANGLE_DEG;
    }

    if (angle_deg > (int)TIM1_SERVO_MAX_ANGLE_DEG)
    {
        return (int)TIM1_SERVO_MAX_ANGLE_DEG;
    }

    return angle_deg;
}

static int servo_ease_step(int diff_deg)
{
    int abs_diff = (diff_deg < 0) ? -diff_deg : diff_deg;
    int step_deg;

    if (abs_diff > BOARD2_ACTION_SERVO_FAST_ZONE_DEG)
    {
        step_deg = BOARD2_ACTION_SERVO_FAST_STEP_DEG;
    }
    else if (abs_diff > BOARD2_ACTION_SERVO_MID_ZONE_DEG)
    {
        step_deg = BOARD2_ACTION_SERVO_MID_STEP_DEG;
    }
    else
    {
        step_deg = BOARD2_ACTION_SERVO_SLOW_STEP_DEG;
    }

    if (step_deg > abs_diff)
    {
        step_deg = abs_diff;
    }

    return (diff_deg < 0) ? -step_deg : step_deg;
}

static uint8_t servo_apply_one(u8 channel, volatile int *current_deg, int target_deg, uint8_t *active)
{
    int diff_deg;
    int next_deg;

    if ((current_deg == 0) || (active == 0) || (*active == 0U))
    {
        return 1U;
    }

    diff_deg = target_deg - *current_deg;
    if (diff_deg == 0)
    {
        *active = 0U;
        return 1U;
    }

    next_deg = *current_deg + servo_ease_step(diff_deg);
    TIM1_Servo_SetAngle(channel, next_deg);
    *current_deg = next_deg;

    if (next_deg == target_deg)
    {
        *active = 0U;
        return 1U;
    }

    return 0U;
}

static void set_servos_smooth(int servo1_deg, int servo2_deg)
{
    if (servo1_deg != BOARD2_ACTION_SERVO_KEEP)
    {
        s_servo1_target_deg = clamp_servo_angle(servo1_deg);
        s_servo1_active = (g_board2_servo_ch1_angle_deg != s_servo1_target_deg) ? 1U : 0U;
    }

    if (servo2_deg != BOARD2_ACTION_SERVO_KEEP)
    {
        s_servo2_target_deg = clamp_servo_angle(servo2_deg);
        s_servo2_active = (g_board2_servo_ch2_angle_deg != s_servo2_target_deg) ? 1U : 0U;
    }

    s_servo_last_update_ms = g_board2_tick_ms;
}

static uint8_t servo_task(void)
{
    uint8_t servo1_done;
    uint8_t servo2_done;

    if ((s_servo1_active == 0U) && (s_servo2_active == 0U))
    {
        return 1U;
    }

    if ((uint32_t)(g_board2_tick_ms - s_servo_last_update_ms) < BOARD2_ACTION_SERVO_UPDATE_PERIOD_MS)
    {
        return 0U;
    }

    s_servo_last_update_ms = g_board2_tick_ms;
    servo1_done = servo_apply_one(TIM1_SERVO_CH1,
                                  &g_board2_servo_ch1_angle_deg,
                                  s_servo1_target_deg,
                                  &s_servo1_active);
    servo2_done = servo_apply_one(TIM1_SERVO_CH2,
                                  &g_board2_servo_ch2_angle_deg,
                                  s_servo2_target_deg,
                                  &s_servo2_active);

    return ((servo1_done != 0U) && (servo2_done != 0U)) ? 1U : 0U;
}

static uint8_t get_group(uint8_t group_id, const Board2ActionGroup_t **group)
{
    if ((group == 0) || (group_id >= BOARD2_ACTION_GROUP_COUNT))
    {
        return 0U;
    }

    *group = &s_groups[group_id];
    return 1U;
}

static void start_group(uint8_t group_id)
{
    const Board2ActionGroup_t *group;

    if ((get_group(group_id, &group) == 0U) || (group->step_count == 0U))
    {
        s_group_active = 0U;
        g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
        g_board2_action_current_group_step = 0U;
        return;
    }

    g_board2_action_current_group = group_id;
    g_board2_action_current_group_step = 0U;
    s_group_active = 1U;
    s_group_step_active = 0U;
}

static uint8_t group_task(void)
{
    const Board2ActionGroup_t *group;
    const Board2ActionGroupStep_t *step;

    if (s_group_active == 0U)
    {
        return 1U;
    }

    if (motion_done() == 0U)
    {
        return 0U;
    }
    if (g_board2_action_running == 0U)
    {
        return 1U;
    }

    if ((get_group(g_board2_action_current_group, &group) == 0U) ||
        (g_board2_action_current_group_step >= group->step_count))
    {
        s_group_active = 0U;
        s_group_step_active = 0U;
        g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
        g_board2_action_current_group_step = 0U;
        return 1U;
    }

    step = &group->steps[g_board2_action_current_group_step];

    if (s_group_step_active == 0U)
    {
        move_axis_to(BOARD2_AXIS_Z, step->z_target_deg, BOARD2_AXIS_Z_SPEED_RPM, BOARD2_AXIS_Z_ACCEL);
        set_servos_smooth(step->servo1_deg, step->servo2_deg);
        start_wait(step->wait_ms);
        s_group_step_active = 1U;
    }

    if ((servo_task() != 0U) && (wait_done() != 0U))
    {
        s_group_step_active = 0U;
        g_board2_action_current_group_step++;
    }

    return 0U;
}

static void clear_runtime_state(void)
{
    s_wait_start_ms = 0U;
    s_wait_duration_ms = 0U;
    s_waiting = 0U;
    s_motion_start_ms = 0U;
    s_motion_duration_ms = 0U;
    s_motion_waiting = 0U;
    s_motion_axis_mask = 0U;
    s_motion_pending_delta[0] = 0.0f;
    s_motion_pending_delta[1] = 0.0f;
    s_motion_pending_delta[2] = 0.0f;
    s_motion_pending_delta[3] = 0.0f;
    s_group_active = 0U;
    s_group_step_active = 0U;
    s_servo_last_update_ms = 0U;
    s_servo1_active = 0U;
    s_servo2_active = 0U;
    s_motion_feedback_fault = 0U;
}

void Board2_ActionList_Init(void)
{
    g_board2_action_running = 0U;
    g_board2_action_current_step = BOARD2_ACTION_WORKFLOW_HOME_TO_A0;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    g_board2_action_last_point = BOARD2_ACTION_POINT_HOME;
    s_workflow_state = BOARD2_ACTION_WORKFLOW_HOME_TO_A0;
    s_workflow_mode = BOARD2_ACTION_MODE_DEFAULT;
    s_current_a_index = 0U;
    s_current_c_index = 0U;
    g_board2_action_next_c_index = 0U;
    g_board2_action_target_c = 0U;
    g_board2_action_result = BOARD2_ACTION_RESULT_IDLE;
    s_single_store_item = 0U;
    s_next_b_servo1_deg = BOARD2_ACTION_SERVO1_BACK_DEG;
    s_b_flip_count = 0U;
    s_position.x_deg = 0.0f;
    s_position.y_deg = 0.0f;
    s_position.z_deg = 0.0f;
    s_servo1_target_deg = BOARD2_ACTION_SERVO1_FRONT_DEG;
    s_servo2_target_deg = TIM1_SERVO2_INIT_ANGLE_DEG;
    clear_runtime_state();
}

void Board2_ActionList_StartDefault(void)
{
    s_workflow_state = BOARD2_ACTION_WORKFLOW_HOME_TO_A0;
    s_workflow_mode = BOARD2_ACTION_MODE_DEFAULT;
    g_board2_action_current_step = s_workflow_state;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    g_board2_action_last_point = BOARD2_ACTION_POINT_HOME;
    s_current_a_index = 0U;
    s_current_c_index = get_next_c_index_for_new_store();
    g_board2_action_target_c = (s_current_c_index < BOARD2_ACTION_C_COUNT) ?
                               (uint8_t)(s_current_c_index + 1U) : 0U;
    g_board2_action_result = BOARD2_ACTION_RESULT_RUNNING;
    s_single_store_item = 0U;
    s_b_flip_count = 0U;
    s_position.x_deg = g_board2_x_accum_deg;
    s_position.y_deg = g_board2_y_accum_deg;
    s_position.z_deg = g_board2_z_accum_deg;
    g_board2_servo_ch1_angle_deg = BOARD2_ACTION_SERVO1_FRONT_DEG;
    TIM1_Servo_SetAngle(TIM1_SERVO_CH1, BOARD2_ACTION_SERVO1_FRONT_DEG);
    g_board2_servo_ch4_angle_deg = BOARD2_ACTION_STORE_SERVO4_DEG;
    TIM1_Servo_SetAngle(TIM1_SERVO_CH4, BOARD2_ACTION_STORE_SERVO4_DEG);
    s_servo1_target_deg = BOARD2_ACTION_SERVO1_FRONT_DEG;
    s_servo2_target_deg = g_board2_servo_ch2_angle_deg;
    s_next_b_servo1_deg = BOARD2_ACTION_SERVO1_BACK_DEG;
    clear_runtime_state();
    Board2_LinkSetPhotoFillLight(0U);
    g_board2_action_running = 1U;
}

void Board2_ActionList_StartDefaultToC(uint8_t target_c)
{
    Board2_ActionList_StartDefault();
    if ((target_c < 1U) || (target_c > BOARD2_ACTION_C_COUNT))
    {
        g_board2_action_running = 0U;
        g_board2_action_result = BOARD2_ACTION_RESULT_ERROR;
        g_board2_action_target_c = target_c;
        return;
    }
    s_current_c_index = (uint8_t)(target_c - 1U);
    g_board2_action_target_c = target_c;
    s_single_store_item = 1U;
}

void Board2_ActionList_StartStorePrepare(void)
{
    s_workflow_state = BOARD2_ACTION_WORKFLOW_HOME_TO_A0;
    s_workflow_mode = BOARD2_ACTION_MODE_STORE_PREPARE;
    g_board2_action_current_step = s_workflow_state;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    g_board2_action_last_point = BOARD2_ACTION_POINT_HOME;
    s_current_a_index = 0U;
    s_current_c_index = 0U;
    g_board2_action_target_c = 0U;
    g_board2_action_result = BOARD2_ACTION_RESULT_RUNNING;
    s_single_store_item = 0U;
    s_b_flip_count = 0U;
    s_position.x_deg = g_board2_x_accum_deg;
    s_position.y_deg = g_board2_y_accum_deg;
    s_position.z_deg = g_board2_z_accum_deg;
    s_servo1_target_deg = g_board2_servo_ch1_angle_deg;
    s_servo2_target_deg = g_board2_servo_ch2_angle_deg;
    clear_runtime_state();
    Board2_LinkSetPhotoFillLight(0U);
    g_board2_action_running = 1U;
}

void Board2_ActionList_StartStoreRetract(void)
{
    s_workflow_state = BOARD2_ACTION_WORKFLOW_TELESCOPE_RETRACT;
    s_workflow_mode = BOARD2_ACTION_MODE_STORE_RETRACT;
    g_board2_action_current_step = s_workflow_state;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    g_board2_action_last_point = BOARD2_ACTION_POINT_HOME;
    s_current_a_index = 0U;
    s_current_c_index = 0U;
    g_board2_action_target_c = 0U;
    g_board2_action_result = BOARD2_ACTION_RESULT_RUNNING;
    s_single_store_item = 0U;
    s_b_flip_count = 0U;
    s_position.x_deg = g_board2_x_accum_deg;
    s_position.y_deg = g_board2_y_accum_deg;
    s_position.z_deg = g_board2_z_accum_deg;
    s_servo1_target_deg = g_board2_servo_ch1_angle_deg;
    s_servo2_target_deg = g_board2_servo_ch2_angle_deg;
    clear_runtime_state();
    Board2_LinkSetPhotoFillLight(0U);
    g_board2_action_running = 1U;
}

void Board2_ActionList_StartFetchReset(void)
{
    s_workflow_state = BOARD2_ACTION_WORKFLOW_FETCH_RESET_TELESCOPE_HOME;
    s_workflow_mode = BOARD2_ACTION_MODE_FETCH_RESET;
    g_board2_action_current_step = s_workflow_state;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    g_board2_action_last_point = BOARD2_ACTION_POINT_HOME;
    s_current_a_index = 0U;
    s_current_c_index = 0U;
    g_board2_action_target_c = 0U;
    g_board2_action_result = BOARD2_ACTION_RESULT_RUNNING;
    s_single_store_item = 0U;
    s_b_flip_count = 0U;
    s_position.x_deg = g_board2_x_accum_deg;
    s_position.y_deg = g_board2_y_accum_deg;
    s_position.z_deg = g_board2_z_accum_deg;
    s_servo1_target_deg = g_board2_servo_ch1_angle_deg;
    s_servo2_target_deg = g_board2_servo_ch2_angle_deg;
    clear_runtime_state();
    Board2_LinkSetPhotoFillLight(0U);
    g_board2_action_running = 1U;
}

void Board2_ActionList_StartDefaultAfterTelescope(void)
{
    s_workflow_state = BOARD2_ACTION_WORKFLOW_Z_TO_A0;
    s_workflow_mode = BOARD2_ACTION_MODE_DEFAULT;
    g_board2_action_current_step = s_workflow_state;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    g_board2_action_last_point = BOARD2_ACTION_POINT_HOME;
    s_current_a_index = 0U;
    s_current_c_index = get_next_c_index_for_new_store();
    g_board2_action_target_c = (s_current_c_index < BOARD2_ACTION_C_COUNT) ?
                               (uint8_t)(s_current_c_index + 1U) : 0U;
    g_board2_action_result = BOARD2_ACTION_RESULT_RUNNING;
    s_single_store_item = 0U;
    s_b_flip_count = 0U;
    s_position.x_deg = g_board2_x_accum_deg;
    s_position.y_deg = g_board2_y_accum_deg;
    s_position.z_deg = g_board2_z_accum_deg;
    g_board2_servo_ch1_angle_deg = BOARD2_ACTION_SERVO1_FRONT_DEG;
    TIM1_Servo_SetAngle(TIM1_SERVO_CH1, BOARD2_ACTION_SERVO1_FRONT_DEG);
    g_board2_servo_ch4_angle_deg = BOARD2_ACTION_STORE_SERVO4_DEG;
    TIM1_Servo_SetAngle(TIM1_SERVO_CH4, BOARD2_ACTION_STORE_SERVO4_DEG);
    s_servo1_target_deg = BOARD2_ACTION_SERVO1_FRONT_DEG;
    s_servo2_target_deg = g_board2_servo_ch2_angle_deg;
    s_next_b_servo1_deg = BOARD2_ACTION_SERVO1_BACK_DEG;
    clear_runtime_state();
    Board2_LinkSetPhotoFillLight(0U);
    g_board2_action_running = 1U;
}

void Board2_ActionList_StartDefaultAfterTelescopeToC(uint8_t target_c)
{
    Board2_ActionList_StartDefaultAfterTelescope();
    if ((target_c < 1U) || (target_c > BOARD2_ACTION_C_COUNT))
    {
        g_board2_action_running = 0U;
        g_board2_action_result = BOARD2_ACTION_RESULT_ERROR;
        g_board2_action_target_c = target_c;
        return;
    }
    s_current_c_index = (uint8_t)(target_c - 1U);
    g_board2_action_target_c = target_c;
    s_single_store_item = 1U;
}

void Board2_ActionList_StartTakeClothes(void)
{
    s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_HOME_TO_A0;
    s_workflow_mode = BOARD2_ACTION_MODE_TAKE_C_TO_A;
    g_board2_action_current_step = s_workflow_state;
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    g_board2_action_last_point = BOARD2_ACTION_POINT_HOME;
    s_current_a_index = 0U;
    s_current_c_index = 0U;
    g_board2_action_target_c = 1U;
    g_board2_action_result = BOARD2_ACTION_RESULT_RUNNING;
    s_single_store_item = 0U;
    s_b_flip_count = 0U;
    s_position.x_deg = g_board2_x_accum_deg;
    s_position.y_deg = g_board2_y_accum_deg;
    s_position.z_deg = g_board2_z_accum_deg;
    /* 数字7取衣流程必须到B过渡点才翻舵机1，启动时只沿用当前舵机1角度，不立即改PWM。 */
    g_board2_servo_ch4_angle_deg = BOARD2_ACTION_TAKE_SERVO4_DEG;
    TIM1_Servo_SetAngle(TIM1_SERVO_CH4, BOARD2_ACTION_TAKE_SERVO4_DEG);
    s_servo1_target_deg = g_board2_servo_ch1_angle_deg;
    s_servo2_target_deg = g_board2_servo_ch2_angle_deg;
    s_next_b_servo1_deg = BOARD2_ACTION_SERVO1_BACK_DEG;
    clear_runtime_state();
    Board2_LinkSetPhotoFillLight(0U);
    g_board2_action_running = 1U;
}

void Board2_ActionList_StartTakeClothesFromC(uint8_t target_c)
{
    Board2_ActionList_StartTakeClothes();
    if ((target_c < 1U) || (target_c > BOARD2_ACTION_C_COUNT))
    {
        g_board2_action_running = 0U;
        g_board2_action_result = BOARD2_ACTION_RESULT_ERROR;
        g_board2_action_target_c = target_c;
        return;
    }
    s_current_c_index = (uint8_t)(target_c - 1U);
    g_board2_action_target_c = target_c;
}

void Board2_ActionList_Stop(void)
{
    Board2_LinkSetPhotoFillLight(0U);
    g_board2_action_running = 0U;
    if (g_board2_action_result == BOARD2_ACTION_RESULT_RUNNING)
    {
        g_board2_action_result = BOARD2_ACTION_RESULT_STOPPED;
    }
    g_board2_action_current_group = BOARD2_ACTION_GROUP_NONE;
    g_board2_action_current_group_step = 0U;
    clear_runtime_state();
}

uint8_t Board2_ActionList_IsRunning(void)
{
    return g_board2_action_running;
}

uint8_t Board2_ActionList_GetResult(void)
{
    return g_board2_action_result;
}

void Board2_ActionList_Task(void)
{
    uint8_t servo_done;

    if (g_board2_action_running == 0U)
    {
        return;
    }

    if ((s_group_active != 0U) && (group_task() == 0U))
    {
        return;
    }

    if (g_board2_action_running == 0U)
    {
        return;
    }

    servo_done = servo_task();

    if ((motion_done() == 0U) || (wait_done() == 0U) || (servo_done == 0U))
    {
        return;
    }

    if (g_board2_action_running == 0U)
    {
        return;
    }

    if (workflow_has_item() == 0U)
    {
        if ((s_workflow_mode == BOARD2_ACTION_MODE_DEFAULT) &&
            (s_workflow_state != BOARD2_ACTION_WORKFLOW_RETURN_A00) &&
            (s_workflow_state != BOARD2_ACTION_WORKFLOW_END))
        {
            s_workflow_state = BOARD2_ACTION_WORKFLOW_RETURN_A00;
        }
        else if (s_workflow_mode != BOARD2_ACTION_MODE_DEFAULT)
        {
            s_workflow_state = BOARD2_ACTION_WORKFLOW_END;
        }
    }

    switch (s_workflow_state)
    {
        case BOARD2_ACTION_WORKFLOW_HOME_TO_A0:
            move_to_point(BOARD2_ACTION_POINT_A0);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_AFTER_HOME_TO_A0_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TELESCOPE_EXTEND;
            break;

        case BOARD2_ACTION_WORKFLOW_TELESCOPE_EXTEND:
            move_axis_relative(BOARD2_AXIS_TELESCOPE,
                               BOARD2_ACTION_TELESCOPE_EXTEND_DEG,
                               BOARD2_AXIS_TELESCOPE_SPEED_RPM,
                               BOARD2_AXIS_TELESCOPE_ACCEL);
            start_wait(BOARD2_ACTION_WAIT_AFTER_TELESCOPE_EXTEND_MS);
            if (s_workflow_mode == BOARD2_ACTION_MODE_STORE_PREPARE)
            {
                s_workflow_state = BOARD2_ACTION_WORKFLOW_END;
                break;
            }
            s_workflow_state = BOARD2_ACTION_WORKFLOW_HANG_WAIT;
            break;

        case BOARD2_ACTION_WORKFLOW_HANG_WAIT:
            start_wait(BOARD2_ACTION_WAIT_HANG_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TELESCOPE_RETRACT;
            break;

        case BOARD2_ACTION_WORKFLOW_TELESCOPE_RETRACT:
            move_axis_relative(BOARD2_AXIS_TELESCOPE,
                               BOARD2_ACTION_TELESCOPE_RETRACT_DEG,
                               BOARD2_AXIS_TELESCOPE_SPEED_RPM,
                               BOARD2_AXIS_TELESCOPE_ACCEL);
            start_wait(BOARD2_ACTION_WAIT_AFTER_TELESCOPE_RETRACT_MS);
            if (s_workflow_mode == BOARD2_ACTION_MODE_STORE_RETRACT)
            {
                s_workflow_state = BOARD2_ACTION_WORKFLOW_END;
                break;
            }
            s_workflow_state = BOARD2_ACTION_WORKFLOW_Z_TO_A0;
            break;

        case BOARD2_ACTION_WORKFLOW_Z_TO_A0:
            move_z_to_point(BOARD2_ACTION_POINT_A0);
            start_wait(BOARD2_ACTION_WAIT_AFTER_Z_TO_A0_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_OPEN_SERVO2;
            break;

        case BOARD2_ACTION_WORKFLOW_OPEN_SERVO2:
            set_servos_smooth(BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO2_OPEN_DEG);
            start_wait(BOARD2_ACTION_WAIT_AFTER_OPEN_SERVO2_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_MOVE_TO_A;
            break;

        case BOARD2_ACTION_WORKFLOW_MOVE_TO_A:
            if (select_next_default_a_point() == 0U)
            {
                s_workflow_state = BOARD2_ACTION_WORKFLOW_RETURN_A00;
                break;
            }
            move_to_a_point(get_current_a_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_AFTER_MOVE_TO_A_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_KEEP_SERVO1_AT_A;
            break;

        case BOARD2_ACTION_WORKFLOW_KEEP_SERVO1_AT_A:
            if (current_default_a_has_clothes() == 0U)
            {
                if (s_current_a_index < default_a_candidate_count())
                {
                    s_current_a_index++;
                }
                s_workflow_state = BOARD2_ACTION_WORKFLOW_Z_TO_A0;
                break;
            }
            /* 从上一个C点去下一个A点时，舵机1保持当前角度，抓取时不提前翻面。 */
            set_servos_smooth(BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP);
            start_wait(BOARD2_ACTION_WAIT_AFTER_KEEP_SERVO1_AT_A_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_GRAB;
            break;

        case BOARD2_ACTION_WORKFLOW_GRAB:
            start_group(BOARD2_ACTION_GROUP_GRAB);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_MOVE_Y_AFTER_GRAB_1;
            break;

        case BOARD2_ACTION_WORKFLOW_MOVE_Y_AFTER_GRAB_1:
            /* A区夹取后Z已在抓取组内抬到3200，沿Y+离开暂存衣杆后接X轴离开A区。 */
            move_axis_relative(BOARD2_AXIS_Y,
                               BOARD2_ACTION_AFTER_GRAB_Y_OFFSET1_DEG,
                               BOARD2_AXIS_Y_SPEED_RPM,
                               BOARD2_AXIS_Y_ACCEL);
            start_wait(BOARD2_ACTION_WAIT_AFTER_MOVE_Y_AFTER_GRAB1_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_MOVE_X_AFTER_GRAB;
            break;

        case BOARD2_ACTION_WORKFLOW_MOVE_X_AFTER_GRAB:
            move_axis_relative(BOARD2_AXIS_X,
                               BOARD2_ACTION_AFTER_GRAB_X_OFFSET_DEG,
                               BOARD2_AXIS_X_SPEED_RPM,
                               BOARD2_AXIS_X_ACCEL);
            start_wait(BOARD2_ACTION_WAIT_AFTER_MOVE_X_AFTER_GRAB_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_MOVE_TO_B;
            break;

        case BOARD2_ACTION_WORKFLOW_MOVE_TO_B:
            /* 先到B中转导航点，再进入B0识别点，避免从A区直接斜插到识别位。 */
            move_to_point(BOARD2_ACTION_POINT_B);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_AFTER_MOVE_TO_B_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_MOVE_TO_B0;
            break;

        case BOARD2_ACTION_WORKFLOW_MOVE_TO_B0:
            /* B0是正式识别点，到达后才请求K230进行正面识别/拍照。 */
            move_to_point(BOARD2_ACTION_POINT_B0);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            Board2_LinkSetPhotoFillLight(1U);
            start_wait(BOARD2_ACTION_WAIT_AFTER_MOVE_TO_B0_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_CAPTURE_FRONT;
            break;

        case BOARD2_ACTION_WORKFLOW_CAPTURE_FRONT:
            Board2_LinkRequestK230Capture(0U);
            start_wait(BOARD2_ACTION_WAIT_CAPTURE_FRONT_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_Z_DOWN_FOR_BACK;
            break;

        case BOARD2_ACTION_WORKFLOW_Z_DOWN_FOR_BACK:
            move_z_to_b_rotate_range();
            start_wait(BOARD2_ACTION_WAIT_AFTER_Z_DOWN_FOR_BACK_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_ROTATE_BACK;
            break;

        case BOARD2_ACTION_WORKFLOW_ROTATE_BACK:
            /* 此时Z已经降到0，只在这个安全高度把舵机1切到另一面。 */
            s_b_flip_count++;
            set_servos_smooth(s_next_b_servo1_deg, BOARD2_ACTION_SERVO_KEEP);
            s_next_b_servo1_deg = get_other_servo1_angle(s_next_b_servo1_deg);
            start_wait(BOARD2_ACTION_WAIT_AFTER_ROTATE_BACK_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_CAPTURE_BACK;
            break;

        case BOARD2_ACTION_WORKFLOW_CAPTURE_BACK:
            Board2_LinkRequestK230Capture(1U);
            start_wait(BOARD2_ACTION_WAIT_CAPTURE_BACK_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_Z_UP_AFTER_BACK;
            break;

        case BOARD2_ACTION_WORKFLOW_Z_UP_AFTER_BACK:
            Board2_LinkSetPhotoFillLight(0U);
            move_axis_to(BOARD2_AXIS_Z,
                         BOARD2_ACTION_B_SAFE_Z_DEG,
                         BOARD2_AXIS_Z_SPEED_RPM,
                         BOARD2_AXIS_Z_ACCEL);
            start_wait(BOARD2_ACTION_WAIT_AFTER_Z_UP_AFTER_BACK_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_MOVE_TO_C_X_PRE;
            break;

        case BOARD2_ACTION_WORKFLOW_MOVE_TO_C_X_PRE:
            move_to_c_x_pre_point(get_current_c_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_AFTER_MOVE_TO_C_X_PRE_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_MOVE_TO_C_PRE;
            break;

        case BOARD2_ACTION_WORKFLOW_MOVE_TO_C_PRE:
            move_to_c_pre_point(get_current_c_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_AFTER_MOVE_TO_C_PRE_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_MOVE_TO_C;
            break;

        case BOARD2_ACTION_WORKFLOW_MOVE_TO_C:
            move_to_c_point(get_current_c_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_AFTER_MOVE_TO_C_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_PLACE;
            break;

        case BOARD2_ACTION_WORKFLOW_PLACE:
            start_group(BOARD2_ACTION_GROUP_PLACE);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_NEXT_ITEM;
            break;

        case BOARD2_ACTION_WORKFLOW_NEXT_ITEM:
            if (s_single_store_item != 0U)
            {
                s_workflow_state = BOARD2_ACTION_WORKFLOW_RETURN_A00;
                break;
            }
            if (s_current_a_index < BOARD2_ACTION_A_COUNT)
            {
                s_current_a_index++;
            }
            advance_next_c_index_after_place();
            s_workflow_state = BOARD2_ACTION_WORKFLOW_Z_TO_A0;
            break;

        case BOARD2_ACTION_WORKFLOW_RETURN_A00:
            move_to_point(BOARD2_ACTION_POINT_A00);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_AFTER_RETURN_A00_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_END;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_HOME_TO_A0:
            /* 数字7取衣流程：先回A0，保证从统一安全高度出发。 */
            move_to_point(BOARD2_ACTION_POINT_A0);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_HOME_TO_A0_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01_X_PRE;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01_X_PRE:
            /* 取衣进入C01也走第一个前置点：X负向偏置、Y负向偏置。 */
            move_to_c_x_pre_point(get_current_c_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_C01_X_PRE_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01_PRE;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01_PRE:
            /* 第二个前置点：X回到C01，Y仍保持负向偏置。 */
            move_to_c_pre_point(get_current_c_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_C01_PRE_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01:
            /* 默认衣服挂在C01，经过两个前置点后再进真实C01取衣。 */
            move_to_c_point(get_current_c_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_C01_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_GRAB_FROM_C;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_GRAB_FROM_C:
            start_group(BOARD2_ACTION_GROUP_C_GRAB);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_EXIT_C01_PRE;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_EXIT_C01_PRE:
            /* C01取出后按存放进入C点的逆过程，先退回第二个前置点。 */
            move_to_c_pre_point(get_current_c_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_EXIT_C01_PRE_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_EXIT_C01_X_PRE;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_EXIT_C01_X_PRE:
            /* 再退回第一个前置点，然后离开C区去B3。 */
            move_to_c_x_pre_point(get_current_c_point());
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_EXIT_C01_X_PRE_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_B3;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_B3:
            /* C01取到衣服并退到C区第一个前置点后，再到B3刷毛点。 */
            move_to_point(BOARD2_ACTION_POINT_B3);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_B3_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_Z_DOWN_AT_B3;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_Z_DOWN_AT_B3:
            /* 到B3后降到Z=0低位，准备第一面刷毛。 */
            move_z_to_take_brush_height();
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_Z_DOWN_TO_BRUSH_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_BRUSH_FRONT_AT_B3;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_BRUSH_FRONT_AT_B3:
            Board2_RunBrushCleanMotion();
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_B4;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_B4:
            /* 刷完第一面后保持Z=0低位，只移动XY到B4翻面点。 */
            move_xy_to_point_keep_z(BOARD2_ACTION_POINT_B4);
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_B4_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_ROTATE_SERVO1_AT_B4;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_ROTATE_SERVO1_AT_B4:
            /* 在B4低位切换舵机1方向，再回B3刷另一面。 */
            s_b_flip_count++;
            set_servos_smooth(get_other_servo1_angle(g_board2_servo_ch1_angle_deg),
                              BOARD2_ACTION_SERVO_KEEP);
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_ROTATE_SERVO1_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_MOVE_BACK_TO_B3;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_MOVE_BACK_TO_B3:
            /* 翻面后保持Z=0低位，从B4回到B3。 */
            move_xy_to_point_keep_z(BOARD2_ACTION_POINT_B3);
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_BACK_TO_B3_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_BRUSH_BACK_AT_B3;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_BRUSH_BACK_AT_B3:
            Board2_RunBrushCleanMotion();
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_Z_UP_AFTER_BRUSH;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_Z_UP_AFTER_BRUSH:
            move_z_up_after_take_brush();
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_Z_UP_AFTER_BRUSH_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_A01;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_A01:
            /* 取下衣服后送到A01，A区放置也要走单独的A区放置流程。 */
            move_to_a_point(BOARD2_ACTION_POINT_A01);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_A01_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_PLACE_TO_A;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_PLACE_TO_A:
            start_group(BOARD2_ACTION_GROUP_A_PLACE);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_RETURN_A0;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_RETURN_A0:
            move_to_point(BOARD2_ACTION_POINT_A0);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_RETURN_A0_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_TAKE_TELESCOPE_EXTEND;
            break;

        case BOARD2_ACTION_WORKFLOW_TAKE_TELESCOPE_EXTEND:
            move_axis_relative(BOARD2_AXIS_TELESCOPE,
                               BOARD2_ACTION_TELESCOPE_EXTEND_DEG,
                               BOARD2_AXIS_TELESCOPE_SPEED_RPM,
                               BOARD2_AXIS_TELESCOPE_ACCEL);
            start_wait(BOARD2_ACTION_WAIT_TAKE_AFTER_TELESCOPE_EXTEND_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_END;
            break;

        case BOARD2_ACTION_WORKFLOW_FETCH_RESET_TELESCOPE_HOME:
            /* 按伸缩轴软件累计位置绝对回零，重复执行不会再次固定缩回一个完整行程。 */
            move_axis_to(BOARD2_AXIS_TELESCOPE,
                         0.0f,
                         BOARD2_AXIS_TELESCOPE_SPEED_RPM,
                         BOARD2_AXIS_TELESCOPE_ACCEL);
            start_wait(BOARD2_ACTION_WAIT_FETCH_RESET_AFTER_TELESCOPE_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_FETCH_RESET_HOME_TO_A0;
            break;

        case BOARD2_ACTION_WORKFLOW_FETCH_RESET_HOME_TO_A0:
            move_to_point(BOARD2_ACTION_POINT_A0);
            if (g_board2_action_running == 0U)
            {
                return;
            }
            start_wait(BOARD2_ACTION_WAIT_FETCH_RESET_AFTER_HOME_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_FETCH_RESET_SERVOS;
            break;

        case BOARD2_ACTION_WORKFLOW_FETCH_RESET_SERVOS:
            set_servos_smooth(TIM1_SERVO1_INIT_ANGLE_DEG,
                              TIM1_SERVO2_INIT_ANGLE_DEG);
            g_board2_servo_ch3_angle_deg = TIM1_SERVO3_INIT_ANGLE_DEG;
            TIM1_Servo_SetAngle(TIM1_SERVO_CH3, TIM1_SERVO3_INIT_ANGLE_DEG);
            g_board2_servo_ch4_angle_deg = TIM1_SERVO4_INIT_ANGLE_DEG;
            TIM1_Servo_SetAngle(TIM1_SERVO_CH4, TIM1_SERVO4_INIT_ANGLE_DEG);
            start_wait(BOARD2_ACTION_WAIT_FETCH_RESET_AFTER_SERVOS_MS);
            s_workflow_state = BOARD2_ACTION_WORKFLOW_END;
            break;

        case BOARD2_ACTION_WORKFLOW_END:
        default:
            g_board2_action_result = BOARD2_ACTION_RESULT_DONE;
            Board2_ActionList_Stop();
            return;
    }

    g_board2_action_current_step = s_workflow_state;
}
