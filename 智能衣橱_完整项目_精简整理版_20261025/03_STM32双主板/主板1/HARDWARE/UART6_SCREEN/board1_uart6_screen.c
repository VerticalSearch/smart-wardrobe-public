#include "board1_uart6_screen.h"
#include "board1_uart3_pi_gateway.h"
#include "control.h"
#include <stdlib.h>
#include <string.h>

#define BOARD1_SCREEN_RX_BUF_LEN                 8U
#define BOARD1_SCREEN_ASCII_RX_BUF_LEN           128U
#define BOARD1_SCREEN_X5_READY_RET               0xFEU
#define BOARD1_SCREEN_X5_DONE_RET                0xFDU

static volatile uint8_t g_screen_rx_buf[BOARD1_SCREEN_RX_BUF_LEN];
static volatile uint8_t g_screen_rx_len = 0U;
static char g_screen_ascii_rx_buf[BOARD1_SCREEN_ASCII_RX_BUF_LEN];
static volatile uint16_t g_screen_ascii_rx_len = 0U;
static volatile uint8_t g_screen_x5_file_ready = 0U;
static volatile uint8_t g_screen_x5_file_done = 0U;
static volatile uint8_t g_screen_x5_error = 0U;
static volatile uint8_t g_screen_event = BOARD1_SCREEN_EVENT_NONE;
static volatile uint8_t g_screen_wifi_selected_index = 0U;
static char g_screen_wifi_password[BOARD1_SCREEN_WIFI_PASSWORD_MAX_LEN];

static void board1_screen_copy_text(char *dst, uint16_t dst_len, const char *src)
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
            if ((ch == '\r') || (ch == '\n'))
            {
                ch = ' ';
            }
            dst[i] = ch;
            i++;
        }
    }

    dst[i] = '\0';
}

static void board1_screen_append_u32(char *buf, uint8_t *idx, uint32_t value)
{
    char tmp[10];
    uint8_t len = 0U;

    if (value == 0U)
    {
        buf[*idx] = '0';
        *idx = (uint8_t)(*idx + 1U);
        return;
    }

    while ((value != 0U) && (len < sizeof(tmp)))
    {
        tmp[len] = (char)('0' + (value % 10U));
        value /= 10U;
        len++;
    }

    while (len != 0U)
    {
        len--;
        buf[*idx] = tmp[len];
        *idx = (uint8_t)(*idx + 1U);
    }
}

static void board1_screen_reset_rx_state(void)
{
    g_screen_rx_len = 0U;
    g_screen_ascii_rx_len = 0U;
    g_screen_x5_file_ready = 0U;
    g_screen_x5_file_done = 0U;
    g_screen_x5_error = 0U;
}

static void board1_screen_handle_ascii_line(char *line)
{
    if (line == 0)
    {
        return;
    }

    if (strcmp(line, "@WSCAN") == 0)
    {
        g_screen_event = BOARD1_SCREEN_EVENT_WIFI_SCAN;
        return;
    }

    if (strncmp(line, "@WSEL|", 6U) == 0)
    {
        int index = atoi(line + 6U);
        if (index < 0)
        {
            index = 0;
        }
        if (index >= BOARD1_PI_GATEWAY_WIFI_MAX_APS)
        {
            index = BOARD1_PI_GATEWAY_WIFI_MAX_APS - 1;
        }
        g_screen_wifi_selected_index = (uint8_t)index;
        g_screen_event = BOARD1_SCREEN_EVENT_WIFI_SELECT;
        return;
    }

    if (strncmp(line, "@WCONN|", 7U) == 0)
    {
        board1_screen_copy_text(g_screen_wifi_password,
                                BOARD1_SCREEN_WIFI_PASSWORD_MAX_LEN,
                                line + 7U);
        g_screen_event = BOARD1_SCREEN_EVENT_WIFI_CONNECT;
        return;
    }
}

