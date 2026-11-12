#ifndef __TIMER_H
#define __TIMER_H			

#include "stm32f4xx.h"

#define TIM1_SERVO_PERIOD_COUNT    20000U
#define TIM1_SERVO_TIM1_PSC          168U
#define TIM1_SERVO_TIM4_PSC           84U
#define TIM1_SERVO_TIM3_PSC           84U
#define TIM1_SERVO_MIN_PWM_COUNT     500U
#define TIM1_SERVO_MID_PWM_COUNT    1500U
#define TIM1_SERVO_MAX_PWM_COUNT    2500U
#define TIM1_SERVO_MIN_ANGLE_DEG        0U
#define TIM1_SERVO_MID_ANGLE_DEG      135U
#define TIM1_SERVO_MAX_ANGLE_DEG      270U
#define TIM1_SERVO_STEP_ANGLE_DEG       5U
#define TIM1_SERVO1_INIT_ANGLE_DEG     190U   /* 135 + 16*5 */
#define TIM1_SERVO2_INIT_ANGLE_DEG    170U   /* 135 - 7*5 */
#define TIM1_SERVO3_INIT_ANGLE_DEG    250U   /* PWME，对应舵机3 */
#define TIM1_SERVO4_INIT_ANGLE_DEG    0U   /* PWMF，对应舵机4 */
#define TIM1_SERVO_CH1                 1U
#define TIM1_SERVO_CH2                 2U
#define TIM1_SERVO_CH3                 3U
#define TIM1_SERVO_CH4                 4U

void TIM7_Cnt_Init(u16 arr,u32 psc);

void TIM1_Servo_PWM_Init(void);
void TIM1_Servo_SetPwmCount(u8 channel, u16 pwm_count);
void TIM1_Servo_SetPulseUs(u8 channel, u16 pulse_us);
void TIM1_Servo_SetAngle(u8 channel, int angle);



#endif
