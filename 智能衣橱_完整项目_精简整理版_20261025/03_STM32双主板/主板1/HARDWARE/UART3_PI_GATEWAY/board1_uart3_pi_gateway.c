#include "board1_uart3_pi_gateway.h"
#include "board1_uart6_screen.h"
#include "timer.h"
#include "usartdma.h"
#include "usart4dma.h"
#include "delay.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

volatile uint8_t g_board1_pi_gateway_last_status = BOARD1_PI_GATEWAY_STATUS_IDLE;
volatile uint8_t g_board1_pi_gateway_last_style_id = BOARD1_PI_GATEWAY_STYLE_UNKNOWN;
volatile uint8_t g_board1_pi_gateway_response_ready = 0U;
volatile uint8_t g_board1_pi_gateway_busy = 0U;
volatile uint8_t g_board1_pi_gateway_bad_frame_count = 0U;
volatile uint32_t g_board1_pi_gateway_image_size = 0U;
volatile uint32_t g_board1_pi_gateway_image_received = 0U;
volatile uint8_t g_board1_pi_gateway_image_info_ready = 0U;
volatile uint8_t g_board1_pi_gateway_image_done = 0U;
volatile uint8_t g_board1_pi_gateway_image_error = 0U;
volatile uint8_t g_board1_pi_gateway_wifi_scan_ready = 0U;
volatile uint8_t g_board1_pi_gateway_wifi_status_update = 0U;
volatile uint8_t g_board1_pi_gateway_wifi_status = BOARD1_PI_GATEWAY_WIFI_STATUS_IDLE;
volatile uint8_t g_board1_pi_gateway_wifi_busy = 0U;
volatile uint32_t g_board1_pi_gateway_ascii_cmd_count = 0U;
volatile uint32_t g_board1_pi_gateway_motion_cmd_count = 0U;
volatile uint32_t g_board1_pi_gateway_servo_cmd_count = 0U;
volatile uint32_t g_board1_pi_gateway_action_cmd_count = 0U;
volatile uint32_t g_board1_pi_gateway_output_cmd_count = 0U;
volatile uint8_t g_board1_pi_gateway_last_ascii_cmd = 0U;
volatile uint8_t g_board1_pi_gateway_last_motion_cmd = 0U;
volatile uint8_t g_board1_pi_gateway_last_motion_channel = 0U;
volatile uint16_t g_board1_pi_gateway_last_motion_angle = 0U;
volatile uint8_t g_board1_pi_gateway_last_servo_channel = 0U;
volatile uint16_t g_board1_pi_gateway_last_servo_angle = 0U;
volatile uint8_t g_board1_pi_gateway_last_output_device = 0U;
volatile uint8_t g_board1_pi_gateway_last_output_enable = 0U;
volatile uint8_t g_board1_pi_gateway_debug_dirty = 0U;
char g_board1_pi_gateway_last_command_id[BOARD1_PI_GATEWAY_COMMAND_ID_MAX_LEN];

static volatile uint8_t g_pi_gateway_rx_buf[BOARD1_PI_GATEWAY_RX_BUF_LEN];
static volatile uint8_t g_pi_gateway_rx_len = 0U;
static volatile uint32_t g_pi_gateway_request_tick_ms = 0U;
static volatile uint8_t g_pi_gateway_image_streaming = 0U;
static char g_pi_gateway_ascii_rx_buf[BOARD1_PI_GATEWAY_ASCII_RX_BUF_LEN];
static volatile uint16_t g_pi_gateway_ascii_rx_len = 0U;
static board1_pi_gateway_wifi_ap_t g_pi_gateway_wifi_aps[BOARD1_PI_GATEWAY_WIFI_MAX_APS];
static volatile uint8_t g_pi_gateway_wifi_count = 0U;
static char g_pi_gateway_wifi_info[BOARD1_PI_GATEWAY_WIFI_INFO_MAX_LEN];
static volatile uint32_t g_pi_gateway_wifi_request_tick_ms = 0U;
static volatile uint8_t g_pi_gateway_local_action_pending = 0U;
static volatile uint8_t g_pi_gateway_fan_move_task_state = 0U;
static volatile uint32_t g_pi_gateway_fan_move_task_tick_ms = 0U;
static char g_pi_gateway_fan_task_command_id[BOARD1_PI_GATEWAY_COMMAND_ID_MAX_LEN];
static volatile uint8_t g_pi_gateway_board2_active_action_id = 0U;
static volatile uint8_t g_pi_gateway_board2_active_target_c = 0U;
static char g_pi_gateway_board2_active_command_id[BOARD1_PI_GATEWAY_COMMAND_ID_MAX_LEN];

#define BOARD1_PI_GATEWAY_LOCAL_MOTION_QUEUE_LEN 8U
#define BOARD1_PI_GATEWAY_FAN_MOVE_TASK_IDLE     0U
#define BOARD1_PI_GATEWAY_FAN_MOVE_TASK_OUT      1U
#define BOARD1_PI_GATEWAY_FAN_MOVE_TASK_HOME     2U

#define BOARD1_PI_GATEWAY_OUTPUT_UV_LAMP         1U
#define BOARD1_PI_GATEWAY_OUTPUT_VENT_FAN        2U

#define BOARD1_PI_GATEWAY_INT64_MAX_VALUE 9223372036854775807LL
#define BOARD1_PI_GATEWAY_INT64_MIN_VALUE (-9223372036854775807LL - 1LL)
#define BOARD1_PI_GATEWAY_INT64_MIN_ABS   9223372036854775808ULL

typedef struct
{
    uint8_t cmd;
    uint8_t control;
    uint64_t angle_deg;
    int64_t target_deg;
} board1_pi_gateway_local_motion_t;

static volatile board1_pi_gateway_local_motion_t g_pi_gateway_local_motion_queue[BOARD1_PI_GATEWAY_LOCAL_MOTION_QUEUE_LEN];
static volatile uint8_t g_pi_gateway_local_motion_q_head = 0U;
static volatile uint8_t g_pi_gateway_local_motion_q_tail = 0U;
static volatile uint8_t g_pi_gateway_local_motion_q_count = 0U;

static const char *board1_pi_gateway_task_type_from_action(uint8_t action_id)
{
    switch (action_id)
    {
        case 6U: return "store";
        case 11U: return "store";
        case 12U: return "store";
        case 13U: return "store";
        case 7U: return "fetch";
        case 14U: return "fetch-reset";
        case BOARD1_PI_GATEWAY_ACTION_FAN_MOVE: return "hot-air-start";
        case BOARD1_PI_GATEWAY_ACTION_FAN_STOP: return "hot-air-stop";
        case 10U: return "brush";
        case 0U: return "emergency";
        default: return "task";
    }
}

static void board1_pi_gateway_copy_command_id(char *dst, uint8_t dst_len, const char *src)
{
    uint8_t i;

    if ((dst == 0) || (dst_len == 0U))
    {
        return;
    }

    if (src == 0)
    {
        src = "";
    }

    for (i = 0U; (i + 1U) < dst_len; i++)
    {
        dst[i] = src[i];
        if (src[i] == '\0')
        {
            return;
        }
    }
    dst[dst_len - 1U] = '\0';
}

