#ifndef __USART5_DMA_H
#define __USART5_DMA_H	 
#include "sys.h"

// OPENMV数据接收配置
#define UART5_DATA_LEN  50
#define OPENMV_HEADER   'X'
#define OPENMV_BAUDRATE 115200

// OPENMV数据结构
typedef struct {
    uint16_t x_coordinate;    // X坐标 (0-319)
    uint16_t y_coordinate;    // Y坐标 (0-239) 
    uint16_t area;            // 面积
    uint8_t  data_valid;      // 数据有效标志
    uint32_t timestamp;       // 时间戳
} OpenMVData_t;

// 外部变量声明
extern u8 rec_data_u5[UART5_DATA_LEN];   // 接收缓冲区
extern u8 send_data_u5[UART5_DATA_LEN];  // 发送缓冲区
extern OpenMVData_t openmv_data;         // OPENMV数据
extern uint8_t openmv_data_ready;        // 数据就绪标志

// 函数声明
void USART5_Init(int Baud);
void USART5_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize);
void DMA1_S2_Reset(void);
void USART_Senddatas(USART_TypeDef* USARTxx, u8* addr, int size);
void OpenMV_ProcessReceivedData(void);
void OpenMV_ResetData(void);
uint8_t OpenMV_ParseData(uint8_t* buffer, uint8_t len);

#endif

