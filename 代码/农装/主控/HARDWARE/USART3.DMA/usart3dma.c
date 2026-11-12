#include "usart3dma.h"
#include "delay.h"

//雷达3接收数据
uint8_t Uart3_Receive;
//激光雷达处理后的各个参数
uint16_t receive_cnt3;
uint8_t confidence3;
uint16_t distance3,noise3,reftof3;
uint32_t peak3,intg3;
LidarPointTypedef Pack3_Data[12];/* 雷达接收到的数据存储，12个点 */

/*
********************************************************************************************************************
*                  void  uart3_init(uint32_t bound)
*
*Description : 串口3的初始化 
*							 USART3_RX---PD9
*							 USART3_TX---PD8
*Arguments   : bound波特率参数
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void uart3_init(uint32_t bound)
{  	 
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	//初始化串口参数
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_BaudRate = bound;
	//初始化串口
	USART_Init(USART3,&USART_InitStructure);
	//配置中断
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//中断使能
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);
	//使能串口
	USART_Cmd(USART3, ENABLE);
	//使能IO时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_USART3);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/*
********************************************************************************************************************
*                  void  uart3_send_byte(uint8_t data)
*
*Description : 串口3发送单字节数据
*Arguments   : data要发送的数据
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void uart3_send_byte(uint8_t data)
{
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET); //等待发送缓冲区空
	USART_SendData(USART3, data); //发送数据
}


/*
********************************************************************************************************************
*                  void  USART3_IRQHandler(void)
*
*Description : 串口3接收中断
*Arguments   : none
*Returns     : none
*Notes       : 基于F103 STP-23激光雷达协议
********************************************************************************************************************
*/
void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		static uint8_t state = 0;
		static uint8_t crc = 0;
		static uint8_t cnt = 0;
		static uint8_t PACK_FLAG = 0;
		static uint8_t data_len  = 0;
		static uint32_t timestamp = 0;
		static uint8_t state_flag = 1;
		uint8_t temp_data;
		
		Uart3_Receive = USART_ReceiveData(USART3);
		temp_data=Uart3_Receive;
		
		if(state< 4)
		{
			if(temp_data == HEADER_OLD) 
			{
				state ++;
			}
			else 
			{
				state = 0;
			}
		}
		else if(state<10&&state>3)
		{
			switch(state)
			{
				case 4:   
					if(temp_data == device_address)
					{							
						state ++;
						crc = crc + temp_data;									
						break;        
					} 
					else state = 0,crc = 0;
				case 5:   
					if(temp_data == PACK_GET_DISTANCE)
					{  
						PACK_FLAG = PACK_GET_DISTANCE;
						state ++;	
						crc = crc + temp_data;	
						break;									
					}		 
					else if(temp_data == PACK_RESET_SYSTEM)
					{
						PACK_FLAG = PACK_RESET_SYSTEM;
						state ++; 
						crc = crc + temp_data;	
						break;	
					}
					else if(temp_data == PACK_STOP)
					{ 
						PACK_FLAG = PACK_STOP;
						state ++; 
						crc = crc + temp_data;	
						break;
					}
					else if(temp_data == PACK_ACK)
					{  
						PACK_FLAG = PACK_ACK;
						state ++;
						crc = crc + temp_data;	
						break;
					}			 				 
					else if(temp_data == PACK_VERSION)
					{ 
						PACK_FLAG = PACK_VERSION,
						state ++,
						crc = crc + temp_data;	   	     
						break;
					}
					else state = 0,crc = 0;
				case 6: if(temp_data == chunk_offset)
					{  
						state ++;
						crc = crc + temp_data;
						break; 	  
					}	
					else state = 0,crc = 0;
				case 7: if(temp_data == chunk_offset)
					{  
						state ++;
						crc = crc + temp_data;
						break;
					}
					else state = 0,crc = 0;
				case 8: 
					data_len = (u16)temp_data;
					state ++; 
					crc = crc + temp_data;
					break;																			 
				case 9: 
					data_len = data_len + ((u16)temp_data<<8);
					state ++;
					crc = crc + temp_data;
					break; 
				default: break;
			}
		}
		else if(state == 10 ) state_flag = 0;
		if(PACK_FLAG == PACK_GET_DISTANCE&&state_flag == 0)
		{
			if(state>9)
			{
				if(state<190)
				{
					static uint8_t state_num;
					state_num = (state-10)%15;
					switch(state_num)
					{
						case 0: 
							Pack3_Data[cnt].distance = (uint16_t)temp_data ;
							crc = crc + temp_data;
							state++;
							break;        
						case 1: 
							Pack3_Data[cnt].distance = ((u16)temp_data<<8) + Pack3_Data[cnt].distance;
							crc = crc + temp_data;
							state++;
							break; 
						case 2:
							Pack3_Data[cnt].noise = (u16)temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 3:
							Pack3_Data[cnt].noise = ((u16)temp_data<<8) + Pack3_Data[cnt].noise;
							crc = crc + temp_data;
							state++;
							break; 
						case 4:
							Pack3_Data[cnt].peak = (u32)temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 5:
							Pack3_Data[cnt].peak = ((u32)temp_data<<8) + Pack3_Data[cnt].peak;
							crc = crc + temp_data;
							state++;
							break; 
						case 6:
							Pack3_Data[cnt].peak = ((u32)temp_data<<16) + Pack3_Data[cnt].peak;	
							crc = crc + temp_data;
							state++;
							break; 
						case 7:
							Pack3_Data[cnt].peak = ((u32)temp_data<<24) + Pack3_Data[cnt].peak;
							crc = crc + temp_data;
							state++;
							break; 
						case 8:
							Pack3_Data[cnt].confidence = temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 9:
							Pack3_Data[cnt].intg = (u32)temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 10:
							Pack3_Data[cnt].intg = ((u32)temp_data<<8) + Pack3_Data[cnt].intg;
							crc = crc + temp_data;
							state++;
							break; 
						case 11:
							Pack3_Data[cnt].intg = ((u32)temp_data<<16) + Pack3_Data[cnt].intg;
							crc = crc + temp_data;
							state++;
							break; 
						case 12:
							Pack3_Data[cnt].intg = ((u32)temp_data<<24) + Pack3_Data[cnt].intg;
							crc = crc + temp_data;
							state++;
							break; 
						case 13:
							Pack3_Data[cnt].reftof = (int16_t)temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 14:
							Pack3_Data[cnt].reftof = ((int16_t)temp_data<<8) +Pack3_Data[cnt].reftof;
							crc = crc + temp_data;
							state++;
							cnt++;
							break; 
						default: break;
					}
				}
				if(state == 190) timestamp = temp_data,state++,crc = crc + temp_data;
				else if(state == 191) timestamp = ((u32)temp_data<<8) + timestamp,state++,crc = crc + temp_data; 
				else if(state == 192) timestamp = ((u32)temp_data<<16) + timestamp,state++,crc = crc + temp_data;
				else if(state == 193) timestamp = ((u32)temp_data<<24) + timestamp,state++,crc = crc + temp_data; 
				else if(state==194)
				{
					if(temp_data == crc)
					{
						data3_process();
						receive_cnt3++;
					}
					distance3 = Pack3_Data[0].distance;
					crc = 0;
					state = 0;
					state_flag = 1;
					cnt = 0;
				}
			}
		}
	}
} 