static void board1_pi_gateway_send_task_status(uint8_t action_id,
                                               const char *command_id,
                                               const char *state,
                                               uint8_t target_c)
{
    char line[192];

    if (state == 0)
    {
        state = "";
    }

    if ((target_c >= 1U) && (target_c <= 12U))
    {
        snprintf(line,
                 sizeof(line),
                 "@TASK|type=%s|action=%u|id=%s|state=%s|target_c=C%02u|detail=C%02u",
                 board1_pi_gateway_task_type_from_action(action_id),
                 (unsigned int)action_id,
                 (command_id != 0) ? command_id : "",
                 state,
                 (unsigned int)target_c,
                 (unsigned int)target_c);
    }
    else
    {
        snprintf(line,
                 sizeof(line),
                 "@TASK|type=%s|action=%u|id=%s|state=%s",
                 board1_pi_gateway_task_type_from_action(action_id),
                 (unsigned int)action_id,
                 (command_id != 0) ? command_id : "",
                 state);
    }
    board1_uart3_pi_gateway_send_line(line);
}

static const char *board1_pi_gateway_state_from_board2_status(uint8_t status_code)
{
    switch (status_code)
    {
        case BOARD_ACTION_STATUS_RUNNING: return "running";
        case BOARD_ACTION_STATUS_DONE: return "done";
        case BOARD_ACTION_STATUS_STOPPED: return "stopped";
        case BOARD_ACTION_STATUS_BUSY: return "error";
        case BOARD_ACTION_STATUS_ERROR: return "error";
        default: return "error";
    }
}

static void board1_pi_gateway_set_board2_active_task(uint8_t action_id,
                                                      const char *command_id,
                                                      uint8_t target_c)
{
    g_pi_gateway_board2_active_action_id = action_id;
    g_pi_gateway_board2_active_target_c = target_c;
    board1_pi_gateway_copy_command_id(g_pi_gateway_board2_active_command_id,
                                      (uint8_t)sizeof(g_pi_gateway_board2_active_command_id),
                                      command_id);
}

void board1_uart3_pi_gateway_handle_board2_status(uint8_t action_id,
                                                  uint8_t status_code,
                                                  uint16_t detail)
{
    uint8_t report_action_id = action_id;
    uint8_t report_target_c = (uint8_t)detail;
    const char *state = board1_pi_gateway_state_from_board2_status(status_code);
    const char *command_id = g_pi_gateway_board2_active_command_id;

    if (report_action_id == 0U)
    {
        report_action_id = g_pi_gateway_board2_active_action_id;
    }

    if ((report_target_c < 1U) || (report_target_c > 12U))
    {
        report_target_c = g_pi_gateway_board2_active_target_c;
    }

    if (command_id[0] == '\0')
    {
        command_id = g_board1_pi_gateway_last_command_id;
    }

    board1_pi_gateway_send_task_status(report_action_id, command_id, state, report_target_c);

    if ((status_code == BOARD_ACTION_STATUS_DONE) ||
        (status_code == BOARD_ACTION_STATUS_STOPPED) ||
        (status_code == BOARD_ACTION_STATUS_ERROR) ||
        (status_code == BOARD_ACTION_STATUS_BUSY))
    {
        g_pi_gateway_board2_active_action_id = 0U;
        g_pi_gateway_board2_active_target_c = 0U;
        g_pi_gateway_board2_active_command_id[0] = '\0';
    }
}

static int64_t board1_pi_gateway_parse_i64(const char *text)
{
    uint8_t negative = 0U;
    uint8_t has_digit = 0U;
    uint64_t value = 0ULL;
    uint64_t limit;

    if (text == 0)
    {
        return 0;
    }

    while ((*text == ' ') || (*text == '\t'))
    {
        text++;
    }

    if ((*text == '+') || (*text == '-'))
    {
        negative = (*text == '-') ? 1U : 0U;
        text++;
    }

    limit = (negative != 0U) ? BOARD1_PI_GATEWAY_INT64_MIN_ABS : (uint64_t)BOARD1_PI_GATEWAY_INT64_MAX_VALUE;
    while ((*text >= '0') && (*text <= '9'))
    {
        uint8_t digit = (uint8_t)(*text - '0');

        has_digit = 1U;
        if (value > ((limit - (uint64_t)digit) / 10ULL))
        {
            return (negative != 0U) ? BOARD1_PI_GATEWAY_INT64_MIN_VALUE : BOARD1_PI_GATEWAY_INT64_MAX_VALUE;
        }

        value = (value * 10ULL) + (uint64_t)digit;
        text++;
    }

    if (has_digit == 0U)
    {
        return 0;
    }

    if (negative != 0U)
    {
        if (value == BOARD1_PI_GATEWAY_INT64_MIN_ABS)
        {
            return BOARD1_PI_GATEWAY_INT64_MIN_VALUE;
        }
        return -(int64_t)value;
    }

    return (int64_t)value;
}

static uint64_t board1_pi_gateway_abs_i64(int64_t value)
{
    if (value >= 0)
    {
        return (uint64_t)value;
    }
    if (value == BOARD1_PI_GATEWAY_INT64_MIN_VALUE)
    {
        return BOARD1_PI_GATEWAY_INT64_MIN_ABS;
    }
    return (uint64_t)(-value);
}

static void board1_pi_gateway_copy_text(char *dst, uint16_t dst_len, const char *src)
{
    uint16_t i = 0U;

    if ((dst == 0) || (dst_len == 0U))
    {
        return;
    }

    if (src != 0)
    {
        while ((src[i] != '\0') && (i < (uint16_t)(dst_len - 1U)))
        {
            char ch = src[i];
            if ((ch == '|') || (ch == '\r') || (ch == '\n'))
            {
                ch = ' ';
            }
            dst[i] = ch;
            i++;
        }
    }

    dst[i] = '\0';
}

static void board1_pi_gateway_store_command_id(const char *src)
{
    board1_pi_gateway_copy_text(g_board1_pi_gateway_last_command_id,
                          BOARD1_PI_GATEWAY_COMMAND_ID_MAX_LEN,
                          src);
}

static void board1_pi_gateway_clear_fan_move_task(void)
{
    g_pi_gateway_fan_move_task_state = BOARD1_PI_GATEWAY_FAN_MOVE_TASK_IDLE;
    g_board1_pi_gateway_debug_dirty = 1U;
}

static void board1_pi_gateway_cancel_fan_move_task_outputs(void)
{
    board1_pi_gateway_clear_fan_move_task();
    Board1_Relay_Set(BOARD1_RELAY_CH_HOT_AIR, 0U);
    Board1_Relay_Set(BOARD1_RELAY_CH_FAN_MAIN, 0U);
}

static void board1_pi_gateway_stop_fan_move_task_outputs(void)
{
    board1_pi_gateway_clear_fan_move_task();
    Board1_HotAirStepper_Stop();
    Board1_HotAirStepper_Enable(0U);
    Board1_Relay_Set(BOARD1_RELAY_CH_HOT_AIR, 0U);
    Board1_Relay_Set(BOARD1_RELAY_CH_FAN_MAIN, 0U);
}

static void board1_pi_gateway_stop_output_switches(void)
{
    Board1_UvLamp_Set(0U);
    Board1_VentFan_Set(0U);
    g_board1_pi_gateway_last_output_device = 0U;
    g_board1_pi_gateway_last_output_enable = 0U;
    g_board1_pi_gateway_debug_dirty = 1U;
}

static void board1_pi_gateway_wifi_clear_aps(void)
{
    uint8_t i;

    for (i = 0U; i < BOARD1_PI_GATEWAY_WIFI_MAX_APS; i++)
    {
        g_pi_gateway_wifi_aps[i].ssid[0] = '\0';
        g_pi_gateway_wifi_aps[i].rssi = 0;
        g_pi_gateway_wifi_aps[i].auth = 0U;
        g_pi_gateway_wifi_aps[i].valid = 0U;
    }

    g_pi_gateway_wifi_count = 0U;
}

