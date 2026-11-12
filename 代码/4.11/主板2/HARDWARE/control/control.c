#include "control.h"
#include "stm32f4xx.h"
#include "usartdma.h"
#include "stepper_multi.h"
#include "key.h"
#include "board2_action_list.h"

volatile uint32_t g_board2_tick_ms = 0;
volatile float g_board2_x_accum_deg = 0.0f;
volatile float g_board2_y_accum_deg = 0.0f;
volatile float g_board2_z_accum_deg = 0.0f;
volatile float g_board2_uart5_motor1_accum_deg = 0.0f;
volatile float g_board2_telescope_accum_deg = 0.0f;
volatile uint32_t g_board2_x_send_count = 0U;
volatile uint32_t g_board2_y_send_count = 0U;
volatile uint32_t g_board2_z_send_count = 0U;
volatile uint32_t g_board2_uart5_motor1_send_count = 0U;
volatile uint32_t g_board2_telescope_send_count = 0U;
volatile uint8_t g_board2_last_key_event = 0;
volatile uint8_t g_board2_pending_key_event = 0;
volatile uint8_t g_board2_adc_io_active_mask = 0U;
volatile uint8_t g_board2_adc_io_last_active_mask = 0U;
volatile uint32_t g_board2_adc_io_change_count = 0U;

static uint8_t s_key_state[4] = {0, 0, 0, 0};
static uint8_t s_key_debounce[4] = {0, 0, 0, 0};
#if (BOARD2_KEY_REPEAT_ENABLE != 0U)
static uint16_t s_key_repeat_ticks[4] = {0, 0, 0, 0};
#endif

static uint8_t board2_get_raw_key(uint8_t key_index)
{
    switch (key_index)
    {
    case 0: return (KEY1 == 0U) ? 1U : 0U;
    case 1: return (KEY2 == 0U) ? 1U : 0U;
    case 2: return (KEY3 == 0U) ? 1U : 0U;
    case 3: return (KEY4 == 0U) ? 1U : 0U;
    default: return 0U;
    }
}

void Board2_AdcIo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

uint8_t Board2_AdcIo_Read(uint8_t index)
{
    switch (index)
    {
    case 1U:
        return (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == Bit_RESET) ? 1U : 0U;
    case 2U:
        return (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == Bit_RESET) ? 1U : 0U;
    case 3U:
        return (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == Bit_RESET) ? 1U : 0U;
    case 4U:
        return (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == Bit_RESET) ? 1U : 0U;
    default:
        return 0U;
    }
}

uint8_t Board2_AdcIo_GetActiveMask(void)
{
    uint8_t mask = 0U;

    if (Board2_AdcIo_Read(1U) != 0U)
    {
        mask |= BOARD2_ADC_IO1_MASK;
    }
    if (Board2_AdcIo_Read(2U) != 0U)
    {
        mask |= BOARD2_ADC_IO2_MASK;
    }
    if (Board2_AdcIo_Read(3U) != 0U)
    {
        mask |= BOARD2_ADC_IO3_MASK;
    }
    if (Board2_AdcIo_Read(4U) != 0U)
    {
        mask |= BOARD2_ADC_IO4_MASK;
    }

    return mask;
}

void Board2_AdcIo_Update(void)
{
    uint8_t mask = Board2_AdcIo_GetActiveMask();

    if (mask != g_board2_adc_io_active_mask)
    {
        g_board2_adc_io_last_active_mask = g_board2_adc_io_active_mask;
        g_board2_adc_io_active_mask = mask;
        g_board2_adc_io_change_count++;
    }
}

static uint8_t board2_scan_key_event(void)
{
    uint8_t i;
    uint8_t raw;
    uint8_t event = 0U;

    for (i = 0U; i < 4U; i++)
    {
        raw = board2_get_raw_key(i);

        if (raw == s_key_state[i])
        {
            s_key_debounce[i] = 0U;
        }
        else
        {
            if (s_key_debounce[i] < 0xFFU)
            {
                s_key_debounce[i]++;
            }
            if (s_key_debounce[i] >= BOARD2_KEY_DEBOUNCE_COUNT)
            {
                s_key_debounce[i] = 0U;
                s_key_state[i] = raw;
                if ((raw != 0U) && (event == 0U))
                {
                    event = (uint8_t)(i + 1U);
                }
            }
        }

#if (BOARD2_KEY_REPEAT_ENABLE != 0U)
        if (s_key_state[i] != 0U)
        {
            if (s_key_repeat_ticks[i] < 0xFFFFU)
            {
                s_key_repeat_ticks[i] = (uint16_t)(s_key_repeat_ticks[i] + BOARD2_KEY_SCAN_PERIOD_MS);
            }
            if ((s_key_repeat_ticks[i] >= BOARD2_KEY_REPEAT_PERIOD_MS) && (event == 0U))
            {
                s_key_repeat_ticks[i] = 0U;
                event = (uint8_t)(i + 1U);
            }
        }
        else
        {
            s_key_repeat_ticks[i] = 0U;
        }
#endif
    }

    return event;
}

