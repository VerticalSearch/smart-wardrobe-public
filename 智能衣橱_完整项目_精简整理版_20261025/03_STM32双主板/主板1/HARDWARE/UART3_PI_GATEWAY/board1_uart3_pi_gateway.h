#ifndef __BOARD1_UART3_PI_GATEWAY_H
#define __BOARD1_UART3_PI_GATEWAY_H

#include "sys.h"
#include <stdint.h>

#define BOARD1_PI_GATEWAY_FRAME_HEADER              0xFFU
#define BOARD1_PI_GATEWAY_CMD_CAPTURE               0x01U
#define BOARD1_PI_GATEWAY_CMD_RESULT                0x02U
#define BOARD1_PI_GATEWAY_CMD_IMAGE_INFO            0x03U
#define BOARD1_PI_GATEWAY_CMD_IMAGE_READY           0x04U
#define BOARD1_PI_GATEWAY_TX_FRAME_LEN              5U
#define BOARD1_PI_GATEWAY_STATUS_FRAME_LEN          5U
#define BOARD1_PI_GATEWAY_RESULT_FRAME_LEN          6U
#define BOARD1_PI_GATEWAY_IMAGE_INFO_FRAME_LEN      7U
#define BOARD1_PI_GATEWAY_RX_BUF_LEN                BOARD1_PI_GATEWAY_IMAGE_INFO_FRAME_LEN
#define BOARD1_PI_GATEWAY_ASCII_RX_BUF_LEN          192U
#define BOARD1_PI_GATEWAY_WIFI_MAX_APS              8U
#define BOARD1_PI_GATEWAY_WIFI_SSID_MAX_LEN         64U
#define BOARD1_PI_GATEWAY_WIFI_INFO_MAX_LEN         32U
#define BOARD1_PI_GATEWAY_COMMAND_ID_MAX_LEN        64U

#define BOARD1_PI_GATEWAY_STATUS_OK                 0x00U
#define BOARD1_PI_GATEWAY_STATUS_CAM_FAIL           0x01U
#define BOARD1_PI_GATEWAY_STATUS_WIFI_FAIL          0x02U
#define BOARD1_PI_GATEWAY_STATUS_UPLOAD_FAIL        0x03U
#define BOARD1_PI_GATEWAY_STATUS_NO_RESULT          0x04U
#define BOARD1_PI_GATEWAY_STATUS_BAD_FRAME          0xFCU
#define BOARD1_PI_GATEWAY_STATUS_TIMEOUT            0xFDU
#define BOARD1_PI_GATEWAY_STATUS_PENDING            0xFEU
#define BOARD1_PI_GATEWAY_STATUS_IDLE               0xFFU

#define BOARD1_PI_GATEWAY_STYLE_UNKNOWN             0x00U
#define BOARD1_PI_GATEWAY_STYLE_T_SHIRT             0x01U
#define BOARD1_PI_GATEWAY_STYLE_SHIRT               0x02U
#define BOARD1_PI_GATEWAY_STYLE_COAT                0x03U
#define BOARD1_PI_GATEWAY_STYLE_DRESS               0x04U
#define BOARD1_PI_GATEWAY_STYLE_PANTS               0x05U
#define BOARD1_PI_GATEWAY_STYLE_SKIRT               0x06U
#define BOARD1_PI_GATEWAY_STYLE_SHORTS              0x07U
#define BOARD1_PI_GATEWAY_STYLE_SWEATER             0x08U
#define BOARD1_PI_GATEWAY_STYLE_HOODIE              0x09U
#define BOARD1_PI_GATEWAY_STYLE_JACKET              0x0AU

#define BOARD1_PI_GATEWAY_RESPONSE_TIMEOUT_MS       120000U
#define BOARD1_PI_GATEWAY_IMAGE_MAX_SIZE            61440UL
#define BOARD1_PI_GATEWAY_IMAGE_TRANSFER_TIMEOUT_MS 20000U
#define BOARD1_PI_GATEWAY_WIFI_TIMEOUT_MS           30000U

#define BOARD1_PI_GATEWAY_WIFI_STATUS_IDLE          0U
#define BOARD1_PI_GATEWAY_WIFI_STATUS_SCANNING      1U
#define BOARD1_PI_GATEWAY_WIFI_STATUS_SCAN_DONE     2U
#define BOARD1_PI_GATEWAY_WIFI_STATUS_CONNECTING    3U
#define BOARD1_PI_GATEWAY_WIFI_STATUS_CONNECTED     4U
#define BOARD1_PI_GATEWAY_WIFI_STATUS_FAIL          5U

#define BOARD1_PI_GATEWAY_ACTION_FAN_MOVE           8U
#define BOARD1_PI_GATEWAY_ACTION_FAN_STOP           9U

typedef struct
{
    char ssid[BOARD1_PI_GATEWAY_WIFI_SSID_MAX_LEN];
    int16_t rssi;
    uint8_t auth;
    uint8_t valid;
} board1_pi_gateway_wifi_ap_t;

