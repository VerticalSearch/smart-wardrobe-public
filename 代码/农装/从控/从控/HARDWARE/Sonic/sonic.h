/*
********************************************************************************************************************
*Filename              :UltrasonicWave.h
*Programmer(s)    :Lab 416
*Description          :UltrasonicWave function
********************************************************************************************************************
*/

//#ifndef __sonic_H
//#define	__sonic_H

//#include "sys.h"


//#define TIME_OUT 10000        //???????????


//#define Trig1_on()  (GPIO_SetBits(GPIOE,GPIO_Pin_12))             //??Front_Trig
//#define Trig1_off() (GPIO_ResetBits(GPIOE,GPIO_Pin_12)) 

//#define Trig2_on()  (GPIO_SetBits(GPIOE,GPIO_Pin_13))             //??Front_Trig
//#define Trig2_off() (GPIO_ResetBits(GPIOE,GPIO_Pin_13)) 

//#define Trig3_on()  (GPIO_SetBits(GPIOE,GPIO_Pin_14))             //??Front_Trig
//#define Trig3_off() (GPIO_ResetBits(GPIOE,GPIO_Pin_14)) 

//#define Trig4_on()  (GPIO_SetBits(GPIOE,GPIO_Pin_15))             //??Front_Trig
//#define Trig4_off() (GPIO_ResetBits(GPIOE,GPIO_Pin_15)) 

//#define Trig5_on()  (GPIO_SetBits(GPIOD,GPIO_Pin_14))             //??Front_Trig
//#define Trig5_off() (GPIO_ResetBits(GPIOD,GPIO_Pin_14)) 

//#define Trig6_on()  (GPIO_SetBits(GPIOD,GPIO_Pin_15))             //??Front_Trig
//#define Trig6_off() (GPIO_ResetBits(GPIOD,GPIO_Pin_15)) 



//typedef struct
//{
//	u8  CAPTURE_STA;                 //ͨ�����벶���־������λ�������־����6λ�������־		
//	u16 CAPTURE_UPVAL;               //ͨ����������
//	u16 CAPTURE_DOWNVAL;             //ͨ���½�����
//	u32	CAPTURE_VAL;	     					 //ͨ�����벶��ֵ
//	
//	float DIS; 											 //����������ֵ
//	float Last_DIS;
//	float max;
//	float min;
//	u8  CAPTURE_SELECT;                        
//} Ultra_cap_t;


//typedef struct 
//{
//	
//	int A_1;
//	int A_2;
//	int A_3;
//	int A_4;
//	int A_5;
//	int A_6;
////	float A_1;
////	float A_2;
////	float A_3;
////	float A_4;
////	float A_5;
////	float A_6;

//}SonicDis;


//typedef struct
//{
//	unsigned char 			Complete;
//	unsigned char       Count;
// int  				Cache;	        //�����м仺��ֵ
// int 					Last_Data;	    //��һ�β���ֵ
// int  				Data_His[15];		//������ʷֵ
// }Filter_Struct_S;

//extern SonicDis Sonic_Dis;


///*
//******************************************************************************************************************
//*                                            FUNCTION PROTOTYPES
//******************************************************************************************************************
//*/

//void Ultra_Init(void);//???IO??????
//void Ultrasonic_Trig_Start(void);
//void Sonic_Get_Distance(void);//????

//void TIM5_Sonic_Cap_Init(u32 arr,u16 psc);
//void TIM3_Sonic_Cap_Init(u32 arr,u16 psc);
//void TIM5_IRQHandler(void);
//void TIM3_IRQHandler(void);

//void Ultrasonic_Filter(Filter_Struct_S *filterStruct,float newDatas,unsigned char FilterNum,int *Out);
//void Delay_ULTRA(unsigned char n);

//int Sonic_Length(float l1,float l2);
//int Sonic_Length_l(float l1,float l2);


//#endif 


/**************************************************************************************************************************************/


#ifndef __sonic_H
#define	__sonic_H

#include "sys.h"


#define TIME_OUT 10000        //???????????


#define Trig1_on()  (GPIO_SetBits(GPIOE,GPIO_Pin_12))             //超声波1触发
#define Trig1_off() (GPIO_ResetBits(GPIOE,GPIO_Pin_12)) 

#define Trig2_on()  (GPIO_SetBits(GPIOE,GPIO_Pin_13))             //超声波2触发
#define Trig2_off() (GPIO_ResetBits(GPIOE,GPIO_Pin_13)) 

#define Trig3_on()  (GPIO_SetBits(GPIOE,GPIO_Pin_14))             //超声波3触发
#define Trig3_off() (GPIO_ResetBits(GPIOE,GPIO_Pin_14)) 

#define Trig4_on()  (GPIO_SetBits(GPIOE,GPIO_Pin_15))             //超声波4触发
#define Trig4_off() (GPIO_ResetBits(GPIOE,GPIO_Pin_15)) 

typedef struct 
{
	int A_1;  //超声波1距离
	int A_2;  //超声波2距离
	int A_3;  //超声波3距离
	int A_4;  //超声波4距离
}SonicDis;

extern SonicDis Sonic_Dis;



void Ultra_Init(void);//???IO??????
void Ultrasonic_Trig_Start(void);
void Sonic_Get_Distance(void);//????

void TIM2_Sonic_Cap_Init(u32 arr,u16 psc);
void TIM2_IRQHandler(void);
int Sonic_Length(float l1,float l2);
int Sonic_Length_l(float l1,float l2);


#endif 