static float board2_signed_angle_by_channel(StepperChannel_t ch, uint8_t dir, float angle)
{
    switch (ch)
    {
    case STEPPER_CH1_UART2:
        return (dir == BOARD2_AXIS_X_POS_DIR) ? angle : -angle;
    case STEPPER_CH2_UART3:
        return (dir == BOARD2_AXIS_Y_POS_DIR) ? angle : -angle;
    case STEPPER_CH3_UART4:
        return (dir == BOARD2_AXIS_Z_POS_DIR) ? angle : -angle;
    case STEPPER_CH4_UART5:
        return (dir == BOARD2_AXIS_TELESCOPE_POS_DIR) ? angle : -angle;
    default:
        return angle;
    }
}

static uint8_t board2_wait_axis_reached(StepperChannel_t ch)
{
#if (BOARD2_AXIS_CLOSED_LOOP_ENABLE != 0U)
    if (ch == STEPPER_CH1_UART2)
    {
        return (stepper_multi_wait_reached_dual_opposite_uart2(BOARD2_AXIS_X_ADDR_ID1,
                                                               BOARD2_AXIS_X_ADDR_ID2,
                                                               BOARD2_AXIS_CLOSED_LOOP_TIMEOUT_MS) != 0U) ? 1U : 0U;
    }

    if (ch == STEPPER_CH4_UART5)
    {
        return 1U;
    }

    return (stepper_multi_wait_reached(ch, 1U, BOARD2_AXIS_CLOSED_LOOP_TIMEOUT_MS) != 0U) ? 1U : 0U;
#else
    (void)ch;
    return 1U;
#endif
}

static uint8_t board2_move_axis(StepperChannel_t ch,
                                uint8_t addr,
                                uint8_t dir,
                                uint16_t speed,
                                uint8_t acc,
                                float angle,
                                volatile float *accum_deg)
{
    if (angle <= 0.0f)
    {
        return 1U;
    }

#if (BOARD2_AXIS_ENABLE_BEFORE_MOVE != 0U)
    stepper_multi_enable(ch, addr, true);
#endif

    stepper_multi_move_angle(ch, addr, dir, speed, acc, angle);

    if (board2_wait_axis_reached(ch) == 0U)
    {
        return 0U;
    }

    if (accum_deg != 0)
    {
        *accum_deg += board2_signed_angle_by_channel(ch, dir, angle);
    }

    return 1U;
}

static uint8_t board2_move_x_dual_opposite(uint8_t dir,
                                           uint16_t speed,
                                           uint8_t acc,
                                           float angle,
                                           volatile float *accum_deg)
{
    if (angle <= 0.0f)
    {
        return 1U;
    }

#if (BOARD2_AXIS_ENABLE_BEFORE_MOVE != 0U)
    stepper_multi_enable_dual_opposite_uart2(BOARD2_AXIS_X_ADDR_ID1,
                                             BOARD2_AXIS_X_ADDR_ID2,
                                             true);
#endif

    stepper_multi_move_angle_dual_opposite_uart2(BOARD2_AXIS_X_ADDR_ID1,
                                                 BOARD2_AXIS_X_ADDR_ID2,
                                                 dir,
                                                 speed,
                                                 acc,
                                                 angle);

    if (board2_wait_axis_reached(STEPPER_CH1_UART2) == 0U)
    {
        return 0U;
    }

    if (accum_deg != 0)
    {
        *accum_deg += (dir == BOARD2_AXIS_X_KEY_DIR) ? angle : -angle;
    }

    return 1U;
}