extern volatile uint8_t g_board1_pi_gateway_last_status;
extern volatile uint8_t g_board1_pi_gateway_last_style_id;
extern volatile uint8_t g_board1_pi_gateway_response_ready;
extern volatile uint8_t g_board1_pi_gateway_busy;
extern volatile uint8_t g_board1_pi_gateway_bad_frame_count;
extern volatile uint32_t g_board1_pi_gateway_image_size;
extern volatile uint32_t g_board1_pi_gateway_image_received;
extern volatile uint8_t g_board1_pi_gateway_image_info_ready;
extern volatile uint8_t g_board1_pi_gateway_image_done;
extern volatile uint8_t g_board1_pi_gateway_image_error;
extern volatile uint8_t g_board1_pi_gateway_wifi_scan_ready;
extern volatile uint8_t g_board1_pi_gateway_wifi_status_update;
extern volatile uint8_t g_board1_pi_gateway_wifi_status;
extern volatile uint8_t g_board1_pi_gateway_wifi_busy;
extern volatile uint32_t g_board1_pi_gateway_ascii_cmd_count;
extern volatile uint32_t g_board1_pi_gateway_motion_cmd_count;
extern volatile uint32_t g_board1_pi_gateway_servo_cmd_count;
extern volatile uint32_t g_board1_pi_gateway_action_cmd_count;
extern volatile uint32_t g_board1_pi_gateway_output_cmd_count;
extern volatile uint8_t g_board1_pi_gateway_last_ascii_cmd;
extern volatile uint8_t g_board1_pi_gateway_last_motion_cmd;
extern volatile uint8_t g_board1_pi_gateway_last_motion_channel;
extern volatile uint16_t g_board1_pi_gateway_last_motion_angle;
extern volatile uint8_t g_board1_pi_gateway_last_servo_channel;
extern volatile uint16_t g_board1_pi_gateway_last_servo_angle;
extern volatile uint8_t g_board1_pi_gateway_last_output_device;
extern volatile uint8_t g_board1_pi_gateway_last_output_enable;
extern volatile uint8_t g_board1_pi_gateway_debug_dirty;
extern char g_board1_pi_gateway_last_command_id[BOARD1_PI_GATEWAY_COMMAND_ID_MAX_LEN];

void board1_uart3_pi_gateway_init(uint32_t baud);
void board1_uart3_pi_gateway_send_byte(uint8_t byte);
void board1_uart3_pi_gateway_send(const uint8_t *data, uint16_t len);
void board1_uart3_pi_gateway_send_line(const char *line);
void board1_uart3_pi_gateway_action_service(uint32_t now_ms);
void board1_uart3_pi_gateway_debug_service(uint32_t now_ms);
void board1_uart3_pi_gateway_request_capture(uint32_t now_ms, uint8_t side_id);
void board1_uart3_pi_gateway_handle_board2_status(uint8_t action_id,
                                                  uint8_t status_code,
                                                  uint16_t detail);
void board1_uart3_pi_gateway_request_wifi_scan(uint32_t now_ms);
void board1_uart3_pi_gateway_connect_wifi(const char *ssid, const char *password, uint32_t now_ms);
void board1_uart3_pi_gateway_poll(uint32_t now_ms);
uint8_t board1_uart3_pi_gateway_is_busy(void);
uint8_t board1_uart3_pi_gateway_get_last_status(void);
uint8_t board1_uart3_pi_gateway_get_last_style_id(void);
uint8_t board1_uart3_pi_gateway_has_response(void);
void board1_uart3_pi_gateway_clear_response(void);
uint8_t board1_uart3_pi_gateway_has_image_info(void);
uint32_t board1_uart3_pi_gateway_get_image_size(void);
void board1_uart3_pi_gateway_clear_image_info(void);
void board1_uart3_pi_gateway_start_image_stream(uint32_t size);
uint8_t board1_uart3_pi_gateway_image_is_done(void);
uint8_t board1_uart3_pi_gateway_image_has_error(void);
uint8_t board1_uart3_pi_gateway_image_transfer_active(void);
void board1_uart3_pi_gateway_clear_image_state(void);
void board1_uart3_pi_gateway_send_image_ready(uint8_t ready);
uint8_t board1_uart3_pi_gateway_wifi_has_scan_result(void);
void board1_uart3_pi_gateway_wifi_clear_scan_result(void);
uint8_t board1_uart3_pi_gateway_wifi_has_status_update(void);
void board1_uart3_pi_gateway_wifi_clear_status_update(void);
uint8_t board1_uart3_pi_gateway_wifi_get_status(void);
uint8_t board1_uart3_pi_gateway_wifi_get_count(void);
const char *board1_uart3_pi_gateway_wifi_get_ssid(uint8_t index);
int16_t board1_uart3_pi_gateway_wifi_get_rssi(uint8_t index);
uint8_t board1_uart3_pi_gateway_wifi_get_auth(uint8_t index);
const char *board1_uart3_pi_gateway_wifi_get_info(void);

#endif
