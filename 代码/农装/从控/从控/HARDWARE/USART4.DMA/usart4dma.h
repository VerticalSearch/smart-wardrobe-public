/*
******************************************************************************
*Filename      :usart4dma.h
*Programmer(s) :基于F103 STP-23激光雷达代码移植到F407VGT6
*Description   :F407VGT6串口4 STP-23激光雷达驱动
*Date         :2024
******************************************************************************
*/

#ifndef __USART4_DMA_H
#define __USART4_DMA_H	 

#include "sys.h"
#include "stm32f4xx.h"
#include <stdint.h>
#include "laser_common.h"

// 外部变量声明
extern uint8_t Uart4_Receive;
extern uint16_t receive_cnt4;
extern uint16_t distance4;
extern LiDARFrameTypeDef Pack4_Data;

/*
******************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
*                                                   串口4函数
******************************************************************************************************************
*/

void uart4_init(uint32_t bound);
void UART4_IRQHandler(void);
void data4_process(void);

#endif

