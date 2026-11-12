#ifndef __BOARD1_UART6_SCREEN_H
#define __BOARD1_UART6_SCREEN_H

#include "sys.h"
#include <stdint.h>

#define BOARD1_SCREEN_X5_IMAGE_PATH              "ram/cloth.jpg"
#define BOARD1_SCREEN_X5_IMAGE_COMPONENT         "pic0"
#define BOARD1_SCREEN_X5_READY_TIMEOUT_MS        5000U
#define BOARD1_SCREEN_X5_DONE_TIMEOUT_MS         5000U
#define BOARD1_SCREEN_WIFI_PASSWORD_MAX_LEN      64U

#define BOARD1_SCREEN_EVENT_NONE                 0U
#define BOARD1_SCREEN_EVENT_WIFI_SCAN            1U
#define BOARD1_SCREEN_EVENT_WIFI_SELECT          2U
#define BOARD1_SCREEN_EVENT_WIFI_CONNECT         3U

void board1_uart6_screen_init(uint32_t baud);
void board1_uart6_screen_send_byte(uint8_t byte);
void board1_uart6_screen_send(const uint8_t *data, uint16_t len);
void board1_uart6_screen_send_cmd(const char *cmd);
void board1_uart6_screen_set_text(const char *component, const char *text);
uint8_t board1_uart6_screen_has_event(void);
uint8_t board1_uart6_screen_get_event(void);
void board1_uart6_screen_clear_event(void);
uint8_t board1_uart6_screen_get_selected_wifi_index(void);
const char *board1_uart6_screen_get_wifi_password(void);
uint8_t board1_uart6_screen_x5_start_file_write(const char *path,
                                                uint32_t size,
                                                uint32_t timeout_ms);
uint8_t board1_uart6_screen_x5_wait_file_done(uint32_t timeout_ms);
void board1_uart6_screen_x5_show_external_image(const char *component,
                                                const char *path);

#endif