static void board1_pi_gateway_wifi_set_status(uint8_t status, const char *info)
{
    g_board1_pi_gateway_wifi_status = status;
    board1_pi_gateway_copy_text(g_pi_gateway_wifi_info, BOARD1_PI_GATEWAY_WIFI_INFO_MAX_LEN, info);
    g_board1_pi_gateway_wifi_status_update = 1U;
}

static uint8_t board1_pi_gateway_checksum(const uint8_t *data, uint8_t len)
{
    uint8_t checksum = 0U;
    uint8_t i;

    for (i = 0U; i < len; i++)
    {
        checksum ^= data[i];
    }

    return checksum;
}

static uint8_t board1_pi_gateway_style_is_valid(uint8_t style_id)
{
    switch (style_id)
    {
        case BOARD1_PI_GATEWAY_STYLE_T_SHIRT:
        case BOARD1_PI_GATEWAY_STYLE_SHIRT:
        case BOARD1_PI_GATEWAY_STYLE_COAT:
        case BOARD1_PI_GATEWAY_STYLE_DRESS:
        case BOARD1_PI_GATEWAY_STYLE_PANTS:
        case BOARD1_PI_GATEWAY_STYLE_SKIRT:
        case BOARD1_PI_GATEWAY_STYLE_SHORTS:
        case BOARD1_PI_GATEWAY_STYLE_SWEATER:
        case BOARD1_PI_GATEWAY_STYLE_HOODIE:
        case BOARD1_PI_GATEWAY_STYLE_JACKET:
            return 1U;

        default:
            return 0U;
    }
}

static void board1_pi_gateway_accept_response(uint8_t status, uint8_t style_id)
{
    if ((status == BOARD1_PI_GATEWAY_STATUS_OK) &&
        (board1_pi_gateway_style_is_valid(style_id) == 0U))
    {
        status = BOARD1_PI_GATEWAY_STATUS_NO_RESULT;
        style_id = BOARD1_PI_GATEWAY_STYLE_UNKNOWN;
    }

    g_board1_pi_gateway_last_status = status;
    if (status == BOARD1_PI_GATEWAY_STATUS_OK)
    {
        g_board1_pi_gateway_last_style_id = style_id;
    }
    else
    {
        g_board1_pi_gateway_last_style_id = BOARD1_PI_GATEWAY_STYLE_UNKNOWN;
    }
    g_board1_pi_gateway_response_ready = 1U;
    g_board1_pi_gateway_busy = 0U;
    g_pi_gateway_rx_len = 0U;
}

static void board1_pi_gateway_accept_image_info(uint32_t image_size)
{
    if ((image_size == 0U) || (image_size > BOARD1_PI_GATEWAY_IMAGE_MAX_SIZE))
    {
        g_board1_pi_gateway_image_error = 1U;
        g_board1_pi_gateway_image_info_ready = 0U;
        g_board1_pi_gateway_image_size = 0U;
        return;
    }

    g_board1_pi_gateway_image_size = image_size;
    g_board1_pi_gateway_image_received = 0U;
    g_board1_pi_gateway_image_done = 0U;
    g_board1_pi_gateway_image_error = 0U;
    g_board1_pi_gateway_image_info_ready = 1U;
}

static void board1_pi_gateway_handle_wifi_ap_line(char *payload)
{
    char *index_text;
    char *rssi_text;
    char *auth_text;
    char *ssid_text;
    uint8_t index;

    index_text = strtok(payload, "|");
    rssi_text = strtok(0, "|");
    auth_text = strtok(0, "|");
    ssid_text = strtok(0, "");

    if ((index_text == 0) || (rssi_text == 0) || (auth_text == 0) || (ssid_text == 0))
    {
        return;
    }

    index = (uint8_t)atoi(index_text);
    if (index >= BOARD1_PI_GATEWAY_WIFI_MAX_APS)
    {
        return;
    }

    board1_pi_gateway_copy_text(g_pi_gateway_wifi_aps[index].ssid,
                          BOARD1_PI_GATEWAY_WIFI_SSID_MAX_LEN,
                          ssid_text);
    g_pi_gateway_wifi_aps[index].rssi = (int16_t)atoi(rssi_text);
    g_pi_gateway_wifi_aps[index].auth = (uint8_t)atoi(auth_text);
    g_pi_gateway_wifi_aps[index].valid = 1U;
    if (g_pi_gateway_wifi_count <= index)
    {
        g_pi_gateway_wifi_count = (uint8_t)(index + 1U);
    }
}

static void board1_pi_gateway_queue_local_motion(uint8_t cmd,
                                           uint8_t control,
                                           uint64_t angle_deg,
                                           int64_t target_deg)
{
    __disable_irq();
    if (g_pi_gateway_local_motion_q_count < BOARD1_PI_GATEWAY_LOCAL_MOTION_QUEUE_LEN)
    {
        g_pi_gateway_local_motion_queue[g_pi_gateway_local_motion_q_tail].cmd = cmd;
        g_pi_gateway_local_motion_queue[g_pi_gateway_local_motion_q_tail].control = control;
        g_pi_gateway_local_motion_queue[g_pi_gateway_local_motion_q_tail].angle_deg = angle_deg;
        g_pi_gateway_local_motion_queue[g_pi_gateway_local_motion_q_tail].target_deg = target_deg;
        g_pi_gateway_local_motion_q_tail =
            (uint8_t)((g_pi_gateway_local_motion_q_tail + 1U) % BOARD1_PI_GATEWAY_LOCAL_MOTION_QUEUE_LEN);
        g_pi_gateway_local_motion_q_count++;
    }
    __enable_irq();
}

static uint8_t board1_pi_gateway_take_local_motion(board1_pi_gateway_local_motion_t *motion)
{
    if (motion == 0)
    {
        return 0U;
    }

    __disable_irq();
    if (g_pi_gateway_local_motion_q_count == 0U)
    {
        __enable_irq();
        return 0U;
    }

    motion->cmd = g_pi_gateway_local_motion_queue[g_pi_gateway_local_motion_q_head].cmd;
    motion->control = g_pi_gateway_local_motion_queue[g_pi_gateway_local_motion_q_head].control;
    motion->angle_deg = g_pi_gateway_local_motion_queue[g_pi_gateway_local_motion_q_head].angle_deg;
    motion->target_deg = g_pi_gateway_local_motion_queue[g_pi_gateway_local_motion_q_head].target_deg;
    g_pi_gateway_local_motion_q_head =
        (uint8_t)((g_pi_gateway_local_motion_q_head + 1U) % BOARD1_PI_GATEWAY_LOCAL_MOTION_QUEUE_LEN);
    g_pi_gateway_local_motion_q_count--;
    __enable_irq();

    return 1U;
}