static void board1_screen_handle_response(uint8_t code)
{
    if (code == BOARD1_SCREEN_X5_READY_RET)
    {
        g_screen_x5_file_ready = 1U;
    }
    else if (code == BOARD1_SCREEN_X5_DONE_RET)
    {
        g_screen_x5_file_done = 1U;
    }
    else if (code == 0x01U)
    {
        return;
    }
    else
    {
        g_screen_x5_error = code;
    }
}

static void board1_screen_handle_byte(uint8_t byte)
{
    uint8_t i;

    if ((g_screen_ascii_rx_len != 0U) ||
        ((g_screen_rx_len == 0U) && (byte == '@')))
    {
        if (byte == '\r')
        {
            return;
        }

        if (byte == '\n')
        {
            g_screen_ascii_rx_buf[g_screen_ascii_rx_len] = '\0';
            board1_screen_handle_ascii_line(g_screen_ascii_rx_buf);
            g_screen_ascii_rx_len = 0U;
            return;
        }

        if (g_screen_ascii_rx_len < (uint16_t)(BOARD1_SCREEN_ASCII_RX_BUF_LEN - 1U))
        {
            g_screen_ascii_rx_buf[g_screen_ascii_rx_len] = (char)byte;
            g_screen_ascii_rx_len++;
        }
        else
        {
            g_screen_ascii_rx_len = 0U;
        }
        return;
    }

    if (g_screen_rx_len < BOARD1_SCREEN_RX_BUF_LEN)
    {
        g_screen_rx_buf[g_screen_rx_len] = byte;
        g_screen_rx_len++;
    }

    if (g_screen_rx_len >= 4U)
    {
        if ((g_screen_rx_buf[g_screen_rx_len - 3U] == 0xFFU) &&
            (g_screen_rx_buf[g_screen_rx_len - 2U] == 0xFFU) &&
            (g_screen_rx_buf[g_screen_rx_len - 1U] == 0xFFU))
        {
            board1_screen_handle_response(g_screen_rx_buf[g_screen_rx_len - 4U]);
            g_screen_rx_len = 0U;
            return;
        }
    }

    if (g_screen_rx_len >= BOARD1_SCREEN_RX_BUF_LEN)
    {
        for (i = 1U; i < BOARD1_SCREEN_RX_BUF_LEN; i++)
        {
            g_screen_rx_buf[i - 1U] = g_screen_rx_buf[i];
        }
        g_screen_rx_len = (uint8_t)(BOARD1_SCREEN_RX_BUF_LEN - 1U);
    }
}

void board1_uart6_screen_init(uint32_t baud)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio);

    usart.USART_BaudRate = baud;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART6, &usart);

    nvic.NVIC_IRQChannel = USART6_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 3U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART6, ENABLE);

    board1_screen_reset_rx_state();
    g_screen_event = BOARD1_SCREEN_EVENT_NONE;
    g_screen_wifi_selected_index = 0U;
    g_screen_wifi_password[0] = '\0';
}

void board1_uart6_screen_send_byte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART6, USART_FLAG_TXE) == RESET) {}
    USART_SendData(USART6, byte);
}

void board1_uart6_screen_send(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    for (i = 0U; i < len; i++)
    {
        board1_uart6_screen_send_byte(data[i]);
    }

    while (USART_GetFlagStatus(USART6, USART_FLAG_TC) == RESET) {}
}

void board1_uart6_screen_send_cmd(const char *cmd)
{
    while (*cmd != '\0')
    {
        board1_uart6_screen_send_byte((uint8_t)(*cmd));
        cmd++;
    }

    board1_uart6_screen_send_byte(0xFFU);
    board1_uart6_screen_send_byte(0xFFU);
    board1_uart6_screen_send_byte(0xFFU);
    while (USART_GetFlagStatus(USART6, USART_FLAG_TC) == RESET) {}
}