static void board2_home_all_axes(void)
{
    float x_home_angle;
    float y_home_angle;
    float z_home_angle;
    float uart5_motor1_home_angle;
    float telescope_home_angle;
    uint8_t x_home_ok;
    uint8_t y_home_ok;
    uint8_t z_home_ok;
    uint8_t uart5_motor1_home_ok;
    uint8_t telescope_home_ok;

#if (BOARD2_HOME_USE_ACCUMULATED_ANGLE != 0U)
    x_home_angle = (g_board2_x_accum_deg >= 0.0f) ? g_board2_x_accum_deg : -g_board2_x_accum_deg;
    y_home_angle = (g_board2_y_accum_deg >= 0.0f) ? g_board2_y_accum_deg : -g_board2_y_accum_deg;
    z_home_angle = (g_board2_z_accum_deg >= 0.0f) ? g_board2_z_accum_deg : -g_board2_z_accum_deg;
    uart5_motor1_home_angle = (g_board2_uart5_motor1_accum_deg >= 0.0f) ? g_board2_uart5_motor1_accum_deg : -g_board2_uart5_motor1_accum_deg;
    telescope_home_angle = (g_board2_telescope_accum_deg >= 0.0f) ? g_board2_telescope_accum_deg : -g_board2_telescope_accum_deg;
#else
    x_home_angle = BOARD2_HOME_FIXED_X_ANGLE_DEG;
    y_home_angle = BOARD2_HOME_FIXED_Y_ANGLE_DEG;
    z_home_angle = BOARD2_HOME_FIXED_Z_ANGLE_DEG;
    uart5_motor1_home_angle = BOARD2_HOME_FIXED_UART5_MOTOR1_ANGLE_DEG;
    telescope_home_angle = BOARD2_HOME_FIXED_TELESCOPE_ANGLE_DEG;
#endif

    x_home_ok = board2_move_x_dual_opposite((g_board2_x_accum_deg >= 0.0f) ? BOARD2_AXIS_X_NEG_DIR : BOARD2_AXIS_X_POS_DIR,
                                            BOARD2_HOME_SPEED_RPM,
                                            BOARD2_ACCEL_U8(BOARD2_HOME_ACCEL),
                                            x_home_angle,
                                            0);

    y_home_ok = board2_move_axis(BOARD2_AXIS_Y_CHANNEL,
                                 BOARD2_AXIS_Y_ADDR,
                                 (g_board2_y_accum_deg >= 0.0f) ? BOARD2_AXIS_Y_NEG_DIR : BOARD2_AXIS_Y_POS_DIR,
                                 BOARD2_HOME_SPEED_RPM,
                                 BOARD2_ACCEL_U8(BOARD2_HOME_ACCEL),
                                 y_home_angle,
                                 0);

    z_home_ok = board2_move_axis(BOARD2_AXIS_Z_CHANNEL,
                                 BOARD2_AXIS_Z_ADDR,
                                 (g_board2_z_accum_deg >= 0.0f) ? BOARD2_AXIS_Z_NEG_DIR : BOARD2_AXIS_Z_POS_DIR,
                                 BOARD2_HOME_SPEED_RPM,
                                 BOARD2_ACCEL_U8(BOARD2_HOME_ACCEL),
                                 z_home_angle,
                                 0);

    uart5_motor1_home_ok = board2_move_axis(BOARD2_AXIS_UART5_MOTOR1_CHANNEL,
                                           BOARD2_AXIS_UART5_MOTOR1_ADDR,
                                           (g_board2_uart5_motor1_accum_deg >= 0.0f) ? BOARD2_AXIS_UART5_MOTOR1_NEG_DIR : BOARD2_AXIS_UART5_MOTOR1_POS_DIR,
                                           BOARD2_HOME_SPEED_RPM,
                                           BOARD2_ACCEL_U8(BOARD2_HOME_ACCEL),
                                           uart5_motor1_home_angle,
                                           0);

    telescope_home_ok = board2_move_axis(BOARD2_AXIS_TELESCOPE_CHANNEL,
                                         BOARD2_AXIS_TELESCOPE_ADDR,
                                         (g_board2_telescope_accum_deg >= 0.0f) ? BOARD2_AXIS_TELESCOPE_NEG_DIR : BOARD2_AXIS_TELESCOPE_POS_DIR,
                                         BOARD2_HOME_SPEED_RPM,
                                         BOARD2_ACCEL_U8(BOARD2_HOME_ACCEL),
                                         telescope_home_angle,
                                         0);

    if (x_home_ok != 0U)
    {
        g_board2_x_accum_deg = 0.0f;
        g_board2_x_send_count = 0U;
    }
    if (y_home_ok != 0U)
    {
        g_board2_y_accum_deg = 0.0f;
        g_board2_y_send_count = 0U;
    }
    if (z_home_ok != 0U)
    {
        g_board2_z_accum_deg = 0.0f;
        g_board2_z_send_count = 0U;
    }
    if (uart5_motor1_home_ok != 0U)
    {
        g_board2_uart5_motor1_accum_deg = 0.0f;
        g_board2_uart5_motor1_send_count = 0U;
    }
    if (telescope_home_ok != 0U)
    {
        g_board2_telescope_accum_deg = 0.0f;
        g_board2_telescope_send_count = 0U;
    }
}