static void board1_pi_gateway_execute_local_motion(const board1_pi_gateway_local_motion_t *motion)
{
    uint8_t dir;

    if (motion == 0)
    {
        return;
    }

    if (motion->cmd == CMD_STEPPER_ENABLE)
    {
        Board1_HotAirStepper_Enable((motion->control != 0U) ? 1U : 0U);
    }
    else if (motion->cmd == CMD_STEPPER_MOVE_ANGLE)
    {
        dir = (motion->control == BOARD_LINK_DIR_REVERSE) ?
              BOARD1_HOT_AIR_DIR_REVERSE :
              BOARD1_HOT_AIR_DIR_FORWARD;
        Board1_HotAirStepper_MoveAngle(dir, motion->angle_deg);
    }
    else if (motion->cmd == CMD_STEPPER_GOTO_ANGLE)
    {
        Board1_HotAirStepper_GotoAngle(motion->target_deg);
    }
    else if (motion->cmd == CMD_EMERGENCY)
    {
        Board1_HotAirStepper_Stop();
        Board1_HotAirStepper_Enable(0U);
        board1_pi_gateway_stop_output_switches();
    }
}

static void board1_pi_gateway_handle_motion_line(char *payload)
{
    char *cmd_text;
    char *channel_text;
    char *control_text;
    char *angle_text;
    char *command_id_text;
    uint8_t cmd;
    uint8_t channel;
    uint8_t control;
    uint64_t local_angle_deg;
    uint16_t link_angle_deg;
    int64_t target_deg;

    cmd_text = strtok(payload, "|");
    channel_text = strtok(0, "|");
    control_text = strtok(0, "|");
    angle_text = strtok(0, "|");
    command_id_text = strtok(0, "|");

    if ((cmd_text == 0) || (channel_text == 0) || (control_text == 0) || (angle_text == 0))
    {
        return;
    }

    cmd = (uint8_t)atoi(cmd_text);
    channel = (uint8_t)atoi(channel_text);
    control = (uint8_t)atoi(control_text);
    target_deg = board1_pi_gateway_parse_i64(angle_text);
    if (cmd == CMD_STEPPER_GOTO_ANGLE)
    {
        local_angle_deg = board1_pi_gateway_abs_i64(target_deg);
    }
    else
    {
        local_angle_deg = (target_deg < 0) ? 0U : (uint64_t)target_deg;
    }
    link_angle_deg = (local_angle_deg > 65535ULL) ? 65535U : (uint16_t)local_angle_deg;
    g_board1_pi_gateway_motion_cmd_count++;
    g_board1_pi_gateway_last_motion_cmd = cmd;
    g_board1_pi_gateway_last_motion_channel = channel;
    g_board1_pi_gateway_last_motion_angle = link_angle_deg;
    board1_pi_gateway_store_command_id(command_id_text);
    g_board1_pi_gateway_debug_dirty = 1U;

    if ((channel == BOARD1_HOT_AIR_MOTION_CHANNEL) &&
        ((cmd == CMD_STEPPER_ENABLE) ||
         (cmd == CMD_STEPPER_MOVE_ANGLE) ||
         (cmd == CMD_STEPPER_GOTO_ANGLE)))
    {
        board1_pi_gateway_cancel_fan_move_task_outputs();
        board1_pi_gateway_queue_local_motion(cmd, control, local_angle_deg, target_deg);
        return;
    }

    if (cmd == CMD_STEPPER_ENABLE)
    {
        Board1_LinkSetStepperEnable(channel, control);
    }
    else if (cmd == CMD_STEPPER_MOVE_ANGLE)
    {
        Board1_LinkMoveStepperAngle(channel, control, link_angle_deg);
    }
    else if (cmd == CMD_STEPPER_GOTO_ANGLE)
    {
        if (target_deg > 32767)
        {
            target_deg = 32767;
        }
        else if (target_deg < -32768)
        {
            target_deg = -32768;
        }
        Board1_LinkGotoStepperAngle(channel, (int16_t)target_deg);
    }
    else if (cmd == CMD_EMERGENCY)
    {
        board1_pi_gateway_stop_fan_move_task_outputs();
        board1_pi_gateway_queue_local_motion(CMD_EMERGENCY, 0U, 0U, 0);
        Board1_LinkEmergencyStop();
    }
}

static void board1_pi_gateway_handle_servo_line(char *payload)
{
    char *channel_text;
    char *angle_text;
    char *command_id_text;
    uint8_t channel;
    uint16_t angle_deg;

    channel_text = strtok(payload, "|");
    angle_text = strtok(0, "|");
    command_id_text = strtok(0, "|");

    if ((channel_text == 0) || (angle_text == 0))
    {
        return;
    }

    channel = (uint8_t)atoi(channel_text);
    angle_deg = (uint16_t)atoi(angle_text);
    g_board1_pi_gateway_servo_cmd_count++;
    g_board1_pi_gateway_last_servo_channel = channel;
    g_board1_pi_gateway_last_servo_angle = angle_deg;
    board1_pi_gateway_store_command_id(command_id_text);
    g_board1_pi_gateway_debug_dirty = 1U;
    Board1_LinkSetServoAngle(channel, angle_deg);
}

static uint8_t board1_pi_gateway_output_device_from_text(const char *text)
{
    if (text == 0)
    {
        return 0U;
    }

    if ((strcmp(text, "1") == 0) ||
        (strcmp(text, "uv") == 0) ||
        (strcmp(text, "uv_lamp") == 0) ||
        (strcmp(text, "dirc") == 0) ||
        (strcmp(text, "ultraviolet") == 0))
    {
        return BOARD1_PI_GATEWAY_OUTPUT_UV_LAMP;
    }

    if ((strcmp(text, "2") == 0) ||
        (strcmp(text, "fan") == 0) ||
        (strcmp(text, "vent") == 0) ||
        (strcmp(text, "vent_fan") == 0) ||
        (strcmp(text, "pwmb") == 0))
    {
        return BOARD1_PI_GATEWAY_OUTPUT_VENT_FAN;
    }

    return 0U;
}

static void board1_pi_gateway_handle_output_line(char *payload)
{
    char *device_text;
    char *enable_text;
    char *command_id_text;
    uint8_t device;
    uint8_t enable;

    device_text = strtok(payload, "|");
    enable_text = strtok(0, "|");
    command_id_text = strtok(0, "|");

    if ((device_text == 0) || (enable_text == 0))
    {
        return;
    }

    device = board1_pi_gateway_output_device_from_text(device_text);
    enable = (atoi(enable_text) != 0) ? 1U : 0U;
    if (device == 0U)
    {
        return;
    }

    /* 网页/PI_GATEWAY下发的开关量命令：DIRC 控紫外灯，PWMB 控换气风扇；高电平开启。 */
    if (device == BOARD1_PI_GATEWAY_OUTPUT_UV_LAMP)
    {
        Board1_UvLamp_Set(enable);
    }
    else if (device == BOARD1_PI_GATEWAY_OUTPUT_VENT_FAN)
    {
        Board1_VentFan_Set(enable);
    }

    g_board1_pi_gateway_output_cmd_count++;
    g_board1_pi_gateway_last_output_device = device;
    g_board1_pi_gateway_last_output_enable = enable;
    board1_pi_gateway_store_command_id(command_id_text);
    g_board1_pi_gateway_debug_dirty = 1U;
}

static void board1_pi_gateway_start_fan_move_task(uint32_t now_ms)
{
    Board1_Relay_Set(BOARD1_RELAY_CH_HOT_AIR, 0U);
    Board1_Relay_Set(BOARD1_RELAY_CH_FAN_MAIN, 1U);
    Board1_HotAirStepper_Enable(1U);

    /* 任务8不再阻塞主循环：先到外侧点，后续由 service 按时间回到0，调试和急停都能继续响应。 */
    Board1_HotAirStepper_GotoAngle(BOARD1_HOT_AIR_TASK8_OUT_TARGET_DEG);
    g_pi_gateway_fan_move_task_tick_ms = now_ms;
    g_pi_gateway_fan_move_task_state = BOARD1_PI_GATEWAY_FAN_MOVE_TASK_OUT;
    g_board1_pi_gateway_debug_dirty = 1U;
    board1_pi_gateway_send_task_status(BOARD1_PI_GATEWAY_ACTION_FAN_MOVE, g_pi_gateway_fan_task_command_id, "running", 0U);
}

