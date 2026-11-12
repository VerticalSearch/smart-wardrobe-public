#ifndef __MOTOR_H
#define __MOTOR_H	  
#include "sys.h"

void motor_Gpio_Init(void);
void Motor_pwm_Init(u32 arr,u32 psc);
void MotorA_Ctrl(int Compare);
void MotorB_Ctrl(int Compare);
void MotorC_Ctrl(int Compare);
void MotorD_Ctrl(int Compare);
void control_all (int a,int b,int c, int d);

#endif