/*
********************************************************************************************************************
*                  void  data3_process(void)
*
*Description : 数据处理，接收到一帧之后可以进行数据处理
*Arguments   : none
*Returns     : none
*Notes       : 基于F103的数据处理逻辑，20帧数据取平均
********************************************************************************************************************
*/
void data3_process(void)
{
	u8 i;
	static u16 count = 0;
	LidarPointTypedef Pack3_sum;
	for(i=0;i<12;i++)
	{
		if(Pack3_Data[i].distance != 0)
		{
			count++;
			Pack3_sum.distance += Pack3_Data[i].distance;
			Pack3_sum.noise += Pack3_Data[i].noise;
			Pack3_sum.peak += Pack3_Data[i].peak;
			Pack3_sum.confidence += Pack3_Data[i].confidence;
			Pack3_sum.intg += Pack3_Data[i].intg;
			Pack3_sum.reftof += Pack3_Data[i].reftof;
		}
	}
	if(count !=0)
	{
		distance3 = Pack3_sum.distance/count;
		noise3 = Pack3_sum.noise/count;
		peak3 = Pack3_sum.peak/count;
		confidence3 = Pack3_sum.confidence/count;
		intg3 = Pack3_sum.intg/count;
		reftof3 = Pack3_sum.reftof/count;
		Pack3_sum.distance = 0;
		Pack3_sum.noise = 0;
		Pack3_sum.peak = 0;
		Pack3_sum.confidence = 0;
		Pack3_sum.intg = 0;
		Pack3_sum.reftof = 0;
		count = 0;
	}
}