static void board1_pi_gateway_stop_fan_move_task(void)
{
    board1_pi_gateway_stop_fan_move_task_outputs();
    board1_pi_gateway_send_task_status(BOARD1_PI_GATEWAY_ACTION_FAN_STOP, g_pi_gateway_fan_task_command_id, "stopped", 0U);
}

static void board1_pi_gateway_fan_move_task_service(uint32_t now_ms)
{
    if ((g_pi_gateway_fan_move_task_state == BOARD1_PI_GATEWAY_FAN_MOVE_TASK_OUT) &&
        ((uint32_t)(now_ms - g_pi_gateway_fan_move_task_tick_ms) >= BOARD1_HOT_AIR_TASK8_WAIT_MS))
    {
        Board1_HotAirStepper_GotoAngle(BOARD1_HOT_AIR_TASK8_HOME_TARGET_DEG);
        g_pi_gateway_fan_move_task_tick_ms = now_ms;
        g_pi_gateway_fan_move_task_state = BOARD1_PI_GATEWAY_FAN_MOVE_TASK_HOME;
        g_board1_pi_gateway_debug_dirty = 1U;
    }
    else if ((g_pi_gateway_fan_move_task_state == BOARD1_PI_GATEWAY_FAN_MOVE_TASK_HOME) &&
             ((uint32_t)(now_ms - g_pi_gateway_fan_move_task_tick_ms) >= BOARD1_HOT_AIR_TASK8_WAIT_MS))
    {
        g_pi_gateway_fan_move_task_state = BOARD1_PI_GATEWAY_FAN_MOVE_TASK_IDLE;
        g_board1_pi_gateway_debug_dirty = 1U;
        board1_pi_gateway_send_task_status(BOARD1_PI_GATEWAY_ACTION_FAN_MOVE, g_pi_gateway_fan_task_command_id, "done", 0U);
    }
}

void board1_uart3_pi_gateway_action_service(uint32_t now_ms)
{
    uint8_t action_id;
    board1_pi_gateway_local_motion_t motion;

    __disable_irq();
    action_id = g_pi_gateway_local_action_pending;
    g_pi_gateway_local_action_pending = 0U;
    __enable_irq();

    if (action_id == BOARD1_PI_GATEWAY_ACTION_FAN_MOVE)
    {
        board1_pi_gateway_start_fan_move_task(now_ms);
    }
    else if (action_id == BOARD1_PI_GATEWAY_ACTION_FAN_STOP)
    {
        board1_pi_gateway_stop_fan_move_task();
    }

    board1_pi_gateway_fan_move_task_service(now_ms);

    while (board1_pi_gateway_take_local_motion(&motion) != 0U)
    {
        board1_pi_gateway_execute_local_motion(&motion);
    }
}

static void board1_pi_gateway_handle_action_line(char *payload)
{
    char *action_text;
    char *command_id_text;
    char *target_c_text;
    uint8_t action_id;
    uint8_t target_c = 0U;

    action_text = strtok(payload, "|");
    command_id_text = strtok(0, "|");
    target_c_text = strtok(0, "|");
    if (action_text == 0)
    {
        return;
    }

    action_id = (uint8_t)atoi(action_text);
    if (target_c_text != 0)
    {
        target_c = (uint8_t)atoi(target_c_text);
    }
    g_board1_pi_gateway_action_cmd_count++;
    board1_pi_gateway_store_command_id(command_id_text);
    g_board1_pi_gateway_debug_dirty = 1U;
    board1_pi_gateway_send_task_status(action_id, command_id_text, "received", target_c);

    if ((action_id == BOARD1_PI_GATEWAY_ACTION_FAN_MOVE) ||
        (action_id == BOARD1_PI_GATEWAY_ACTION_FAN_STOP))
    {
        board1_pi_gateway_copy_command_id(g_pi_gateway_fan_task_command_id,
                                          (uint8_t)sizeof(g_pi_gateway_fan_task_command_id),
                                          command_id_text);
        g_pi_gateway_local_action_pending = action_id;
        return;
    }

    if (action_id == 0U)
    {
        board1_pi_gateway_stop_output_switches();
    }

    if ((action_id != 0U) &&
        (action_id != 5U) &&
        (g_pi_gateway_board2_active_action_id != 0U))
    {
        board1_pi_gateway_send_task_status(action_id, command_id_text, "error", target_c);
        return;
    }

    if (Board1_LinkStartAction(action_id, target_c) == 0U)
    {
        board1_pi_gateway_send_task_status(action_id, command_id_text, "error", target_c);
        return;
    }

    if ((action_id != 0U) && (action_id != 5U))
    {
        board1_pi_gateway_set_board2_active_task(action_id, command_id_text, target_c);
    }
    board1_pi_gateway_send_task_status(action_id, command_id_text, "forwarded", target_c);
}

static void board1_pi_gateway_handle_ascii_line(char *line)
{
    if (line == 0)
    {
        return;
    }

    if (strncmp(line, "@MCMD|", 6U) == 0)
    {
        g_board1_pi_gateway_ascii_cmd_count++;
        g_board1_pi_gateway_last_ascii_cmd = 'M';
        board1_pi_gateway_handle_motion_line(line + 6U);
        return;
    }

    if (strncmp(line, "@SCMD|", 6U) == 0)
    {
        g_board1_pi_gateway_ascii_cmd_count++;
        g_board1_pi_gateway_last_ascii_cmd = 'S';
        board1_pi_gateway_handle_servo_line(line + 6U);
        return;
    }

    if (strncmp(line, "@ACMD|", 6U) == 0)
    {
        g_board1_pi_gateway_ascii_cmd_count++;
        g_board1_pi_gateway_last_ascii_cmd = 'A';
        board1_pi_gateway_handle_action_line(line + 6U);
        return;
    }

    if (strncmp(line, "@OCMD|", 6U) == 0)
    {
        g_board1_pi_gateway_ascii_cmd_count++;
        g_board1_pi_gateway_last_ascii_cmd = 'O';
        board1_pi_gateway_handle_output_line(line + 6U);
        return;
    }

    if (strncmp(line, "@WSCAN_BEGIN", 12U) == 0)
    {
        board1_pi_gateway_wifi_clear_aps();
        g_board1_pi_gateway_wifi_scan_ready = 0U;
        board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_SCANNING, "scan");
        return;
    }

    if (strncmp(line, "@WAP|", 5U) == 0)
    {
        board1_pi_gateway_handle_wifi_ap_line(line + 5U);
        return;
    }

    if (strncmp(line, "@WSCAN_END", 10U) == 0)
    {
        g_board1_pi_gateway_wifi_scan_ready = 1U;
        g_board1_pi_gateway_wifi_busy = 0U;
        board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_SCAN_DONE, "scan_done");
        return;
    }

    if (strncmp(line, "@WIFI_CONNECTING|", 17U) == 0)
    {
        board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_CONNECTING, line + 17U);
        return;
    }

    if (strncmp(line, "@WIFI_OK|", 9U) == 0)
    {
        g_board1_pi_gateway_wifi_busy = 0U;
        board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_CONNECTED, line + 9U);
        return;
    }

    if (strncmp(line, "@WIFI_FAIL|", 11U) == 0)
    {
        g_board1_pi_gateway_wifi_busy = 0U;
        board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_FAIL, line + 11U);
        return;
    }

    if (strncmp(line, "@WIFI_ERR|", 10U) == 0)
    {
        g_board1_pi_gateway_wifi_busy = 0U;
        board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_FAIL, line + 10U);
        return;
    }
}

