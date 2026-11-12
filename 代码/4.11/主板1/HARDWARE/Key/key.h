#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h" 

/*按键的方式：通过直接读取IO的方式获取IO*/
#define KEY1 		GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_1)  //PE1
#define KEY2 		GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_0)  //PE0

/*按键的方式：通过位带操作的方式获取IO*/
/*
#define KEY0 		PEin(4)   	//PE4
#define KEY1 		PEin(3)		//PE3 
#define KEY2 		PEin(2)		//P32
#define WK_UP 	PAin(0)		//PA0
*/

#define KEY1_PRES 	  1
#define KEY2_PRES	  2

void KEY_Init(void);	//IO初始化
u8 KEY_Scan(u8);  		//按键扫描函数	
void mode_set(void);
#endif

