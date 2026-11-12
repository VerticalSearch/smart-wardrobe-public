#ifndef __KEY_H
#define __KEY_H

#include "sys.h"

/* 按键IO定义（低电平按下） */
#define KEY1        GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4)  /* PE4 */
#define KEY2        GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5)  /* PE5 */
#define KEY3        GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_6)  /* PE6 */
#define KEY4        GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) /* PC13 */

#define KEY1_PRES   1U
#define KEY2_PRES   2U
#define KEY3_PRES   3U
#define KEY4_PRES   4U

void KEY_Init(void);
u8 KEY_Scan(u8 mode);
void mode_set(void);

#endif
