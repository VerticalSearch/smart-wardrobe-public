#include "stm32f4xx.h"                  // Device header
#include "timer.h"


void TIM7_Cnt_Init(u16 arr,u32 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/***********************Clock initial****************************************/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE); //??TIM5??
	
	/***********************Timer initial****************************************/	 
  TIM_TimeBaseInitStructure.TIM_Period = arr; 	//??????
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //?????
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //??????
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM7,&TIM_TimeBaseInitStructure);//???TIM5
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE); //?????5????0.................................................
	TIM_Cmd(TIM7,ENABLE); //?????6
	
	 /***********************NVIC initial****************************************/
	NVIC_InitStructure.NVIC_IRQChannel=TIM7_IRQn; //???5??
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03; //?????1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //????3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
}

void TIM1_Servo_PWM_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_BDTRInitTypeDef TIM_BDTRInitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_TIM4);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/* PWME=PC9/TIM3_CH4 对应舵机3，PWMF=PC8/TIM3_CH3 对应舵机4 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	TIM_TimeBaseInitStructure.TIM_Period = TIM1_SERVO_PERIOD_COUNT - 1U;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0U;
	TIM_TimeBaseInitStructure.TIM_Prescaler = TIM1_SERVO_TIM1_PSC - 1U;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
	TIM_TimeBaseInitStructure.TIM_Prescaler = TIM1_SERVO_TIM3_PSC - 1U;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
	TIM_TimeBaseInitStructure.TIM_Prescaler = TIM1_SERVO_TIM4_PSC - 1U;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStructure.TIM_Pulse = TIM1_SERVO_MID_PWM_COUNT;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM1, ENABLE);
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	TIM_ARRPreloadConfig(TIM4, ENABLE);
	TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
	TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
	TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF;
	TIM_BDTRInitStructure.TIM_DeadTime = 0U;
	TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;
	TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
	TIM_SetCompare1(TIM1, TIM1_SERVO_MID_PWM_COUNT);
	TIM_SetCompare2(TIM1, TIM1_SERVO_MID_PWM_COUNT);
	TIM_SetCompare3(TIM3, TIM1_SERVO_MID_PWM_COUNT);
	TIM_SetCompare4(TIM3, TIM1_SERVO_MID_PWM_COUNT);
	TIM_SetCompare1(TIM4, TIM1_SERVO_MID_PWM_COUNT);
	TIM_SetCompare2(TIM4, TIM1_SERVO_MID_PWM_COUNT);
	TIM_Cmd(TIM1, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	TIM_Cmd(TIM4, ENABLE);

	/* 上电默认角度：保持和旧版主板2工程一致 */
	TIM1_Servo_SetAngle(TIM1_SERVO_CH1, TIM1_SERVO1_INIT_ANGLE_DEG);
	TIM1_Servo_SetAngle(TIM1_SERVO_CH2, TIM1_SERVO2_INIT_ANGLE_DEG);
	TIM1_Servo_SetAngle(TIM1_SERVO_CH3, TIM1_SERVO3_INIT_ANGLE_DEG);
	TIM1_Servo_SetAngle(TIM1_SERVO_CH4, TIM1_SERVO4_INIT_ANGLE_DEG);
}

void TIM1_Servo_SetPwmCount(u8 channel, u16 pwm_count)
{
	if (pwm_count < TIM1_SERVO_MIN_PWM_COUNT)
	{
		pwm_count = TIM1_SERVO_MIN_PWM_COUNT;
	}
	else if (pwm_count > TIM1_SERVO_MAX_PWM_COUNT)
	{
		pwm_count = TIM1_SERVO_MAX_PWM_COUNT;
	}

	switch (channel)
	{
		case TIM1_SERVO_CH1:
			TIM_SetCompare1(TIM1, pwm_count);
			TIM_SetCompare1(TIM4, pwm_count);
			break;
		case TIM1_SERVO_CH2:
			TIM_SetCompare2(TIM1, pwm_count);
			TIM_SetCompare2(TIM4, pwm_count);
			break;
		case TIM1_SERVO_CH3:
			TIM_SetCompare4(TIM3, pwm_count);
			break;
		case TIM1_SERVO_CH4:
			TIM_SetCompare3(TIM3, pwm_count);
			break;
		default: break;
	}
}

void TIM1_Servo_SetPulseUs(u8 channel, u16 pulse_us)
{
	TIM1_Servo_SetPwmCount(channel, pulse_us);
}

void TIM1_Servo_SetAngle(u8 channel, int angle)
{
	u16 pwm_count;
	u32 pwm_delta;

	if (angle < 0)
	{
		angle = 0;
	}
	else if (angle > (int)TIM1_SERVO_MAX_ANGLE_DEG)
	{
		angle = TIM1_SERVO_MAX_ANGLE_DEG;
	}

	if (angle <= (int)TIM1_SERVO_MID_ANGLE_DEG)
	{
		pwm_delta = ((u32)(TIM1_SERVO_MID_ANGLE_DEG - (u32)angle) *
		             (TIM1_SERVO_MID_PWM_COUNT - TIM1_SERVO_MIN_PWM_COUNT)) /
		            TIM1_SERVO_MID_ANGLE_DEG;
		pwm_count = (u16)(TIM1_SERVO_MID_PWM_COUNT - pwm_delta);
	}
	else
	{
		pwm_delta = ((u32)((u32)angle - TIM1_SERVO_MID_ANGLE_DEG) *
		             (TIM1_SERVO_MAX_PWM_COUNT - TIM1_SERVO_MID_PWM_COUNT)) /
		            (TIM1_SERVO_MAX_ANGLE_DEG - TIM1_SERVO_MID_ANGLE_DEG);
		pwm_count = (u16)(TIM1_SERVO_MID_PWM_COUNT + pwm_delta);
	}

	TIM1_Servo_SetPwmCount(channel, pwm_count);
}
