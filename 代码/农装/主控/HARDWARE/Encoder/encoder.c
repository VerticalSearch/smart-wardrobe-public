#include "encoder.h"

// 编码器数据结构实例
EncoderStru Encoder={0,0,0,0};

// 编码器原始计数值
int Encoder_Length_L1,Encoder_Length_R1,Encoder_Length_L2,Encoder_Length_R2;

// 编码器速度变量 (单位: cm/s)
float enc_speed_1=0,enc_speed_2=0,enc_speed_3=0,enc_speed_4=0;

// 编码器累计距离变量
int enc_1=0,enc_2=0,enc_3=0,enc_4=0;

// 编码器参数定义
#define diam    95.5    // 轮子直径，单位mm
#define PI 3.14159

int Read_EncoderL1(void)
{	
	Encoder.EncoderA = TIM2->CNT;  // 编码器A (TIM2 - PA0,PA1)
	return Encoder.EncoderA;		
}

int Read_EncoderL2(void)
{	
	Encoder.EncoderB = TIM3->CNT;  // 编码器B (TIM3 - PA6,PA7)
	return Encoder.EncoderB;		
}

int Read_EncoderR1(void)
{
	Encoder.EncoderC = TIM4->CNT;  // 编码器C (TIM4 - PD12,PD13)
	return Encoder.EncoderC;
}

int Read_EncoderR2(void)
{
	Encoder.EncoderD = TIM8->CNT;  // 编码器D (TIM8 - PC6,PC7)
	return Encoder.EncoderD;
}

void Read_Encoder(void)
{
	Encoder_Length_L1=Read_EncoderL1();
	Encoder_Length_L2=Read_EncoderL2();
	Encoder_Length_R1=Read_EncoderR1();
	Encoder_Length_R2=Read_EncoderR2();
}

void Timer6_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	TIM_TimeBaseStructure.TIM_Period = 999;
	TIM_TimeBaseStructure.TIM_Prescaler = 83;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter=0;
	TIM_TimeBaseInit(TIM6,&TIM_TimeBaseStructure);

	TIM_ClearFlag(TIM6, TIM_FLAG_Update);
	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel=TIM6_DAC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);       

	TIM_Cmd(TIM6, ENABLE);
}

// 编码器A初始化 - TIM2, PA0, PA1
void TIM2_Encoder_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
	TIM_ICInitTypeDef TIM_ICInitStructure;  
	GPIO_InitTypeDef GPIO_InitStructure;

	// 使能时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// 配置GPIO引脚
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_TIM2); 
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM2);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// 配置定时器时基
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0; // 不分频
	TIM_TimeBaseStructure.TIM_Period = ENCODER_TIM_PERIOD; // 自动重装载值
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	// 配置编码器接口
	TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	
	// 配置输入捕获通道1
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICFilter = 6;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);
	
	// 配置输入捕获通道2
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);

	// 清除标志位并启用中断
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	// 设置初始计数值
	TIM_SetCounter(TIM2, ENC_INIT);

	// 使能定时器
	TIM_Cmd(TIM2, ENABLE); 
}

// 编码器B初始化 - TIM3, PA6, PA7
void TIM3_Encoder_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
	TIM_ICInitTypeDef TIM_ICInitStructure;  
	GPIO_InitTypeDef GPIO_InitStructure;

	// 使能时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// 配置GPIO引脚
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_TIM3); 
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_TIM3);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// 配置定时器时基
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
	TIM_TimeBaseStructure.TIM_Period = ENCODER_TIM_PERIOD;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	// 配置编码器接口
	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	
	// 配置输入捕获通道1
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICFilter = 6;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);
	
	// 配置输入捕获通道2
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	// 清除标志位并启用中断
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	// 设置初始计数值
	TIM_SetCounter(TIM3, ENC_INIT);

	// 使能定时器
	TIM_Cmd(TIM3, ENABLE); 
}

// 编码器C初始化 - TIM4, PD12, PD13
void TIM4_Encoder_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
	TIM_ICInitTypeDef TIM_ICInitStructure;  
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 使能时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	// 配置GPIO引脚
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4); 
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_TIM4);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	// 配置定时器时基
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
	TIM_TimeBaseStructure.TIM_Period = ENCODER_TIM_PERIOD;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	
	// 配置编码器接口
	TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	
	// 配置输入捕获通道1
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICFilter = 6;
	TIM_ICInit(TIM4, &TIM_ICInitStructure);
	
	// 配置输入捕获通道2
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(TIM4, &TIM_ICInitStructure);
	
	// 清除标志位并启用中断
	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	
	// 设置初始计数值
	TIM_SetCounter(TIM4, ENC_INIT);
	
	// 使能定时器
	TIM_Cmd(TIM4, ENABLE); 
}

