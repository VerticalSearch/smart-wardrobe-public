#ifndef __USART_DMA_H
#define __USART_DMA_H

#include "sys.h"
#include <stdint.h>

/* UART1 DMA buffer length for board-to-board packets */
#define UART1_DATA_LEN 16U

/* Board-to-board command protocol */
#define CMD_HEADER             0xFF
#define CMD_SET_MODE           0x02
#define CMD_EMERGENCY          0x03
#define CMD_K230_CAPTURE       0x10
#define CMD_PHOTO_FILL_LIGHT   0x11
#define CMD_ACTION_STATUS      0x12
#define CMD_ACTION_START       0x13
#define CMD_STEPPER_ENABLE     0x30
#define CMD_STEPPER_MOVE_ANGLE 0x31
#define CMD_STEPPER_GOTO_ANGLE 0x32
#define CMD_SERVO_SET_ANGLE    0x40

#define BOARD_LINK_CMD_FRAME_LEN        7U
#define BOARD_LINK_DIR_FORWARD          0U
#define BOARD_LINK_DIR_REVERSE          1U
#define BOARD_ACTION_STATUS_RUNNING     1U
#define BOARD_ACTION_STATUS_DONE        2U
#define BOARD_ACTION_STATUS_STOPPED     3U
#define BOARD_ACTION_STATUS_ERROR       4U
#define BOARD_ACTION_STATUS_BUSY        5U

extern volatile uint32_t g_board2_servo_uart6_rx_count;
extern volatile uint8_t g_board2_servo_uart6_last_byte;
extern volatile uint32_t g_board2_servo_cmd_count;
extern volatile int g_board2_servo_ch1_angle_deg;
extern volatile int g_board2_servo_ch2_angle_deg;
extern volatile int g_board2_servo_ch3_angle_deg;
extern volatile int g_board2_servo_ch4_angle_deg;
extern volatile uint32_t g_board2_link_rx_count;
extern volatile uint32_t g_board2_link_cmd_count;
extern volatile uint8_t g_board2_link_last_cmd;
extern volatile uint8_t g_board2_link_last_channel;
extern volatile uint16_t g_board2_link_last_angle;
extern volatile int16_t g_board2_link_last_target;

/* 7-byte command frame shared with board1 */
typedef struct
{
    uint8_t header;
    uint8_t cmd_type;
    uint8_t channel;
    uint8_t control;
    uint16_t angle_deg;
    uint8_t checksum;
} Board2CommandFrame_t;

void USART1_Init(int baud);
void USART6_DigitRx_Init(int baud);
void USART1_DMATransfer(uint32_t *buffer_src, uint32_t buffer_size);
void DMA2_S5_Reset(void);
void Board2_ProcessReceivedCommand(void);
void Board2_ProcessServoUart6Command(void);
void Board2_LinkTaskStatusService(void);
void Board2_LinkRequestK230Capture(uint8_t side_id);
void Board2_LinkSetPhotoFillLight(uint8_t enable);
void Board2_RunBrushCleanMotion(void);
uint8_t CalculateChecksum(uint8_t *data, uint8_t len);

#endif
