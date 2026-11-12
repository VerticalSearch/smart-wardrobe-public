#ifndef __USART_DMA_H
#define __USART_DMA_H	 
#include "sys.h"

// 通信协议定义
#define UART1_DATA_LEN  10
#define CMD_HEADER      0xFF
#define CMD_GET_SENSOR     0x01
#define CMD_SET_MODE       0x02
#define CMD_EMERGENCY      0x03
#define CMD_STEER_CTRL     0x04  // 舵机控制命令
#define CMD_STEER_TURN     0x05  // 舵机转向命令（自动执行所有舵机）
#define CMD_SET_DIRECTION  0x06  // 设置运动方向：0=前向, 1=右行, 2=左行, 3=倒退
#define CMD_STATE_SWITCH_1 0x07  // 状态切换1：控制舵机1和2转到预设角度1
#define CMD_STATE_SWITCH_2 0x08  // 状态切换2：控制舵机1和2转到预设角度2
#define CMD_STATE_SWITCH_3 0x09  // 状态切换3：控制舵机1和2转到预设角度3

// 从控传感器数据结构（仅包含陀螺仪数据）
typedef struct {
    int16_t  yaw;               // 陀螺仪偏航角（度*10，例如：15.5度 = 155）
} SlaveSensorData_t;

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
extern Command_t received_cmd;            // 接收到的命令
extern SlaveSensorData_t slave_sensor_data; // 从控传感器数据
extern uint8_t cmd_received_flag;        // 命令接收标志
extern uint16_t state_switch_delay0_counter;  // 状态切换延迟0计数器（收到指令->状态1，TIM7中断中递减）
extern uint16_t state_switch_delay1_counter;  // 状态切换延迟1计数器（状态1->状态2，TIM7中断中递减）
extern uint16_t state_switch_delay2_counter;  // 状态切换延迟2计数器（状态2->状态3，TIM7中断中递减）
extern uint16_t state_switch_delay3_counter;  // 状态切换延迟3计数器（状态3->状态4，TIM7中断中递减）
extern uint16_t state_switch_delay4_counter;  // 状态切换延迟4计数器（状态4->状态5，TIM7中断中递减）
extern uint8_t state_switch_delay0_timeout;   // 状态切换延迟0超时标志
extern uint8_t state_switch_delay1_timeout;   // 状态切换延迟1超时标志
extern uint8_t state_switch_delay2_timeout;   // 状态切换延迟2超时标志
extern uint8_t state_switch_delay3_timeout;   // 状态切换延迟3超时标志
extern uint8_t state_switch_delay4_timeout;   // 状态切换延迟4超时标志
extern uint8_t action_group_executing;        // 作业动作组执行标志：1=正在执行，禁止Steer_TurnRight/Left控制舵机3和4
extern uint8_t keep_steer34_state1;          // 保持舵机3和4在状态1的标志：1=在整个跨垄过程中保持状态1，直到跨垄结束
extern uint8_t is_fifth_backward;            // 第五次后退标志：1=第五次后退，使用特殊的舵机控制函数
extern uint16_t left3_steer34_delay_counter; // 第三次跨左垄舵机3和4延时计数器（TIM7中断中递减，1000ms）
extern uint8_t skip_state5_flag;              // 跳过状态5标志：1=第三次右跨垄，只执行状态1-4，不执行状态5

// 函数声明
void USART1_Init(int Baud);
void USART1_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize);
void DMA2_S5_Reset(void);
void USART_Senddatas(USART_TypeDef* USARTxx, u8* addr, int size);
void Slave_SendSensorData(void);
void Slave_ProcessReceivedCommand(void);
void Slave_CheckStateSwitchDelay(void);  // 检查状态切换延迟（非阻塞）
void Slave_UpdateSensorData(void);
uint8_t CalculateChecksum(uint8_t* data, uint8_t len);

#endif