// 编码器D初始化 - TIM8, PC6, PC7
void TIM8_Encoder_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
	TIM_ICInitTypeDef TIM_ICInitStructure;  
	TIM_BDTRInitTypeDef TIM_BDTRInitStructure;  // TIM8是高级定时器，需要BDTR配置
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 使能时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	
	// 配置GPIO引脚
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM8); 
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM8);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// 配置定时器时基
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
	TIM_TimeBaseStructure.TIM_Period = ENCODER_TIM_PERIOD;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);
	
	// 配置编码器接口
	TIM_EncoderInterfaceConfig(TIM8, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	
	// 配置输入捕获通道1
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICFilter = 6;
	TIM_ICInit(TIM8, &TIM_ICInitStructure);
	
	// 配置输入捕获通道2
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(TIM8, &TIM_ICInitStructure);
	
	// TIM8是高级定时器，必须配置BDTR寄存器才能正常工作
	TIM_BDTRStructInit(&TIM_BDTRInitStructure);
	TIM_BDTRInitStructure.TIM_DeadTime = 0x00;  // 死区时间设为0
	TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;  // 禁用Break功能
	TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;
	TIM_BDTRConfig(TIM8, &TIM_BDTRInitStructure);
	
	// 清除标志位并启用中断
	TIM_ClearFlag(TIM8, TIM_FLAG_Update);
	TIM_ITConfig(TIM8, TIM_IT_Update, ENABLE);
	
	// 设置初始计数值
	TIM_SetCounter(TIM8, ENC_INIT);
	
	// 使能定时器
	TIM_Cmd(TIM8, ENABLE);
	
	// TIM8是高级定时器，必须启用主输出（MOE位）才能正常工作
	// 即使用于编码器模式也需要设置此位
	TIM_CtrlPWMOutputs(TIM8, ENABLE);
}

// 编码器数据处理变量
int Encoder_Fabs_L1=0,Encoder_Fabs_L2=0,Encoder_Fabs_R1=0,Encoder_Fabs_R2=0;
int Encoder_Fabs_L1_last=0,Encoder_Fabs_L2_last=0,Encoder_Fabs_R1_last=0,Encoder_Fabs_R2_last=0;
int Encoder_Real_L1=0,Encoder_Real_L2=0,Encoder_Real_R1=0,Encoder_Real_R2=0;

void Encoder_Fabs(void)//ȡֵӦߵľ,ȡֵӦλ
{
	Read_Encoder();
	Encoder_Fabs_L1=Encoder_Length_L1-ENC_INIT;
	Encoder_Fabs_R1=Encoder_Length_R1-ENC_INIT;
	Encoder_Fabs_L2=Encoder_Length_L2-ENC_INIT;
	Encoder_Fabs_R2=Encoder_Length_R2-ENC_INIT;
	
	if(Encoder_Fabs_L1>=0)
	{
		Encoder_Fabs_L1=Encoder_Fabs_L1;
	}else if(Encoder_Fabs_L1<0)
	{
		Encoder_Fabs_L1=-Encoder_Fabs_L1;
	}
	
	if(Encoder_Fabs_L2>=0)
	{
		Encoder_Fabs_L2=Encoder_Fabs_L2;
	}else if(Encoder_Fabs_L2<0)
	{
		Encoder_Fabs_L2=-Encoder_Fabs_L2;
	}
	
	
	if(Encoder_Fabs_R1>=0)
	{
		Encoder_Fabs_R1=Encoder_Fabs_R1;
	}else if(Encoder_Fabs_R1<0)
	{
		Encoder_Fabs_R1=-Encoder_Fabs_R1;
	}
	
	if(Encoder_Fabs_R2>=0)
	{
		Encoder_Fabs_R2=Encoder_Fabs_R2;
	}else if(Encoder_Fabs_R2<0)
	{
		Encoder_Fabs_R2=-Encoder_Fabs_R2;
	}
}

void Encoder_Real(void)//ȡֵӦλ
{
	Read_Encoder();
	Encoder_Real_L1=-(Encoder_Length_L1-ENC_INIT);
	Encoder_Real_R1=(Encoder_Length_R1-ENC_INIT);
	Encoder_Real_L2=-(Encoder_Length_L2-ENC_INIT);
	Encoder_Real_R2=(Encoder_Length_R2-ENC_INIT);
}

/**************************************************************************
 * 功能：编码器速度计算函数（每10ms调用一次）
 * 说明：读取编码器增量值，计算速度反馈值供PID控制使用
 *       速度单位：编码器计数/10ms
 **************************************************************************/
void Encoder_Speed_Calculate(void)
{
	// 步骤1：读取编码器相对位置（相对于ENC_INIT的增量）
	Encoder_Real();
	
	// 步骤2：累计位置（用于位置控制）
	enc_1 = enc_1 + Encoder_Real_L1;  // 编码器A累计位置
	enc_2 = enc_2 + Encoder_Real_L2;  // 编码器B累计位置
	enc_3 = enc_3 + Encoder_Real_R1;  // 编码器C累计位置
	enc_4 = enc_4 + Encoder_Real_R2;  // 编码器D累计位置
	
	// 步骤3：计算速度反馈值（编码器增量值 = 速度）
	// 单位：编码器计数/10ms
	enc_speed_1 = Encoder_Real_L1;  // 电机A速度反馈
	enc_speed_2 = Encoder_Real_L2;  // 电机B速度反馈
	enc_speed_3 = Encoder_Real_R1;  // 电机C速度反馈
	enc_speed_4 = Encoder_Real_R2;  // 电机D速度反馈
	
	// 步骤4：重置编码器计数器（准备下一次采样）
	TIM2->CNT = ENC_INIT;  // 编码器A重置
	TIM3->CNT = ENC_INIT;  // 编码器B重置
	TIM4->CNT = ENC_INIT;  // 编码器C重置
	TIM8->CNT = ENC_INIT;  // 编码器D重置
}
