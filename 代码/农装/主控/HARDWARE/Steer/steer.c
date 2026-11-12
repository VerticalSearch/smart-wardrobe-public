#include "stm32f4xx.h"                  // Device header
void steer_PWm_Init(u32 arr,u32 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;  
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);    	//TIM2时钟使能    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); 	//使能GPIOB时钟	
	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_TIM2); //GPIOB10配置为定时器2
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_TIM2); //GPIOB11配置为定时器2
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;   
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            //复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	    //速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            //上拉
	GPIO_Init(GPIOB,&GPIO_InitStructure);                   //初始化PB10,11
	  
	TIM_TimeBaseStructure.TIM_Prescaler=psc;                  //定时器分频
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseStructure.TIM_Period=arr;                     //自动重装载值
	TIM_TimeBaseStructure.TIM_ClockDivision=0; 	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);            //初始化定时器2
	
	//初始化TIM2 Channel 3,4 配置PWM模式	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;    //选择定时器模式:TIM脉冲宽度调制模式1
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);  //根据指定参数初始化TIM2 OC3 
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);  //使能TIM2在CCR3上的预装载寄存器
	
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;//PWM输出使能
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
	TIM_OC4Init(TIM2, &TIM_OCInitStructure);  //根据指定参数初始化TIM2 OC4
	TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);  //使能TIM2在CCR4上的预装载寄存器

	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //使能定时器2更新中断

	TIM_ARRPreloadConfig(TIM2,ENABLE);//ARPE使能 
	TIM_CtrlPWMOutputs(TIM2,ENABLE);
	TIM_Cmd(TIM2, ENABLE);  //使能TIM2					
}


void Steer_Angle_Control(u8 Which,int Steer_Angle)
{
    u16 Steer_PWM;
    
//    // 180度舵机控制
//    if(Steer_Angle < 0) Steer_Angle = 0;
//    if(Steer_Angle > 180) Steer_Angle = 180;
    
    // 脉宽：0.5ms-2.5ms对应0°-180°
    Steer_PWM = 500 + (Steer_Angle * 2000) / 180;
    
    switch(Which)
    {
        case 1: TIM_SetCompare3(TIM2, Steer_PWM); break;
        case 2: TIM_SetCompare4(TIM2, Steer_PWM); break;
        default: break;
    }
}