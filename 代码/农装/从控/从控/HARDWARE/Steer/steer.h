#ifndef __STEER_H
#define __STEER_H

#include "sys.h"

#include "stm32f4xx.h"                  // Device header

// 270度舵机PWM和角度转换的宏定义
#define STEER_MIN_PWM     500     // 最小PWM值 (对应0度)
#define STEER_MAX_PWM     2500    // 最大PWM值 (对应270度)
#define STEER_MID_PWM     1500    // 中间PWM值 (对应135度)
#define STEER_MIN_ANGLE   0       // 最小角度
#define STEER_MAX_ANGLE   270     // 最大角度
#define STEER_MID_ANGLE   135     // 中间角度

// 函数声明
void TIM1_PWM_Init(u16 arr, u16 psc);  // TIM1定时器初始化，用于舵机1、2、3、4
void TIM3_PWM_Init(u16 arr, u16 psc);  // TIM3定时器初始化，用于舵机5、6、7、8

// 舵机1、2、3、4控制函数（使用TIM1）
void Steer1_4_PWM_Control(u8 which, int PWM);  // which: 1-4
void ALL_Steer1_4_PWM_Control(int PWM1, int PWM2, int PWM3, int PWM4);  // 统一控制舵机1-4
void Steer1_4_Init_Position(void);  // 初始化舵机1-4到中间位置

// 舵机5、6、7、8控制函数（使用TIM3）
void Steer5_8_PWM_Control(u8 which, int PWM);  // which: 5-8
void ALL_Steer5_8_PWM_Control(int PWM5, int PWM6, int PWM7, int PWM8);  // 统一控制舵机5-8
void Steer5_8_Init_Position(void);  // 初始化舵机5-8到中间位置

// 270度舵机核心函数（简化版）
int PWM_To_Angle(u16 pwm);                      // PWM转角度
void Steer_Angle_Control(u8 which, int angle);  // 单个舵机角度控制

// 舵机初始角度常量（用于陀螺仪修正）
extern int Steer1_init, Steer2_init, Steer3_init, Steer4_init;


#endif


