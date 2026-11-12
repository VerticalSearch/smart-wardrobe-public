#include "stm32f4xx.h"                  // Device header
#include "delay.h"  
#include "guangdian.h" 

// 光电管初始化函数
void GD_Init()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	// 使能GPIOE时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	
//	// 初始化PE8为输出模式，初始化为高电平
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;        // 输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;        // 推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;    // 100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;      // 无上下拉
//	GPIO_Init(GPIOE, &GPIO_InitStructure);
//	GPIO_SetBits(GPIOE, GPIO_Pin_8);  // 初始化为高电平
	
	// 初始化GD2-4 (PE9, PE10, PE11)，PE8已单独配置为输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;        // 输入模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // 推挽输出（虽然这里是输入，但保持默认）
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // 100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       // 上拉输入（NPN NO常开型光电管）
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	// 初始化GD5-8 (PE15, PE14, PE13, PE12)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15|GPIO_Pin_14|GPIO_Pin_13|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;        // 输入模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // 推挽输出（虽然这里是输入，但保持默认）
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // 100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       // 上拉输入（NPN NO常开型光电管）
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

// 读取光电管状态函数
u8 GD_Read_GD1(void)
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_8);
}

u8 GD_Read_GD2(void)
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_9);
}

u8 GD_Read_GD3(void)
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_10);
}

u8 GD_Read_GD4(void)
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_11);
}

u8 GD_Read_GD5(void)
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_15);
}

u8 GD_Read_GD6(void)
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_14);
}

u8 GD_Read_GD7(void)
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_13);
}

u8 GD_Read_GD8(void)
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_12);
}

// 读取所有光电管状态
void GD_Read_All(u8 *gd_status)
{
	gd_status[0] = GD_Read_GD1();  // GD1
	gd_status[1] = GD_Read_GD2();  // GD2
	gd_status[2] = GD_Read_GD3();  // GD3
	gd_status[3] = GD_Read_GD4();  // GD4
	gd_status[4] = GD_Read_GD5();  // GD5
	gd_status[5] = GD_Read_GD6();  // GD6
	gd_status[6] = GD_Read_GD7();  // GD7
	gd_status[7] = GD_Read_GD8();  // GD8
}

//// PE8输出控制函数
//void GD_Set_PE8_High(void)
//{
//	GPIO_SetBits(GPIOE, GPIO_Pin_8);  // 设置PE8为高电平
//}

//void GD_Set_PE8_Low(void)
//{
//	GPIO_ResetBits(GPIOE, GPIO_Pin_8);  // 设置PE8为低电平
//}


