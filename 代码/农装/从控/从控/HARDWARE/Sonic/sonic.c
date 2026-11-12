/********************************************************************************************************************************/

#include "sonic.h"
#include "delay.h"
#include "filter.h"
#include "oled.h"
#include "math.h"
#include "usartdma.h"

/*
******************************************************************************************************************
*                                            CONSTANTS & MACROS
******************************************************************************************************************
*/


u8     TIM2CH1_CAPTURE_STA=0;	//定时器2通道1捕获状态	    				
u32	 TIM2CH1_CAPTURE_VAL=0;	//定时器2通道1捕获值
u8     TIM2CH2_CAPTURE_STA=0;	//定时器2通道2捕获状态	    				
u32	 TIM2CH2_CAPTURE_VAL=0;	//定时器2通道2捕获值
u8     TIM2CH3_CAPTURE_STA=0;	//定时器2通道3捕获状态	    				
u32	 TIM2CH3_CAPTURE_VAL=0;	//定时器2通道3捕获值
u8     TIM2CH4_CAPTURE_STA=0;	//定时器2通道4捕获状态	    				
u32	 TIM2CH4_CAPTURE_VAL=0;	//定时器2通道4捕获值




long long temp1=0;  //超声波临时值
long long temp2=0;  //超声波临时值
long long temp3=0;  //超声波临时值
long long temp4=0;  //超声波临时值

int TEMPA_1=0;  //超声波距离值
int TEMPA_2=0;  //超声波距离值
int TEMPA_3=0;  //超声波距离值
int TEMPA_4=0;  //超声波距离值

SonicDis Sonic_Dis={0,0,0,0};

Filter_Struct SonicA1_Filter = {0,0,0,0,{0}};
Filter_Struct SonicA2_Filter = {0,0,0,0,{0}};
Filter_Struct SonicA3_Filter = {0,0,0,0,{0}};
Filter_Struct SonicA4_Filter = {0,0,0,0,{0}};

void Ultra_Init(void)
{
  GPIO_InitTypeDef    GPIO_InitStructure;	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);//
	 /***********************GPIO initial****************************************/
		 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;//TRIG1-4触发引脚
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//
  GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;//
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//100MHz        
  GPIO_Init(GPIOE, &GPIO_InitStructure);//GPIOE
	
  GPIO_ResetBits(GPIOE,GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);//TRIG=0
	
}


void Sonic_Get_Distance(){	
	
	if(TIM2CH1_CAPTURE_STA&0X80){
		temp1=TIM2CH1_CAPTURE_STA&0X3F; 
		temp1+=TIM2CH1_CAPTURE_VAL;//获取总时间
		TIM2CH1_CAPTURE_STA=0;     //清除标志位
	}
	TEMPA_1=temp1*340/6000; //转换为cm
	Enaverage_Filter(&SonicA1_Filter,TEMPA_1,15, &Sonic_Dis.A_1);
	
	if(TIM2CH2_CAPTURE_STA&0X80){
		temp2=TIM2CH2_CAPTURE_STA&0X3F;
		temp2+=TIM2CH2_CAPTURE_VAL;//获取总时间
		TIM2CH2_CAPTURE_STA=0;     //清除标志位
	}
	TEMPA_2=temp2*340/6000; //转换为cm
	Enaverage_Filter(&SonicA2_Filter,TEMPA_2,15, &Sonic_Dis.A_2);	
	    	
	if(TIM2CH3_CAPTURE_STA&0X80){
		temp3=TIM2CH3_CAPTURE_STA&0X3F; 
		temp3+=TIM2CH3_CAPTURE_VAL;//获取总时间
		TIM2CH3_CAPTURE_STA=0;     //清除标志位
	}
	TEMPA_3=temp3*340/6000; //转换为cm
	Enaverage_Filter(&SonicA3_Filter,TEMPA_3,15, &Sonic_Dis.A_3);
	
	if(TIM2CH4_CAPTURE_STA&0X80){
		temp4=TIM2CH4_CAPTURE_STA&0X3F; 
		temp4+=TIM2CH4_CAPTURE_VAL;//获取总时间
		TIM2CH4_CAPTURE_STA=0;     //清除标志位
	}
	TEMPA_4=temp4*340/6000; //转换为cm
	Enaverage_Filter(&SonicA4_Filter,TEMPA_4,15, &Sonic_Dis.A_4);
	
		if(Sonic_Dis.A_1>300)
			Sonic_Dis.A_1=300;
		else
			Sonic_Dis.A_1=Sonic_Dis.A_1;
		
		if(Sonic_Dis.A_2>300)
			Sonic_Dis.A_2=300;
		else
			Sonic_Dis.A_2=Sonic_Dis.A_2;
		
		if(Sonic_Dis.A_3>300)
			Sonic_Dis.A_3=300;
		else
			Sonic_Dis.A_3=Sonic_Dis.A_3;
		
		if(Sonic_Dis.A_4>300)
			Sonic_Dis.A_4=300;
		else
			Sonic_Dis.A_4=Sonic_Dis.A_4;
}