void board1_uart6_screen_set_text(const char *component, const char *text)
{
    char cmd[128];
    uint16_t idx = 0U;

    if ((component == 0) || (text == 0))
    {
        return;
    }

    while ((*component != '\0') && (idx < 32U))
    {
        cmd[idx++] = *component;
        component++;
    }
    cmd[idx++] = '.';
    cmd[idx++] = 't';
    cmd[idx++] = 'x';
    cmd[idx++] = 't';
    cmd[idx++] = '=';
    cmd[idx++] = '"';

    while ((*text != '\0') && (idx < (uint16_t)(sizeof(cmd) - 3U)))
    {
        char ch = *text;
        if ((ch == '"') || (ch == '\r') || (ch == '\n'))
        {
            ch = ' ';
        }
        cmd[idx++] = ch;
        text++;
    }

    cmd[idx++] = '"';
    cmd[idx] = '\0';
    board1_uart6_screen_send_cmd(cmd);
}

uint8_t board1_uart6_screen_has_event(void)
{
    return (g_screen_event != BOARD1_SCREEN_EVENT_NONE) ? 1U : 0U;
}

uint8_t board1_uart6_screen_get_event(void)
{
    return g_screen_event;
}

void board1_uart6_screen_clear_event(void)
{
    g_screen_event = BOARD1_SCREEN_EVENT_NONE;
}

uint8_t board1_uart6_screen_get_selected_wifi_index(void)
{
    return g_screen_wifi_selected_index;
}

const char *board1_uart6_screen_get_wifi_password(void)
{
    return g_screen_wifi_password;
}

uint8_t board1_uart6_screen_x5_start_file_write(const char *path,
                                                uint32_t size,
                                                uint32_t timeout_ms)
{
    char cmd[64];
    uint8_t idx = 0U;
    uint32_t start_tick;

    if ((path == 0) || (size == 0U))
    {
        return 0U;
    }

    board1_screen_reset_rx_state();

    cmd[idx++] = 't';
    cmd[idx++] = 'w';
    cmd[idx++] = 'f';
    cmd[idx++] = 'i';
    cmd[idx++] = 'l';
    cmd[idx++] = 'e';
    cmd[idx++] = ' ';
    cmd[idx++] = '"';
    while ((*path != '\0') && (idx < 48U))
    {
        cmd[idx++] = *path;
        path++;
    }
    cmd[idx++] = '"';
    cmd[idx++] = ',';
    board1_screen_append_u32(cmd, &idx, size);
    cmd[idx] = '\0';

    board1_uart6_screen_send_cmd(cmd);

    start_tick = g_board1_tick_ms;
    while ((uint32_t)(g_board1_tick_ms - start_tick) < timeout_ms)
    {
        if (g_screen_x5_file_ready != 0U)
        {
            return 1U;
        }
        if (g_screen_x5_error != 0U)
        {
            return 0U;
        }
    }

    return 0U;
}

uint8_t board1_uart6_screen_x5_wait_file_done(uint32_t timeout_ms)
{
    uint32_t start_tick = g_board1_tick_ms;

    while ((uint32_t)(g_board1_tick_ms - start_tick) < timeout_ms)
    {
        if (g_screen_x5_file_done != 0U)
        {
            return 1U;
        }
        if (g_screen_x5_error != 0U)
        {
            return 0U;
        }
    }

    return 0U;
}

void board1_uart6_screen_x5_show_external_image(const char *component,
                                                const char *path)
{
    char cmd[64];
    uint8_t idx = 0U;

    if ((component == 0) || (path == 0))
    {
        return;
    }

    while ((*component != '\0') && (idx < 20U))
    {
        cmd[idx++] = *component;
        component++;
    }
    cmd[idx++] = '.';
    cmd[idx++] = 'p';
    cmd[idx++] = 'a';
    cmd[idx++] = 't';
    cmd[idx++] = 'h';
    cmd[idx++] = '=';
    cmd[idx++] = '"';
    while ((*path != '\0') && (idx < 60U))
    {
        cmd[idx++] = *path;
        path++;
    }
    cmd[idx++] = '"';
    cmd[idx] = '\0';

    board1_uart6_screen_send_cmd(cmd);
}

void USART6_IRQHandler(void)
{
    if (USART_GetITStatus(USART6, USART_IT_RXNE) != RESET)
    {
        uint8_t byte = (uint8_t)USART_ReceiveData(USART6);

        USART_ClearITPendingBit(USART6, USART_IT_RXNE);
        board1_screen_handle_byte(byte);
    }
}
