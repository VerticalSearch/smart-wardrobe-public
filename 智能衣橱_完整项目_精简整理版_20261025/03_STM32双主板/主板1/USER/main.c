#include "stm32f4xx.h"
#include "delay.h"
#include "timer.h"
#include "control.h"
#include "oled.h"
#include "usartdma.h"
#include "usart4dma.h"
#include "Stepper.h"
#include "key.h"
#include "board1_uart3_pi_gateway.h"
#include "board1_uart6_screen.h"
#include "board1_hx711.h"
#include <stdio.h>

#define BOARD1_PI_GATEWAY_IMAGE_INFO_WAIT_TIMEOUT_MS   8000U
#define BOARD1_HX711_LEFT_TARE_COUNTS                  (-1007843L)
#define BOARD1_HX711_RIGHT_TARE_COUNTS                 (480204L)
#define BOARD1_HX711_REPORT_INTERVAL_MS                200U

static uint8_t g_board1_wifi_selected_index = 0U;

static void board1_screen_show_wifi_status(const char *text)
{
    board1_uart6_screen_set_text("wifi_status", text);
}

static void board1_screen_show_wifi_selected(const char *ssid)
{
    board1_uart6_screen_set_text("wifi_ssid", ssid);
}

static void board1_screen_update_wifi_list(void)
{
    uint8_t i;

    for (i = 0U; i < BOARD1_PI_GATEWAY_WIFI_MAX_APS; i++)
    {
        char component[8];
        const char *ssid = board1_uart3_pi_gateway_wifi_get_ssid(i);

        component[0] = 'w';
        component[1] = 'i';
        component[2] = 'f';
        component[3] = 'i';
        component[4] = (char)('0' + i);
        component[5] = '\0';

        if ((ssid != 0) && (ssid[0] != '\0'))
        {
            board1_uart6_screen_set_text(component, ssid);
        }
        else
        {
            board1_uart6_screen_set_text(component, "");
        }
    }
}

static void board1_screen_update_wifi_status_from_pi_gateway(void)
{
    switch (board1_uart3_pi_gateway_wifi_get_status())
    {
        case BOARD1_PI_GATEWAY_WIFI_STATUS_SCANNING:
            board1_screen_show_wifi_status("Scanning");
            break;

        case BOARD1_PI_GATEWAY_WIFI_STATUS_SCAN_DONE:
            board1_screen_show_wifi_status("Scan done");
            break;

        case BOARD1_PI_GATEWAY_WIFI_STATUS_CONNECTING:
            board1_screen_show_wifi_status("Connecting");
            break;

        case BOARD1_PI_GATEWAY_WIFI_STATUS_CONNECTED:
            board1_screen_show_wifi_status(board1_uart3_pi_gateway_wifi_get_info());
            break;

        case BOARD1_PI_GATEWAY_WIFI_STATUS_FAIL:
            board1_screen_show_wifi_status(board1_uart3_pi_gateway_wifi_get_info());
            break;

        default:
            board1_screen_show_wifi_status("Ready");
            break;
    }
}

static void board1_screen_service(void)
{
    if (board1_uart6_screen_has_event() != 0U)
    {
        uint8_t event = board1_uart6_screen_get_event();

        if (event == BOARD1_SCREEN_EVENT_WIFI_SCAN)
        {
            board1_screen_show_wifi_status("Scanning");
            board1_uart3_pi_gateway_request_wifi_scan(g_board1_tick_ms);
            board1_screen_update_wifi_list();
        }
        else if (event == BOARD1_SCREEN_EVENT_WIFI_SELECT)
        {
            const char *ssid;

            g_board1_wifi_selected_index = board1_uart6_screen_get_selected_wifi_index();
            ssid = board1_uart3_pi_gateway_wifi_get_ssid(g_board1_wifi_selected_index);
            board1_screen_show_wifi_selected(ssid);
        }
        else if (event == BOARD1_SCREEN_EVENT_WIFI_CONNECT)
        {
            const char *ssid = board1_uart3_pi_gateway_wifi_get_ssid(g_board1_wifi_selected_index);

            if ((ssid != 0) && (ssid[0] != '\0'))
            {
                board1_screen_show_wifi_status("Connecting");
                board1_uart3_pi_gateway_connect_wifi(ssid,
                                               board1_uart6_screen_get_wifi_password(),
                                               g_board1_tick_ms);
            }
            else
            {
                board1_screen_show_wifi_status("No SSID");
            }
        }

        board1_uart6_screen_clear_event();
    }

    if (board1_uart3_pi_gateway_wifi_has_scan_result() != 0U)
    {
        board1_screen_update_wifi_list();
        board1_screen_update_wifi_status_from_pi_gateway();
        board1_uart3_pi_gateway_wifi_clear_scan_result();
    }

    if (board1_uart3_pi_gateway_wifi_has_status_update() != 0U)
    {
        board1_screen_update_wifi_status_from_pi_gateway();
        board1_uart3_pi_gateway_wifi_clear_status_update();
    }
}

