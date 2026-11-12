#ifndef __OLED_H
#define __OLED_H			  	 

#include "sys.h"
#include "stdlib.h"	    

 /*
******************************************************************************************************************
*                                            CONSTANTS & MACROS
******************************************************************************************************************
*/    						  
//-----------------OLED引脚定义------------------------------------------------------- 		
//D0/SCK--PB12,D1/MOSI--PB13,CS--PB15,DC--PB14  RST--3.3V
#define OLED_CS 	                          	PBout(14) 		//OLED片选
#define OLED_DC	                            	PBout(12) 		//OLED数据/命令
#define OLED_SCK 	                          	PBout(13) 		//OLED时钟线
#define OLED_MOSI	                            PBout(15) 		//OLED数据线

//#define OLED_CS 	                          	PBout(15) 		//OLED片选
//#define OLED_DC	                            	PBout(14) 		//OLED数据/命令
//#define OLED_SCK 	                          	PBout(12) 		//OLED时钟线
//#define OLED_MOSI	                            PBout(13) 		//OLED数据线

#define OLED_CMD  	                               0			//命令
#define OLED_DATA 	                               1			//数据
/*
******************************************************************************************************************
*                                            FUNCTION PROTOTYPES
*                                                   OLED相关函数
******************************************************************************************************************
*/
void OLED_WR_Byte(u8 dat,u8 cmd);	    
void OLED_Display_On(void);//开启OLED显示  
void OLED_Display_Off(void);//关闭OLED显示  
void OLED_Refresh_Gram(void);//更新显存到OLED	(调用此函数后，只是把显存更新，需要调用OLED_Refresh_Gram()才能显示)	   	   
							   		    
void OLED_Init(void);//OLED初始化
void OLED_Clear(void);//清屏
void OLED_DrawPoint(u8 x,u8 y,u8 t);//画点
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot);//填充
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size,u8 mode);//显示字符
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size);//显示数字
void OLED_ShowIntNum(u8 x,u8 y,int32_t num,u8 len,u8 size);//显示有符号整数
void OLED_Show3FNum(u8 x,u8 y,float num,u8 lenf,u8 lenb,u8 size);//显示小数
void OLED_ShowString(u8 x,u8 y,const u8 *p,u16 size);//显示字符串 
void OLED_ShowCHinese(u8 x,u8 y,u8 no);//显示汉字
#endif