static void board2_handle_key_event(uint8_t key_event)
{
    switch (key_event)
    {
    case KEY1_PRES:
        if (board2_move_x_dual_opposite(BOARD2_AXIS_X_KEY_DIR,
                                        BOARD2_AXIS_X_SPEED_RPM,
                                        BOARD2_ACCEL_U8(BOARD2_AXIS_X_ACCEL),
                                        BOARD2_AXIS_X_STEP_ANGLE_DEG,
                                        &g_board2_x_accum_deg) != 0U)
        {
            g_board2_x_send_count++;
        }
        break;

    case KEY2_PRES:
        if (board2_move_axis(BOARD2_AXIS_Y_CHANNEL,
                             BOARD2_AXIS_Y_ADDR,
                             BOARD2_AXIS_Y_KEY_DIR,
                             BOARD2_AXIS_Y_SPEED_RPM,
                             BOARD2_ACCEL_U8(BOARD2_AXIS_Y_ACCEL),
                             BOARD2_AXIS_Y_STEP_ANGLE_DEG,
                             &g_board2_y_accum_deg) != 0U)
        {
            g_board2_y_send_count++;
        }
        break;

    case KEY3_PRES:
        if (board2_move_axis(BOARD2_AXIS_Z_CHANNEL,
                             BOARD2_AXIS_Z_ADDR,
                             BOARD2_AXIS_Z_KEY_DIR,
                             BOARD2_AXIS_Z_SPEED_RPM,
                             BOARD2_ACCEL_U8(BOARD2_AXIS_Z_ACCEL),
                             BOARD2_AXIS_Z_STEP_ANGLE_DEG,
                             &g_board2_z_accum_deg) != 0U)
        {
            g_board2_z_send_count++;
        }
        break;

    case KEY4_PRES:
        board2_home_all_axes();
        break;

    default:
        break;
    }
}

void Control_Init(void)
{
    uint8_t i;

    g_board2_tick_ms = 0U;
    g_board2_x_accum_deg = 0.0f;
    g_board2_y_accum_deg = 0.0f;
    g_board2_z_accum_deg = 0.0f;
    g_board2_uart5_motor1_accum_deg = 0.0f;
    g_board2_telescope_accum_deg = 0.0f;
    g_board2_x_send_count = 0U;
    g_board2_y_send_count = 0U;
    g_board2_z_send_count = 0U;
    g_board2_uart5_motor1_send_count = 0U;
    g_board2_telescope_send_count = 0U;
    g_board2_last_key_event = 0U;
    g_board2_pending_key_event = 0U;
    g_board2_adc_io_active_mask = 0U;
    g_board2_adc_io_last_active_mask = 0U;
    g_board2_adc_io_change_count = 0U;

    for (i = 0U; i < 4U; i++)
    {
        s_key_state[i] = 0U;
        s_key_debounce[i] = 0U;
#if (BOARD2_KEY_REPEAT_ENABLE != 0U)
        s_key_repeat_ticks[i] = 0U;
#endif
    }

    KEY_Init();
    Board2_AdcIo_Init();
    Board2_AdcIo_Update();
}

void Board2_MoveTelescopeStep(void)
{
    if (board2_move_axis(BOARD2_AXIS_TELESCOPE_CHANNEL,
                         BOARD2_AXIS_TELESCOPE_ADDR,
                         BOARD2_AXIS_TELESCOPE_KEY_DIR,
                         BOARD2_AXIS_TELESCOPE_SPEED_RPM,
                         BOARD2_ACCEL_U8(BOARD2_AXIS_TELESCOPE_ACCEL),
                         BOARD2_AXIS_TELESCOPE_STEP_ANGLE_DEG,
                         &g_board2_telescope_accum_deg) != 0U)
    {
        g_board2_telescope_send_count++;
    }
}

void Board2_RunPendingKeyEvent(void)
{
    uint8_t key_event;

    if (g_board2_pending_key_event == 0U)
    {
        return;
    }

    __disable_irq();
    key_event = g_board2_pending_key_event;
    g_board2_pending_key_event = 0U;
    __enable_irq();

    if (key_event != 0U)
    {
        if (Board2_ActionList_IsRunning() != 0U)
        {
            return;
        }

        board2_handle_key_event(key_event);
    }
}

void TIM7_IRQHandler(void)
{
    static uint16_t key_scan_period_cnt = 0U;
    uint8_t key_event;

    if (TIM_GetITStatus(TIM7, TIM_IT_Update) == SET)
    {
        g_board2_tick_ms += BOARD2_TICK_PERIOD_MS;

        key_scan_period_cnt = (uint16_t)(key_scan_period_cnt + BOARD2_TICK_PERIOD_MS);

        if (key_scan_period_cnt >= BOARD2_KEY_SCAN_PERIOD_MS)
        {
            key_scan_period_cnt = 0U;
            Board2_AdcIo_Update();
            key_event = board2_scan_key_event();
            if (key_event != 0U)
            {
                g_board2_last_key_event = key_event;
                if (g_board2_pending_key_event == 0U)
                {
                    g_board2_pending_key_event = key_event;
                }
            }
        }

        TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
    }
}