static uint8_t board1_pi_gateway_try_parse_frame(uint8_t frame_len)
{
    if ((board1_pi_gateway_checksum((const uint8_t *)g_pi_gateway_rx_buf, (uint8_t)(frame_len - 1U)) !=
         g_pi_gateway_rx_buf[frame_len - 1U]) ||
        (g_pi_gateway_rx_buf[0] != BOARD1_PI_GATEWAY_FRAME_HEADER))
    {
        return 0U;
    }

    if ((frame_len == BOARD1_PI_GATEWAY_STATUS_FRAME_LEN) &&
        (g_pi_gateway_rx_buf[1] == BOARD1_PI_GATEWAY_CMD_CAPTURE))
    {
        board1_pi_gateway_accept_response(g_pi_gateway_rx_buf[2],
                                    BOARD1_PI_GATEWAY_STYLE_UNKNOWN);
        return 1U;
    }

    if ((frame_len == BOARD1_PI_GATEWAY_RESULT_FRAME_LEN) &&
        (g_pi_gateway_rx_buf[1] == BOARD1_PI_GATEWAY_CMD_RESULT))
    {
        board1_pi_gateway_accept_response(g_pi_gateway_rx_buf[2],
                                    g_pi_gateway_rx_buf[3]);
        return 1U;
    }

    if ((frame_len == BOARD1_PI_GATEWAY_IMAGE_INFO_FRAME_LEN) &&
        (g_pi_gateway_rx_buf[1] == BOARD1_PI_GATEWAY_CMD_IMAGE_INFO))
    {
        uint32_t image_size;

        image_size = ((uint32_t)g_pi_gateway_rx_buf[2] << 24) |
                     ((uint32_t)g_pi_gateway_rx_buf[3] << 16) |
                     ((uint32_t)g_pi_gateway_rx_buf[4] << 8) |
                     ((uint32_t)g_pi_gateway_rx_buf[5]);
        board1_pi_gateway_accept_image_info(image_size);
        g_pi_gateway_rx_len = 0U;
        return 1U;
    }

    return 0U;
}

static void board1_pi_gateway_resync_buffer(void)
{
    uint8_t i;

    if (g_board1_pi_gateway_bad_frame_count < 0xFFU)
    {
        g_board1_pi_gateway_bad_frame_count++;
    }

    for (i = 1U; i < g_pi_gateway_rx_len; i++)
    {
        if (g_pi_gateway_rx_buf[i] == BOARD1_PI_GATEWAY_FRAME_HEADER)
        {
            uint8_t j;
            uint8_t remain_len = (uint8_t)(g_pi_gateway_rx_len - i);

            for (j = 0U; j < remain_len; j++)
            {
                g_pi_gateway_rx_buf[j] = g_pi_gateway_rx_buf[i + j];
            }

            g_pi_gateway_rx_len = remain_len;
            return;
        }
    }

    g_pi_gateway_rx_len = 0U;
}

static void board1_pi_gateway_handle_byte(uint8_t byte)
{
    if (g_pi_gateway_image_streaming != 0U)
    {
        board1_uart6_screen_send_byte(byte);
        g_board1_pi_gateway_image_received++;

        if (g_board1_pi_gateway_image_received >= g_board1_pi_gateway_image_size)
        {
            g_pi_gateway_image_streaming = 0U;
            g_board1_pi_gateway_image_done = 1U;
        }
        return;
    }

    if ((g_pi_gateway_ascii_rx_len != 0U) ||
        ((g_pi_gateway_rx_len == 0U) && (byte == '@')))
    {
        if (byte == '\r')
        {
            return;
        }

        if (byte == '\n')
        {
            g_pi_gateway_ascii_rx_buf[g_pi_gateway_ascii_rx_len] = '\0';
            board1_pi_gateway_handle_ascii_line(g_pi_gateway_ascii_rx_buf);
            g_pi_gateway_ascii_rx_len = 0U;
            return;
        }

        if (g_pi_gateway_ascii_rx_len < (uint16_t)(BOARD1_PI_GATEWAY_ASCII_RX_BUF_LEN - 1U))
        {
            g_pi_gateway_ascii_rx_buf[g_pi_gateway_ascii_rx_len] = (char)byte;
            g_pi_gateway_ascii_rx_len++;
        }
        else
        {
            g_pi_gateway_ascii_rx_len = 0U;
        }
        return;
    }

    if (g_pi_gateway_rx_len == 0U)
    {
        if (byte != BOARD1_PI_GATEWAY_FRAME_HEADER)
        {
            return;
        }
    }

    g_pi_gateway_rx_buf[g_pi_gateway_rx_len] = byte;
    g_pi_gateway_rx_len++;

    if (g_pi_gateway_rx_len == BOARD1_PI_GATEWAY_STATUS_FRAME_LEN)
    {
        if (board1_pi_gateway_try_parse_frame(BOARD1_PI_GATEWAY_STATUS_FRAME_LEN) != 0U)
        {
            return;
        }
        if (((g_pi_gateway_rx_buf[1] != BOARD1_PI_GATEWAY_CMD_RESULT) &&
             (g_pi_gateway_rx_buf[1] != BOARD1_PI_GATEWAY_CMD_IMAGE_INFO)) ||
            (g_pi_gateway_rx_len >= BOARD1_PI_GATEWAY_RX_BUF_LEN))
        {
            board1_pi_gateway_resync_buffer();
        }
        return;
    }

    if (g_pi_gateway_rx_len == BOARD1_PI_GATEWAY_RESULT_FRAME_LEN)
    {
        if (board1_pi_gateway_try_parse_frame(BOARD1_PI_GATEWAY_RESULT_FRAME_LEN) != 0U)
        {
            return;
        }
        if ((g_pi_gateway_rx_buf[1] == BOARD1_PI_GATEWAY_CMD_IMAGE_INFO) &&
            (g_pi_gateway_rx_len < BOARD1_PI_GATEWAY_IMAGE_INFO_FRAME_LEN))
        {
            return;
        }
        board1_pi_gateway_resync_buffer();
        return;
    }

    if (g_pi_gateway_rx_len == BOARD1_PI_GATEWAY_IMAGE_INFO_FRAME_LEN)
    {
        if (board1_pi_gateway_try_parse_frame(BOARD1_PI_GATEWAY_IMAGE_INFO_FRAME_LEN) != 0U)
        {
            return;
        }
        board1_pi_gateway_resync_buffer();
        return;
    }

    if (g_pi_gateway_rx_len >= BOARD1_PI_GATEWAY_RX_BUF_LEN)
    {
        board1_pi_gateway_resync_buffer();
    }

}

