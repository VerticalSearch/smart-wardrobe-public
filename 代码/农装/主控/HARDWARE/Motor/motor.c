#include "stm32f4xx.h"                  // Device header
#include <stddef.h>                     // 包含NULL定义
void Motor_pwm_Init(u32 arr,u32 psc)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;  
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);    	//TIM1时钟使能    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 	//使能GPIOA时钟	
	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1); //GPIOA8配置为定时器1
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1); //GPIOA9配置为定时器1
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1); //GPIOA10配置为定时器1
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_TIM1); //GPIOA11配置为定时器1
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;   
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            //复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	    //速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            //上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure);                   //初始化PA8,9,10,11
	  
	TIM_TimeBaseStructure.TIM_Prescaler=psc;                  //定时器分频
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseStructure.TIM_Period=arr;                     //自动重装载值
	TIM_TimeBaseStructure.TIM_ClockDivision=0; 	
	TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructure);            //初始化定时器1
	
	//初始化TIM1 Channel 1,2,3,4 配置PWM模式	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;    //选择定时器模式:TIM脉冲宽度调制模式1
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
	// 禁用互补通道输出 - 关键配置！
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable; //禁用互补输出
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High; //互补输出极性
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset; //空闲状态
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset; //互补空闲状态
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);  //根据指定参数初始化TIM1 OC1 
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);  //使能TIM1在CCR1上的预装载寄存器
	
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;//PWM输出使能
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
	// 禁用互补通道输出
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);  //根据指定参数初始化TIM1 OC2
	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);  //使能TIM1在CCR2上的预装载寄存器

	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;//PWM输出使能
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
	// 禁用互补通道输出
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);  //根据指定参数初始化TIM1 OC3
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);  //使能TIM1在CCR3上的预装载寄存器

	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;//PWM输出使能
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
	// 禁用互补通道输出
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	TIM_OC4Init(TIM1, &TIM_OCInitStructure);  //根据指定参数初始化TIM1 OC4
	TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);  //使能TIM1在CCR4上的预装载寄存器

	// 配置BDTR寄存器 - 定时器1高级功能必需
	TIM_BDTRInitTypeDef TIM_BDTRInitStructure;
	TIM_BDTRStructInit(&TIM_BDTRInitStructure);
	TIM_BDTRInitStructure.TIM_DeadTime = 0x00;  // 死区时间设为0
	TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;  // 禁用Break功能
	TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

	TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE); //使能定时器1更新中断

	TIM_ARRPreloadConfig(TIM1,ENABLE);//ARPE使能 
	TIM_CtrlPWMOutputs(TIM1,ENABLE);
	TIM_Cmd(TIM1, ENABLE);  //使能TIM1					
	
}
void motor_Gpio_Init()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);//使能GPIOD时钟
  
	//PA2,PA3,PA4,PA5初始化方向控制
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉	
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化	
	
	//PD14,PD15,PD10,PD11初始化方向控制
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14|GPIO_Pin_15|GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉	
	GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化	

}

void MotorA_Ctrl(int Compare)
{
	if(Compare>0)
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_2);  // AIN1
		GPIO_ResetBits(GPIOA,GPIO_Pin_3); // AIN2
		TIM_SetCompare1(TIM1,Compare);
	}
	else if(Compare<0)
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_2); // AIN1
		GPIO_SetBits(GPIOA,GPIO_Pin_3);  // AIN2
		TIM_SetCompare1(TIM1,-Compare);
	}
	else 
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_2); // AIN1
		GPIO_ResetBits(GPIOA,GPIO_Pin_3); // AIN2
		TIM_SetCompare1(TIM1,0);
	}
}
void MotorB_Ctrl(int Compare)
{
	if(Compare>0)
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_4);  // BIN1
		GPIO_ResetBits(GPIOA,GPIO_Pin_5); // BIN2
		TIM_SetCompare2(TIM1,Compare);
	}
	else if(Compare<0)
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_4); // BIN1
		GPIO_SetBits(GPIOA,GPIO_Pin_5);  // BIN2
		TIM_SetCompare2(TIM1,-Compare);
	}
	else 
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_4); // BIN1
		GPIO_ResetBits(GPIOA,GPIO_Pin_5); // BIN2
		TIM_SetCompare2(TIM1,0);
	}
}
void MotorC_Ctrl(int Compare)
{
	if(Compare>0)
	{
		GPIO_SetBits(GPIOD,GPIO_Pin_14);  // CIN1
		GPIO_ResetBits(GPIOD,GPIO_Pin_15); // CIN2
		TIM_SetCompare3(TIM1,Compare);
	}
	else if(Compare<0)
	{
		GPIO_ResetBits(GPIOD,GPIO_Pin_14); // CIN1
		GPIO_SetBits(GPIOD,GPIO_Pin_15);  // CIN2
		TIM_SetCompare3(TIM1,-Compare);
	}
	else 
	{
		GPIO_ResetBits(GPIOD,GPIO_Pin_14); // CIN1
		GPIO_ResetBits(GPIOD,GPIO_Pin_15); // CIN2
		TIM_SetCompare3(TIM1,0);
	}
}
void MotorD_Ctrl(int Compare)
{
	if(Compare>0)
	{
		GPIO_SetBits(GPIOD,GPIO_Pin_10);  // DIN1
		GPIO_ResetBits(GPIOD,GPIO_Pin_11); // DIN2
		TIM_SetCompare4(TIM1,Compare);
	}
	else if(Compare<0)
	{
		GPIO_ResetBits(GPIOD,GPIO_Pin_10); // DIN1
		GPIO_SetBits(GPIOD,GPIO_Pin_11);  // DIN2
		TIM_SetCompare4(TIM1,-Compare);
	}
	else 
	{
		GPIO_ResetBits(GPIOD,GPIO_Pin_10); // DIN1
		GPIO_ResetBits(GPIOD,GPIO_Pin_11); // DIN2
		TIM_SetCompare4(TIM1,0);
	}
}


void control_all (int a,int b,int c, int d)
{
	MotorA_Ctrl(a);
	MotorB_Ctrl(b);
	MotorC_Ctrl(c);
	MotorD_Ctrl(d);
}

