#ifndef  _ENCODER_H
#define  _ENCODER_H

#include "sys.h"

// 编码器配置定义
#define ENCODER_TIM_PERIOD (u16)(65535)   // 编码器计数器周期值
#define ENC_INIT 30000                    // 编码器初始值

// 编码器数据结构
typedef struct
{
	int EncoderA;  // 编码器A (TIM2 - PA0,PA1)
	int EncoderB;  // 编码器B (TIM3 - PA6,PA7) 
	int EncoderC;  // 编码器C (TIM4 - PD12,PD13)
	int EncoderD;  // 编码器D (TIM8 - PC6,PC7)
}EncoderStru;

// 外部变量声明
extern int Encoder_Length_L1,Encoder_Length_R1,Encoder_Length_L2,Encoder_Length_R2;
extern int Encoder_Real_L1,Encoder_Real_L2,Encoder_Real_R1,Encoder_Real_R2;

// 编码器速度变量 (单位: cm/s)
extern float enc_speed_1,enc_speed_2,enc_speed_3,enc_speed_4;

// 编码器累计距离变量
extern int enc_1,enc_2,enc_3,enc_4;

// 函数声明
void Timer6_Init(void);
void Read_Encoder(void);
void Encoder_Real(void);

// 编码器初始化函数
void TIM2_Encoder_Init(u16 arr,u16 psc);  // 编码器A - PA0,PA1
void TIM3_Encoder_Init(u16 arr,u16 psc);  // 编码器B - PA6,PA7
void TIM4_Encoder_Init(u16 arr,u16 psc);  // 编码器C - PD12,PD13
void TIM8_Encoder_Init(u16 arr,u16 psc);  // 编码器D - PC6,PC7

// 编码器读取函数
int Read_EncoderL1(void);  // 读取编码器A
int Read_EncoderL2(void);  // 读取编码器B
int Read_EncoderR1(void);  // 读取编码器C
int Read_EncoderR2(void);  // 读取编码器D

// 编码器速度计算函数
void Encoder_Speed_Calculate(void);  // 计算编码器速度

#endif