static void board1_oled_show_status(void)
{
    const volatile Board1_HX711_Data_t *left_data;
    const volatile Board1_HX711_Data_t *right_data;

    Board1_HX711_Update(BOARD1_HX711_LEFT);
    Board1_HX711_Update(BOARD1_HX711_RIGHT);
    left_data = Board1_HX711_GetData(BOARD1_HX711_LEFT);
    right_data = Board1_HX711_GetData(BOARD1_HX711_RIGHT);

    OLED_Clear();

    OLED_ShowString(0, 0, (const u8 *)"HX711 RAW", 16);

    OLED_ShowString(0, 16, (const u8 *)"L:", 16);
    if ((left_data != 0) && (left_data->valid != 0U))
    {
        OLED_ShowIntNum(16, 16, left_data->filtered, 8, 16);
    }
    else
    {
        OLED_ShowString(24, 16, (const u8 *)(((left_data != 0) && (left_data->read_error != 0U)) ? "ERR" : "WAIT"), 16);
    }

    OLED_ShowString(0, 32, (const u8 *)"R:", 16);
    if ((right_data != 0) && (right_data->valid != 0U))
    {
        OLED_ShowIntNum(16, 32, right_data->filtered, 8, 16);
    }
    else
    {
        OLED_ShowString(24, 32, (const u8 *)(((right_data != 0) && (right_data->read_error != 0U)) ? "ERR" : "WAIT"), 16);
    }

    OLED_ShowString(0, 48, (const u8 *)"L:S1/S2 R:S3/S4", 12);

    OLED_Refresh_Gram();
}

static void board1_hx711_report_to_pi(void)
{
    static uint32_t last_report_tick_ms = 0U;
    const volatile Board1_HX711_Data_t *left_data;
    const volatile Board1_HX711_Data_t *right_data;
    int32_t left_net = 0;
    int32_t right_net = 0;
    char line[96];

    if ((uint32_t)(g_board1_tick_ms - last_report_tick_ms) < BOARD1_HX711_REPORT_INTERVAL_MS)
    {
        return;
    }
    last_report_tick_ms = g_board1_tick_ms;

    Board1_HX711_Update(BOARD1_HX711_LEFT);
    Board1_HX711_Update(BOARD1_HX711_RIGHT);
    left_data = Board1_HX711_GetData(BOARD1_HX711_LEFT);
    right_data = Board1_HX711_GetData(BOARD1_HX711_RIGHT);

    if ((left_data == 0) || (right_data == 0) ||
        (left_data->valid == 0U) || (right_data->valid == 0U))
    {
        return;
    }

    left_net = left_data->filtered - BOARD1_HX711_LEFT_TARE_COUNTS;
    right_net = right_data->filtered - BOARD1_HX711_RIGHT_TARE_COUNTS;
    snprintf(line,
             sizeof(line),
             "@CARE|left=%ld|right=%ld",
             (long)left_net,
             (long)right_net);
    board1_uart3_pi_gateway_send_line(line);
}