void board1_uart3_pi_gateway_init(uint32_t baud)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);

    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpio);

    usart.USART_BaudRate = baud;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &usart);

    nvic.NVIC_IRQChannel = USART3_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 2U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART3, ENABLE);

    g_board1_pi_gateway_last_status = BOARD1_PI_GATEWAY_STATUS_IDLE;
    g_board1_pi_gateway_last_style_id = BOARD1_PI_GATEWAY_STYLE_UNKNOWN;
    g_board1_pi_gateway_response_ready = 0U;
    g_board1_pi_gateway_busy = 0U;
    g_board1_pi_gateway_bad_frame_count = 0U;
    g_board1_pi_gateway_image_size = 0U;
    g_board1_pi_gateway_image_received = 0U;
    g_board1_pi_gateway_image_info_ready = 0U;
    g_board1_pi_gateway_image_done = 0U;
    g_board1_pi_gateway_image_error = 0U;
    g_board1_pi_gateway_wifi_scan_ready = 0U;
    g_board1_pi_gateway_wifi_status_update = 0U;
    g_board1_pi_gateway_wifi_status = BOARD1_PI_GATEWAY_WIFI_STATUS_IDLE;
    g_board1_pi_gateway_wifi_busy = 0U;
    g_board1_pi_gateway_ascii_cmd_count = 0U;
    g_board1_pi_gateway_motion_cmd_count = 0U;
    g_board1_pi_gateway_servo_cmd_count = 0U;
    g_board1_pi_gateway_action_cmd_count = 0U;
    g_board1_pi_gateway_output_cmd_count = 0U;
    g_board1_pi_gateway_last_ascii_cmd = 0U;
    g_board1_pi_gateway_last_motion_cmd = 0U;
    g_board1_pi_gateway_last_motion_channel = 0U;
    g_board1_pi_gateway_last_motion_angle = 0U;
    g_board1_pi_gateway_last_servo_channel = 0U;
    g_board1_pi_gateway_last_servo_angle = 0U;
    g_board1_pi_gateway_last_output_device = 0U;
    g_board1_pi_gateway_last_output_enable = 0U;
    g_board1_pi_gateway_debug_dirty = 0U;
    g_board1_pi_gateway_last_command_id[0] = '\0';
    g_pi_gateway_rx_len = 0U;
    g_pi_gateway_ascii_rx_len = 0U;
    g_pi_gateway_request_tick_ms = 0U;
    g_pi_gateway_wifi_request_tick_ms = 0U;
    g_pi_gateway_image_streaming = 0U;
    g_pi_gateway_fan_move_task_state = BOARD1_PI_GATEWAY_FAN_MOVE_TASK_IDLE;
    g_pi_gateway_fan_move_task_tick_ms = 0U;
    g_pi_gateway_board2_active_action_id = 0U;
    g_pi_gateway_board2_active_target_c = 0U;
    g_pi_gateway_board2_active_command_id[0] = '\0';
    board1_pi_gateway_wifi_clear_aps();
    g_pi_gateway_wifi_info[0] = '\0';
}

void board1_uart3_pi_gateway_send_byte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET) {}
    USART_SendData(USART3, byte);
}

void board1_uart3_pi_gateway_send(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    for (i = 0U; i < len; i++)
    {
        board1_uart3_pi_gateway_send_byte(data[i]);
    }

    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET) {}
}

void board1_uart3_pi_gateway_send_line(const char *line)
{
    if (line == 0)
    {
        return;
    }

    while (*line != '\0')
    {
        board1_uart3_pi_gateway_send_byte((uint8_t)(*line));
        line++;
    }

    board1_uart3_pi_gateway_send_byte('\n');
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET) {}
}

void board1_uart3_pi_gateway_debug_service(uint32_t now_ms)
{
    static uint32_t last_send_tick_ms = 0U;
    static uint32_t last_ascii_count = 0U;
    static uint32_t last_tx_sent_count = 0U;
    char line[192];

    if ((g_board1_pi_gateway_debug_dirty == 0U) &&
        (last_ascii_count == g_board1_pi_gateway_ascii_cmd_count) &&
        (last_tx_sent_count == g_board1_link_tx_sent_count))
    {
        return;
    }

    if ((uint32_t)(now_ms - last_send_tick_ms) < 120U)
    {
        return;
    }

    last_send_tick_ms = now_ms;
    last_ascii_count = g_board1_pi_gateway_ascii_cmd_count;
    last_tx_sent_count = g_board1_link_tx_sent_count;
    g_board1_pi_gateway_debug_dirty = 0U;

    snprintf(line,
             sizeof(line),
             "@DBG1|id=%s|rx=%lu|last=%c|m=%lu|s=%lu|o=%lu|txq=%lu|tx=%lu|drop=%lu|cmd=%u|ch=%u|a=%u|out=%u:%u|fan=%u",
             g_board1_pi_gateway_last_command_id,
             (unsigned long)g_board1_pi_gateway_ascii_cmd_count,
             (char)((g_board1_pi_gateway_last_ascii_cmd != 0U) ? g_board1_pi_gateway_last_ascii_cmd : '-'),
             (unsigned long)g_board1_pi_gateway_motion_cmd_count,
             (unsigned long)g_board1_pi_gateway_servo_cmd_count,
             (unsigned long)g_board1_pi_gateway_output_cmd_count,
             (unsigned long)g_board1_link_tx_queue_count,
             (unsigned long)g_board1_link_tx_sent_count,
             (unsigned long)g_board1_link_tx_drop_count,
             (unsigned int)g_board1_link_last_tx_cmd,
             (unsigned int)g_board1_link_last_tx_channel,
             (unsigned int)g_board1_link_last_tx_angle,
             (unsigned int)g_board1_pi_gateway_last_output_device,
             (unsigned int)g_board1_pi_gateway_last_output_enable,
             (unsigned int)g_pi_gateway_fan_move_task_state);
    board1_uart3_pi_gateway_send_line(line);
}

void board1_uart3_pi_gateway_request_capture(uint32_t now_ms, uint8_t side_id)
{
    uint8_t frame[BOARD1_PI_GATEWAY_TX_FRAME_LEN];

    if (g_board1_pi_gateway_busy != 0U)
    {
        return;
    }

    frame[0] = BOARD1_PI_GATEWAY_FRAME_HEADER;
    frame[1] = BOARD1_PI_GATEWAY_CMD_CAPTURE;
    frame[2] = side_id;
    frame[3] = 0x00U;
    frame[4] = board1_pi_gateway_checksum(frame, BOARD1_PI_GATEWAY_TX_FRAME_LEN - 1U);

    g_board1_pi_gateway_last_status = BOARD1_PI_GATEWAY_STATUS_PENDING;
    g_board1_pi_gateway_last_style_id = BOARD1_PI_GATEWAY_STYLE_UNKNOWN;
    g_board1_pi_gateway_response_ready = 0U;
    g_board1_pi_gateway_busy = 1U;
    g_pi_gateway_request_tick_ms = now_ms;

    board1_uart3_pi_gateway_send(frame, BOARD1_PI_GATEWAY_TX_FRAME_LEN);
}

void board1_uart3_pi_gateway_request_wifi_scan(uint32_t now_ms)
{
    board1_pi_gateway_wifi_clear_aps();
    g_board1_pi_gateway_wifi_scan_ready = 0U;
    g_board1_pi_gateway_wifi_busy = 1U;
    g_pi_gateway_wifi_request_tick_ms = now_ms;
    board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_SCANNING, "scan");
    board1_uart3_pi_gateway_send_line("@WSCAN");
}

