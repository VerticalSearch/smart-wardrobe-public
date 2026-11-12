/*
******************************************************************************
*Filename      :usart3dma.h
*Programmer(s) :基于F103 STP-23激光雷达代码移植到F407VGT6
*Description   :F407VGT6串口3 STP-23激光雷达驱动
*Date         :2024
******************************************************************************
*/

#ifndef __USART3_DMA_H
#define __USART3_DMA_H	 

#include "sys.h"
#include "stm32f4xx.h"
#include <stdint.h>
#include "laser_common.h"

// 外部变量声明
extern uint8_t Uart3_Receive;
extern uint16_t receive_cnt3;
extern uint8_t confidence3;
extern uint16_t distance3,noise3,reftof3;
extern uint32_t peak3,intg3;
extern LidarPointTypedef Pack3_Data[12];

/*
******************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
*                                                   串口3函数
******************************************************************************************************************
*/

void uart3_init(uint32_t bound);
void USART3_IRQHandler(void);
void data3_process(void);
void uart3_send_byte(uint8_t data);

#endif

