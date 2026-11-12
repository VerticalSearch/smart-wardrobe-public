/*
******************************************************************************************************************
*Filename      :usart2_dma.h
*Programmer(s) :chu (修改为激光雷达驱动)
*Description   :USART2激光雷达驱动

							 USART2_RX---PD6
							 USART2_TX---PD5
******************************************************************************************************************
*/
#ifndef __USART2_DMA_H
#define __USART2_DMA_H	 
#include "sys.h"
#include "laser_common.h"

// DMA缓冲区大小定义（保留用于兼容性）
#define U2_DATA_LEN  60

//激光雷达处理后的各个参数
extern uint8_t Uart2_Receive;
extern uint16_t receive_cnt2;
extern uint8_t confidence2;
extern uint16_t distance2,noise2,reftof2;
extern uint32_t peak2,intg2;
extern LidarPointTypedef Pack2_Data[12];

/*
******************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
*                                                   串口2函数
******************************************************************************************************************
*/
void uart2_init(uint32_t bound);
void USART2_Init(u32 bound);  // 兼容性函数，调用uart2_init
void data2_process(void);  // 激光雷达数据处理

// 其他函数（保留用于兼容性）
extern void USART2_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize);
extern void DMA1_S1_Reset(void);

#endif