void TIM2_Sonic_Cap_Init(u32 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
    TIM_ICInitTypeDef  TIM2_ICInitStructure;
	
	/***********************Clock initial****************************************/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);      
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	/***********************GPIO initial****************************************/	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_DOWN;//下拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//速度100MHz 
    GPIO_Init(GPIOA,&GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3);
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource0,GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource1,GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_TIM2);
	
	 /***********************Timer initial****************************************/	 
	TIM_TimeBaseStructure.TIM_Prescaler=psc;//预分频器
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;//向上计数
	TIM_TimeBaseStructure.TIM_Period=arr;//自动重装载值
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);
	
	
	//配置TIM2的四个通道
    TIM2_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;//上升沿捕获
    TIM2_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;//映射到TI1上
    TIM2_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;//配置输入分频,不分频 
    TIM2_ICInitStructure.TIM_ICFilter = 0x00;//IC1F=0000 配置输入滤波器 不滤波
    TIM_ICInit(TIM2, &TIM2_ICInitStructure);
	
	TIM2_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInit(TIM2, &TIM2_ICInitStructure);
	
	TIM2_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(TIM2, &TIM2_ICInitStructure);
	
	TIM2_ICInitStructure.TIM_Channel = TIM_Channel_3;
	TIM_ICInit(TIM2, &TIM2_ICInitStructure);
	
	TIM2_ICInitStructure.TIM_Channel = TIM_Channel_4;
	TIM_ICInit(TIM2, &TIM2_ICInitStructure);
	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);//使能更新中断 ,使能CC4IE中断		
	TIM_ITConfig(TIM2,TIM_IT_CC1,ENABLE);						// 使能捕获中断 ,使能CC2IE中断	
	TIM_ITConfig(TIM2,TIM_IT_CC2,ENABLE);						// 使能捕获中断 ,使能CC2IE中断	
	TIM_ITConfig(TIM2,TIM_IT_CC3,ENABLE);						// 使能捕获中断 ,使能CC2IE中断	
	TIM_ITConfig(TIM2,TIM_IT_CC4,ENABLE);						// 使能捕获中断 ,使能CC2IE中断	
	
    TIM_Cmd(TIM2,ENABLE );//使能定时器2
	
  /***********************NVIC initial****************************************/
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =1;//子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器	
}