static void board1_pi_gateway_service(void)
{
    uint8_t key_value;
    static uint8_t image_info_waiting = 0U;
    static uint8_t image_stream_active = 0U;
    static uint32_t image_info_wait_tick_ms = 0U;
    static uint32_t image_stream_tick_ms = 0U;

    board1_uart3_pi_gateway_poll(g_board1_tick_ms);

    if (board1_uart3_pi_gateway_has_response() != 0U)
    {
        if (board1_uart3_pi_gateway_get_last_status() == BOARD1_PI_GATEWAY_STATUS_OK)
        {
            image_info_waiting = 1U;
            image_info_wait_tick_ms = g_board1_tick_ms;
        }
        board1_uart3_pi_gateway_clear_response();
    }

    if ((image_info_waiting != 0U) &&
        ((uint32_t)(g_board1_tick_ms - image_info_wait_tick_ms) >=
         BOARD1_PI_GATEWAY_IMAGE_INFO_WAIT_TIMEOUT_MS))
    {
        image_info_waiting = 0U;
    }

    if ((image_stream_active != 0U) &&
        (board1_uart3_pi_gateway_image_is_done() == 0U) &&
        ((uint32_t)(g_board1_tick_ms - image_stream_tick_ms) >=
         BOARD1_PI_GATEWAY_IMAGE_TRANSFER_TIMEOUT_MS))
    {
        image_stream_active = 0U;
        board1_uart3_pi_gateway_clear_image_state();
    }

    if (board1_uart3_pi_gateway_image_has_error() != 0U)
    {
        image_info_waiting = 0U;
        image_stream_active = 0U;
        board1_uart3_pi_gateway_send_image_ready(0U);
        board1_uart3_pi_gateway_clear_image_state();
    }

    key_value = KEY_Scan(0);
    if (key_value == KEY1_PRES)
    {
        g_board1_camera_trigger_side_id = 0U;
        g_board1_camera_trigger_placeholder = 1U;
    }

    if ((g_board1_camera_trigger_placeholder != 0U) &&
        (board1_uart3_pi_gateway_is_busy() == 0U) &&
        (image_info_waiting == 0U) &&
        (image_stream_active == 0U) &&
        (board1_uart3_pi_gateway_image_transfer_active() == 0U))
    {
        g_board1_camera_trigger_placeholder = 0U;
        board1_uart3_pi_gateway_request_capture(g_board1_tick_ms,
                                                g_board1_camera_trigger_side_id);
    }

    if (board1_uart3_pi_gateway_has_image_info() != 0U)
    {
        uint32_t image_size = board1_uart3_pi_gateway_get_image_size();

        image_info_waiting = 0U;
        board1_uart3_pi_gateway_clear_image_info();
        if (board1_uart6_screen_x5_start_file_write(BOARD1_SCREEN_X5_IMAGE_PATH,
                                                    image_size,
                                                    BOARD1_SCREEN_X5_READY_TIMEOUT_MS) != 0U)
        {
            board1_uart3_pi_gateway_start_image_stream(image_size);
            board1_uart3_pi_gateway_send_image_ready(1U);
            image_stream_active = 1U;
            image_stream_tick_ms = g_board1_tick_ms;
        }
        else
        {
            board1_uart3_pi_gateway_send_image_ready(0U);
            board1_uart3_pi_gateway_clear_image_state();
        }
    }

    if (board1_uart3_pi_gateway_image_is_done() != 0U)
    {
        image_stream_active = 0U;
        if (board1_uart6_screen_x5_wait_file_done(BOARD1_SCREEN_X5_DONE_TIMEOUT_MS) != 0U)
        {
            board1_uart6_screen_x5_show_external_image(BOARD1_SCREEN_X5_IMAGE_COMPONENT,
                                                       BOARD1_SCREEN_X5_IMAGE_PATH);
        }
        board1_uart3_pi_gateway_clear_image_state();
    }
}

int main(void)
{
    uint32_t last_oled_tick_ms = 0U;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init(168);

    Control_Init();
    Board1_RelayIo_Init();
    TIM7_Cnt_Init(1000 - 1, 84 - 1);

    Board1_LinkUart_Init(115200);
    board1_uart3_pi_gateway_init(115200);
    stepper_init(115200);
    Board1_HotAirStepper_Init(115200);
    board1_uart6_screen_init(115200);
    Board1_HX711_Init();
    KEY_Init();

    OLED_Init();
    OLED_Clear();
    OLED_Refresh_Gram();

    while (1)
    {
        Board1_LinkService();
        board1_pi_gateway_service();
        board1_uart3_pi_gateway_action_service(g_board1_tick_ms);
        board1_uart3_pi_gateway_debug_service(g_board1_tick_ms);
        board1_hx711_report_to_pi();
        board1_screen_service();
        if ((uint32_t)(g_board1_tick_ms - last_oled_tick_ms) >= 200U)
        {
            last_oled_tick_ms = g_board1_tick_ms;
            board1_oled_show_status();
        }
        delay_ms(5);
    }
}