void board1_uart3_pi_gateway_connect_wifi(const char *ssid, const char *password, uint32_t now_ms)
{
    char line[160];
    uint16_t idx = 0U;
    uint16_t i;

    if (ssid == 0)
    {
        return;
    }

    g_board1_pi_gateway_wifi_busy = 1U;
    g_pi_gateway_wifi_request_tick_ms = now_ms;
    board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_CONNECTING, ssid);

    line[idx++] = '@';
    line[idx++] = 'W';
    line[idx++] = 'C';
    line[idx++] = 'O';
    line[idx++] = 'N';
    line[idx++] = 'N';
    line[idx++] = '|';

    for (i = 0U; (ssid[i] != '\0') && (idx < 95U); i++)
    {
        char ch = ssid[i];
        if ((ch == '|') || (ch == '\r') || (ch == '\n'))
        {
            ch = ' ';
        }
        line[idx++] = ch;
    }

    line[idx++] = '|';

    if (password != 0)
    {
        for (i = 0U; (password[i] != '\0') && (idx < (uint16_t)(sizeof(line) - 2U)); i++)
        {
            char ch = password[i];
            if ((ch == '|') || (ch == '\r') || (ch == '\n'))
            {
                ch = ' ';
            }
            line[idx++] = ch;
        }
    }

    line[idx] = '\0';
    board1_uart3_pi_gateway_send_line(line);
}

void board1_uart3_pi_gateway_poll(uint32_t now_ms)
{
    if ((g_board1_pi_gateway_wifi_busy != 0U) &&
        ((uint32_t)(now_ms - g_pi_gateway_wifi_request_tick_ms) >= BOARD1_PI_GATEWAY_WIFI_TIMEOUT_MS))
    {
        g_board1_pi_gateway_wifi_busy = 0U;
        board1_pi_gateway_wifi_set_status(BOARD1_PI_GATEWAY_WIFI_STATUS_FAIL, "timeout");
    }

    if (g_board1_pi_gateway_busy == 0U)
    {
        return;
    }

    if ((uint32_t)(now_ms - g_pi_gateway_request_tick_ms) >= BOARD1_PI_GATEWAY_RESPONSE_TIMEOUT_MS)
    {
        g_board1_pi_gateway_last_status = BOARD1_PI_GATEWAY_STATUS_TIMEOUT;
        g_board1_pi_gateway_last_style_id = BOARD1_PI_GATEWAY_STYLE_UNKNOWN;
        g_board1_pi_gateway_response_ready = 1U;
        g_board1_pi_gateway_busy = 0U;
        g_pi_gateway_rx_len = 0U;
    }
}

uint8_t board1_uart3_pi_gateway_is_busy(void)
{
    return g_board1_pi_gateway_busy;
}

uint8_t board1_uart3_pi_gateway_get_last_status(void)
{
    return g_board1_pi_gateway_last_status;
}

uint8_t board1_uart3_pi_gateway_get_last_style_id(void)
{
    return g_board1_pi_gateway_last_style_id;
}

uint8_t board1_uart3_pi_gateway_has_response(void)
{
    return g_board1_pi_gateway_response_ready;
}

void board1_uart3_pi_gateway_clear_response(void)
{
    g_board1_pi_gateway_response_ready = 0U;
}

uint8_t board1_uart3_pi_gateway_has_image_info(void)
{
    return g_board1_pi_gateway_image_info_ready;
}

uint32_t board1_uart3_pi_gateway_get_image_size(void)
{
    return g_board1_pi_gateway_image_size;
}

void board1_uart3_pi_gateway_clear_image_info(void)
{
    g_board1_pi_gateway_image_info_ready = 0U;
}

void board1_uart3_pi_gateway_start_image_stream(uint32_t size)
{
    g_board1_pi_gateway_image_size = size;
    g_board1_pi_gateway_image_received = 0U;
    g_board1_pi_gateway_image_done = 0U;
    g_board1_pi_gateway_image_error = 0U;
    g_pi_gateway_rx_len = 0U;
    g_pi_gateway_image_streaming = 1U;
}

uint8_t board1_uart3_pi_gateway_image_is_done(void)
{
    return g_board1_pi_gateway_image_done;
}

uint8_t board1_uart3_pi_gateway_image_has_error(void)
{
    return g_board1_pi_gateway_image_error;
}

uint8_t board1_uart3_pi_gateway_image_transfer_active(void)
{
    if ((g_board1_pi_gateway_image_info_ready != 0U) || (g_pi_gateway_image_streaming != 0U))
    {
        return 1U;
    }

    return 0U;
}

void board1_uart3_pi_gateway_clear_image_state(void)
{
    g_board1_pi_gateway_image_size = 0U;
    g_board1_pi_gateway_image_received = 0U;
    g_board1_pi_gateway_image_info_ready = 0U;
    g_board1_pi_gateway_image_done = 0U;
    g_board1_pi_gateway_image_error = 0U;
    g_pi_gateway_image_streaming = 0U;
}

void board1_uart3_pi_gateway_send_image_ready(uint8_t ready)
{
    uint8_t frame[BOARD1_PI_GATEWAY_TX_FRAME_LEN];

    frame[0] = BOARD1_PI_GATEWAY_FRAME_HEADER;
    frame[1] = BOARD1_PI_GATEWAY_CMD_IMAGE_READY;
    frame[2] = ready;
    frame[3] = 0x00U;
    frame[4] = board1_pi_gateway_checksum(frame, BOARD1_PI_GATEWAY_TX_FRAME_LEN - 1U);

    board1_uart3_pi_gateway_send(frame, BOARD1_PI_GATEWAY_TX_FRAME_LEN);
}

uint8_t board1_uart3_pi_gateway_wifi_has_scan_result(void)
{
    return g_board1_pi_gateway_wifi_scan_ready;
}

void board1_uart3_pi_gateway_wifi_clear_scan_result(void)
{
    g_board1_pi_gateway_wifi_scan_ready = 0U;
}

uint8_t board1_uart3_pi_gateway_wifi_has_status_update(void)
{
    return g_board1_pi_gateway_wifi_status_update;
}

void board1_uart3_pi_gateway_wifi_clear_status_update(void)
{
    g_board1_pi_gateway_wifi_status_update = 0U;
}

uint8_t board1_uart3_pi_gateway_wifi_get_status(void)
{
    return g_board1_pi_gateway_wifi_status;
}

uint8_t board1_uart3_pi_gateway_wifi_get_count(void)
{
    return g_pi_gateway_wifi_count;
}

const char *board1_uart3_pi_gateway_wifi_get_ssid(uint8_t index)
{
    if ((index >= BOARD1_PI_GATEWAY_WIFI_MAX_APS) || (g_pi_gateway_wifi_aps[index].valid == 0U))
    {
        return "";
    }

    return g_pi_gateway_wifi_aps[index].ssid;
}

int16_t board1_uart3_pi_gateway_wifi_get_rssi(uint8_t index)
{
    if ((index >= BOARD1_PI_GATEWAY_WIFI_MAX_APS) || (g_pi_gateway_wifi_aps[index].valid == 0U))
    {
        return 0;
    }

    return g_pi_gateway_wifi_aps[index].rssi;
}

uint8_t board1_uart3_pi_gateway_wifi_get_auth(uint8_t index)
{
    if ((index >= BOARD1_PI_GATEWAY_WIFI_MAX_APS) || (g_pi_gateway_wifi_aps[index].valid == 0U))
    {
        return 0U;
    }

    return g_pi_gateway_wifi_aps[index].auth;
}

const char *board1_uart3_pi_gateway_wifi_get_info(void)
{
    return g_pi_gateway_wifi_info;
}

void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        uint8_t byte = (uint8_t)USART_ReceiveData(USART3);

        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
        board1_pi_gateway_handle_byte(byte);
    }
}
