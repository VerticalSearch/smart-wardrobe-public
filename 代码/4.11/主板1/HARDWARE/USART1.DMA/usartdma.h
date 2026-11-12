#ifndef __USART_DMA_H
#define __USART_DMA_H	 
#include "sys.h"

// Board1-to-board2 link: board1 uses USART2 on CN5.
// Board1 USART1 is wired to onboard U5, not to an external connector.
#define UART1_DATA_LEN                  16U

#define CMD_HEADER                      0xFFU
#define CMD_SET_MODE                    0x02U
#define CMD_EMERGENCY                   0x03U
#define CMD_PI_GATEWAY_CAPTURE          0x10U
#define CMD_PHOTO_FILL_LIGHT            0x11U
#define CMD_ACTION_STATUS               0x12U
#define CMD_ACTION_START                0x13U
#define CMD_STEPPER_ENABLE              0x30U
#define CMD_STEPPER_MOVE_ANGLE          0x31U
#define CMD_STEPPER_GOTO_ANGLE          0x32U
#define CMD_SERVO_SET_ANGLE             0x40U

#define BOARD_LINK_CMD_FRAME_LEN        7U
#define BOARD_LINK_DIR_FORWARD          0U
#define BOARD_LINK_DIR_REVERSE          1U
#define BOARD_ACTION_STATUS_RUNNING     1U
#define BOARD_ACTION_STATUS_DONE        2U
#define BOARD_ACTION_STATUS_STOPPED     3U
#define BOARD_ACTION_STATUS_ERROR       4U
#define BOARD_ACTION_STATUS_BUSY        5U
#define BOARD1_LINK_STEPPER_CHANNEL_MIN 1U
#define BOARD1_LINK_STEPPER_CHANNEL_MAX 5U

// 7-byte command frame:
// header, cmd, channel, control/dir, angle_hi, angle_lo, checksum.
typedef struct {
    uint8_t header;            // Frame header 0xFF
    uint8_t cmd_type;          // Command type
    uint8_t channel;           // Board2 stepper channel 1-5; 4=rod, 5=brush
    uint8_t control;           // Enable value or direction
    uint16_t angle_deg;        // Angle in degrees
    uint8_t checksum;          // Checksum
} Command_t;

// External variables
extern u8 rec_data_u1[UART1_DATA_LEN];   // RX buffer
extern u8 send_data_u1[UART1_DATA_LEN];  // TX buffer
extern volatile uint32_t g_board1_link_tx_queue_count;
extern volatile uint32_t g_board1_link_tx_sent_count;
extern volatile uint32_t g_board1_link_tx_drop_count;
extern volatile uint8_t g_board1_link_last_tx_cmd;
extern volatile uint8_t g_board1_link_last_tx_channel;
extern volatile uint16_t g_board1_link_last_tx_angle;
extern volatile int16_t g_board1_link_last_tx_target;

// Function declarations
void Board1_LinkUart_Init(int Baud);
void Board1_LinkUart_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize);
void Board1_LinkService(void);
void Board1_LinkRxDma_Reset(void);
void USART1_Init(int Baud);
void USART1_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize);
void DMA2_S5_Reset(void);
void USART_Senddatas(USART_TypeDef* USARTxx, u8* addr, int size);
void Master_SendCommand(uint8_t cmd_type, uint8_t param1, uint8_t param2);
void Board1_LinkSetStepperEnable(uint8_t channel, uint8_t enable);
void Board1_LinkMoveStepperAngle(uint8_t channel,
                                 uint8_t dir,
                                 uint16_t angle_deg);
void Board1_LinkGotoStepperAngle(uint8_t channel, int16_t target_deg);
void Board1_LinkEmergencyStop(void);
void Board1_LinkSetServoAngle(uint8_t channel, uint16_t angle_deg);
uint8_t Board1_LinkStartAction(uint8_t action_id, uint8_t target_c);
uint8_t CalculateChecksum(uint8_t* data, uint8_t len);

#endif
