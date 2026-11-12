#include "stm32f4xx.h"                  // Device header


#include "timer.h"

#define BOARD1_RELAY_PWMD_PIN    GPIO_Pin_11 /* PD/PWMD，MCU引脚U3.70，对应PA11，风机继电器 */
#define BOARD1_RELAY_DIRD_PIN    GPIO_Pin_15 /* DD/DIRD，MCU引脚U3.46，对应PE15，加热继电器 */
#define BOARD1_RELAY_PWMC_PIN    GPIO_Pin_10 /* PWMC，MCU引脚U3.69，对应PA10，备用输出 */
#define BOARD1_RELAY_DIRC_PIN    GPIO_Pin_14 /* DIRC，MCU引脚U3.45，对应PE14，紫外灯：拉高亮灯 */
#define BOARD1_RELAY_PWMB_PIN    GPIO_Pin_9  /* PWMB，MCU引脚U3.68，对应PA9，换气风扇：拉高转动 */
#define BOARD1_RELAY_DIRB_PIN    GPIO_Pin_13 /* DIRB，MCU引脚U3.44，对应PE13，拍照区补光灯 */

void Board1_RelayIo_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE, ENABLE);

	/* 上电先拉低，再配置为输出，避免继电器在初始化瞬间误动作 */
	GPIO_ResetBits(GPIOA,
	               BOARD1_RELAY_PWMD_PIN |
	               BOARD1_RELAY_PWMC_PIN |
	               BOARD1_RELAY_PWMB_PIN);
	GPIO_ResetBits(GPIOE,
	               BOARD1_RELAY_DIRD_PIN |
	               BOARD1_RELAY_DIRC_PIN |
	               BOARD1_RELAY_DIRB_PIN);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Pin = BOARD1_RELAY_PWMD_PIN |
	                              BOARD1_RELAY_PWMC_PIN |
	                              BOARD1_RELAY_PWMB_PIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = BOARD1_RELAY_DIRD_PIN |
	                              BOARD1_RELAY_DIRC_PIN |
	                              BOARD1_RELAY_DIRB_PIN;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

void Board1_Relay_Set(u8 channel, u8 enable)
{
	GPIO_TypeDef *port = 0;
	uint16_t pin = 0U;

	switch (channel)
	{
		case BOARD1_RELAY_CH_FAN_MAIN:     port = GPIOA; pin = BOARD1_RELAY_PWMD_PIN; break;
		case BOARD1_RELAY_CH_HOT_AIR:      port = GPIOE; pin = BOARD1_RELAY_DIRD_PIN; break;
		case BOARD1_RELAY_CH_PWMC_AUX:     port = GPIOA; pin = BOARD1_RELAY_PWMC_PIN; break;
		case BOARD1_RELAY_CH_UV_LAMP:      port = GPIOE; pin = BOARD1_RELAY_DIRC_PIN; break;
		case BOARD1_RELAY_CH_VENT_FAN:     port = GPIOA; pin = BOARD1_RELAY_PWMB_PIN; break;
		case BOARD1_RELAY_CH_FILL_LIGHT:   port = GPIOE; pin = BOARD1_RELAY_DIRB_PIN; break;
		default: break;
	}

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	if (enable != 0U)
	{
		GPIO_SetBits(port, pin);
	}
	else
	{
		GPIO_ResetBits(port, pin);
	}
}

void Board1_FillLight_Set(u8 enable)
{
	Board1_Relay_Set(BOARD1_RELAY_CH_FILL_LIGHT, enable);
}

void Board1_UvLamp_Set(u8 enable)
{
	Board1_Relay_Set(BOARD1_RELAY_CH_UV_LAMP, enable);
}

void Board1_VentFan_Set(u8 enable)
{
	Board1_Relay_Set(BOARD1_RELAY_CH_VENT_FAN, enable);
}

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