void TIM2_IRQHandler(void)
{
	if((TIM2CH1_CAPTURE_STA&0X80)==0)//还未成功捕获	
	{
		if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)//溢出
		{
			if(TIM2CH1_CAPTURE_STA&0X40)        //已经捕获到高电平了
			{
				if((TIM2CH1_CAPTURE_STA&0X3F)==0X3F)//高电平太长了
				{
					TIM2CH1_CAPTURE_STA|=0X80;	      //标记成功捕获了一次
					TIM2CH1_CAPTURE_VAL=0XFFFFFFFF;
				}else TIM2CH1_CAPTURE_STA++;
			}
		}
		if(TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)//捕获1发生捕获事件
		{
			if(TIM2CH1_CAPTURE_STA&0X40)//有上升沿捕获 		
			{
				TIM2CH1_CAPTURE_STA|=0X80;	//标记成功捕获到一次高电平脉宽
				TIM2CH1_CAPTURE_VAL=TIM_GetCapture1(TIM2);//获取当前的捕获值.
				TIM_OC1PolarityConfig(TIM2,TIM_ICPolarity_Rising); //CC1P=0 设置为上升沿捕获
			}else //还未开始,第一次捕获上升沿
			{
				TIM2CH1_CAPTURE_STA=0;	//清空
				TIM2CH1_CAPTURE_VAL=0;
				TIM2CH1_CAPTURE_STA|=0X40;		//标记捕获到了上升沿
				TIM_Cmd(TIM2,DISABLE ); 	//关闭定时器2
				TIM_SetCounter(TIM2,0);
				TIM_OC1PolarityConfig(TIM2,TIM_ICPolarity_Falling);	//CC1P=1 设置为下降沿捕获
				TIM_Cmd(TIM2,ENABLE ); 	//使能定时器2
			}
		}
	}
	
	if((TIM2CH2_CAPTURE_STA&0X80)==0)//还未成功捕获	
	{
		if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)//溢出
		{
			if(TIM2CH2_CAPTURE_STA&0X40)        //已经捕获到高电平了
			{
				if((TIM2CH2_CAPTURE_STA&0X3F)==0X3F)//高电平太长了
				{
					TIM2CH2_CAPTURE_STA|=0X80;	      //标记成功捕获了一次
					TIM2CH2_CAPTURE_VAL=0XFFFFFFFF;
				}else TIM2CH2_CAPTURE_STA++;
			}
		}
		if(TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET)//捕获2发生捕获事件
		{
			if(TIM2CH2_CAPTURE_STA&0X40)//有上升沿捕获 		
			{
				TIM2CH2_CAPTURE_STA|=0X80;	//标记成功捕获到一次高电平脉宽
				TIM2CH2_CAPTURE_VAL=TIM_GetCapture2(TIM2);//获取当前的捕获值.
				TIM_OC2PolarityConfig(TIM2,TIM_ICPolarity_Rising); //CC2P=0 设置为上升沿捕获
			}else //还未开始,第一次捕获上升沿
			{
				TIM2CH2_CAPTURE_STA=0;	//清空
				TIM2CH2_CAPTURE_VAL=0;
				TIM2CH2_CAPTURE_STA|=0X40;		//标记捕获到了上升沿
				TIM_Cmd(TIM2,DISABLE); 	//关闭定时器2
				TIM_SetCounter(TIM2,0);
				TIM_OC2PolarityConfig(TIM2,TIM_ICPolarity_Falling);	//CC2P=1 设置为下降沿捕获
				TIM_Cmd(TIM2,ENABLE ); 	//使能定时器2
			}
		}
	}
	
	if((TIM2CH3_CAPTURE_STA&0X80)==0)//还未成功捕获	
	{
		if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)//溢出
		{
			if(TIM2CH3_CAPTURE_STA&0X40)        //已经捕获到高电平了
			{
				if((TIM2CH3_CAPTURE_STA&0X3F)==0X3F)//高电平太长了
				{
					TIM2CH3_CAPTURE_STA|=0X80;	      //标记成功捕获了一次
					TIM2CH3_CAPTURE_VAL=0XFFFFFFFF;
				}else TIM2CH3_CAPTURE_STA++;
			}
		}
		if(TIM_GetITStatus(TIM2, TIM_IT_CC3) != RESET)//捕获3发生捕获事件
		{
			if(TIM2CH3_CAPTURE_STA&0X40)//有上升沿捕获 		
			{
				TIM2CH3_CAPTURE_STA|=0X80;	//标记成功捕获到一次高电平脉宽
				TIM2CH3_CAPTURE_VAL=TIM_GetCapture3(TIM2);//获取当前的捕获值.
				TIM_OC3PolarityConfig(TIM2,TIM_ICPolarity_Rising); //CC3P=0 设置为上升沿捕获
			}else //还未开始,第一次捕获上升沿
			{
				TIM2CH3_CAPTURE_STA=0;	//清空
				TIM2CH3_CAPTURE_VAL=0;
				TIM2CH3_CAPTURE_STA|=0X40;		//标记捕获到了上升沿
				TIM_Cmd(TIM2,DISABLE); 	//关闭定时器2
				TIM_SetCounter(TIM2,0);
				TIM_OC3PolarityConfig(TIM2,TIM_ICPolarity_Falling);	//CC3P=1 设置为下降沿捕获
				TIM_Cmd(TIM2,ENABLE ); 	//使能定时器2
			}
		}
	}
	
	if((TIM2CH4_CAPTURE_STA&0X80)==0)//还未成功捕获	
	{
		if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)//溢出
		{
			if(TIM2CH4_CAPTURE_STA&0X40)        //已经捕获到高电平了
			{
				if((TIM2CH4_CAPTURE_STA&0X3F)==0X3F)//高电平太长了
				{
					TIM2CH4_CAPTURE_STA|=0X80;	      //标记成功捕获了一次
					TIM2CH4_CAPTURE_VAL=0XFFFFFFFF;
				}else TIM2CH4_CAPTURE_STA++;
			}
		}
		if(TIM_GetITStatus(TIM2, TIM_IT_CC4) != RESET)//捕获4发生捕获事件
		{
			if(TIM2CH4_CAPTURE_STA&0X40)//有上升沿捕获 		
			{
				TIM2CH4_CAPTURE_STA|=0X80;	//标记成功捕获到一次高电平脉宽
				TIM2CH4_CAPTURE_VAL=TIM_GetCapture4(TIM2);//获取当前的捕获值.
				TIM_OC4PolarityConfig(TIM2,TIM_ICPolarity_Rising); //CC4P=0 设置为上升沿捕获
			}else //还未开始,第一次捕获上升沿
			{
				TIM2CH4_CAPTURE_STA=0;	//清空
				TIM2CH4_CAPTURE_VAL=0;
				TIM2CH4_CAPTURE_STA|=0X40;		//标记捕获到了上升沿
				TIM_Cmd(TIM2,DISABLE); 	//关闭定时器2
				TIM_SetCounter(TIM2,0);
				TIM_OC4PolarityConfig(TIM2,TIM_ICPolarity_Falling);	//CC4P=1 设置为下降沿捕获
				TIM_Cmd(TIM2,ENABLE ); 	//使能定时器2
			}
		}
	}
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC1|TIM_IT_CC2|TIM_IT_CC3|TIM_IT_CC4|TIM_IT_Update); //清除中断标志位
}

void Ultrasonic_Trig_Start(void)
{
	// 同时触发4个超声波
	Trig1_on(); 
	Trig2_on(); 
	Trig3_on(); 
	Trig4_on(); 
	
	delay_us(10);  // 只延时10us
	
	// 同时关闭4个超声波
	Trig1_off(); 
	Trig2_off(); 
	Trig3_off(); 
	Trig4_off(); 
}

void Ultrasonic_Filter(void)
{
	Enaverage_Filter(&SonicA1_Filter,TEMPA_1,15, &Sonic_Dis.A_1);
	Enaverage_Filter(&SonicA2_Filter,TEMPA_2,15, &Sonic_Dis.A_2);
	Enaverage_Filter(&SonicA3_Filter,TEMPA_3,15, &Sonic_Dis.A_3);
	Enaverage_Filter(&SonicA4_Filter,TEMPA_4,15, &Sonic_Dis.A_4);
}

