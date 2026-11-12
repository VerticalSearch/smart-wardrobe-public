#ifndef __USART_DMA_H
#define __USART_DMA_H	 
#include "sys.h"

// 通信协议定义
#define UART1_DATA_LEN  10
#define CMD_HEADER      0xFF
#define CMD_GET_SENSOR  0x01
#define CMD_SET_MODE    0x02
#define CMD_EMERGENCY   0x03
#define CMD_STEER_CTRL  0x04  // 舵机控制命令
#define CMD_STEER_TURN  0x05  // 舵机转向命令（自动执行所有舵机）
#define CMD_SET_DIRECTION  0x06  // 设置运动方向：0=前向, 1=右行, 2=左行, 3=倒退
#define CMD_STATE_SWITCH_1 0x07  // 状态切换1：控制舵机1和2转到预设角度1
#define CMD_STATE_SWITCH_2 0x08  // 状态切换2：控制舵机1和2转到预设角度2
#define CMD_STATE_SWITCH_3 0x09  // 状态切换3：控制舵机1和2转到预设角度3
// 注释：确保此定义被正确编译

// 主控接收的传感器数据结构（从从控接收，仅包含陀螺仪数据）
typedef struct {
    int16_t  yaw;               // 陀螺仪偏航角（度*10）
} SensorData_t;

// 命令数据结构
typedef struct {
    uint8_t header;            // 帧头 0xFF
    uint8_t cmd_type;          // 命令类型
    uint8_t param1;            // 参数1
    uint8_t param2;            // 参数2
    uint8_t checksum;          // 校验和
} Command_t;

// 外部变量声明
extern u8 rec_data_u1[UART1_DATA_LEN];   // 接收缓冲区
extern u8 send_data_u1[UART1_DATA_LEN];  // 发送缓冲区
extern SensorData_t sensor_data;         // 传感器数据
extern uint8_t cmd_received_flag;        // 命令接收标志

// 函数声明
void USART1_Init(int Baud);
void USART1_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize);
void DMA2_S5_Reset(void);
void USART_Senddatas(USART_TypeDef* USARTxx, u8* addr, int size);
void Master_SendCommand(uint8_t cmd_type, uint8_t param1, uint8_t param2);
void Master_ProcessReceivedData(void);
uint8_t CalculateChecksum(uint8_t* data, uint8_t len);

#endif
