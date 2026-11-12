/*
******************************************************************************************************************
*Filename      :usart2_dma.c
*Programmer(s) :chu (修改为激光雷达驱动)
*Description   :USART2激光雷达驱动
*							 USART2_RX---PD6
*							 USART2_TX---PD5
******************************************************************************************************************
*/
#include "usart2dma.h"
#include "laser_common.h"
#include "oled.h"

//激光雷达2接收数据
uint8_t Uart2_Receive;
//激光雷达处理后的各个参数
uint16_t receive_cnt2;
uint8_t confidence2;
uint16_t distance2,noise2,reftof2;
uint32_t peak2,intg2;
LidarPointTypedef Pack2_Data[12];/* 雷达接收到的数据存储，12个点 */
/*
********************************************************************************************************************
*                  void  USART2_Init(void)
*
*Description : ����3�ĳ�ʼ�� 
							 USART2_RX---PD9
							 USART2_TX---PD8
*Arguments   : Baud������������
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
//void USART2_Init(int Baud)
//{
//	NVIC_InitTypeDef NVIC_InitStructure ;//�����жϽṹ��
// 	GPIO_InitTypeDef GPIO_InitStructure;//����IO��ʼ���ṹ��
//	USART_InitTypeDef USART_InitStructure;//���崮�ڽṹ��
//	DMA_InitTypeDef DMA_InitStructure;//����DMA�ṹ��

//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);//�򿪴��ڶ�Ӧ������ʱ��

//	//**********************���� ���� DMA ���ã�DMA1_Stream6��**************************
//	// 0 ����DMAʱ��
//    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

//	// 1 DMA�����ж�����
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);

//	// 2 DMAͨ������
//	DMA_DeInit(DMA1_Stream3);
//	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
//	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);//�����ַ
//	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)send_data_u2;//�ڴ��ַ
//	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;//dma���䷽��
//	DMA_InitStructure.DMA_BufferSize = U2_DATA_LEN;//����DMA�ڴ���ʱ�������ĳ���
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//����DMA���������ģʽ��һ������
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//����DMA���ڴ����ģʽ

//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//���������ֳ�
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;//�ڴ������ֳ�
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;//����DMA�Ĵ���ģʽ
//	DMA_InitStructure.DMA_Priority = DMA_Priority_High;	//����DMA�����ȼ���
//	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;    //ָ�����FIFOģʽ��ֱ��ģʽ������ָ������ �� ��ʹ��FIFOģʽ
//	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;  	//ָ����FIFO��ֵˮƽ
//	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single; //ָ����Burstת�������ڴ洫��
//	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; //ָ����Burstת��������Χת�� */

//	// 3 ����DMA1��ͨ��
//	DMA_Init(DMA1_Stream3, &DMA_InitStructure);
//	// 4 ʹ���ж�
//	DMA_ITConfig(DMA1_Stream3,DMA_IT_TC,ENABLE);


//	/* 5.ʹ�ܴ��ڵ�DMA���ͽӿ� */
//    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
//	// 6 �ر�ͨ��
//	DMA_Cmd(DMA1_Stream3,DISABLE);//�ȹر�

//	//**********************���� ���� DMA ����**************************
//	// 0 ����DMAʱ��
//    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);//DMAͨ������
//	
//	// 2 DMAͨ������
//	DMA_DeInit(DMA1_Stream1);
//	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
//	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);//�����ַ
//	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rec_data_u2;//�ڴ��ַ
//	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;//dma���䷽��
//	DMA_InitStructure.DMA_BufferSize = U2_DATA_LEN;//����DMA�ڴ���ʱ�������ĳ���
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//����DMA���������ģʽ��һ������
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//����DMA���ڴ����ģʽ
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//���������ֳ�
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;//�ڴ������ֳ�
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//����DMA�Ĵ���ģʽ
//	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;//����DMA�����ȼ���

//	// 3 ����DMA1��ͨ��
//	DMA_Init(DMA1_Stream1, &DMA_InitStructure);
//	// 4 ʹ��ͨ��
//	DMA_Cmd(DMA1_Stream1,ENABLE);

//	/* 5.ʹ�ܴ��ڵ�DMA���� */
//	USART_DMACmd(USART2,USART_DMAReq_Rx,ENABLE);


//    //��ʼ�����ڲ���
//    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//    USART_InitStructure.USART_StopBits = USART_StopBits_1;
//    USART_InitStructure.USART_Parity = USART_Parity_No;
//    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
//    USART_InitStructure.USART_BaudRate = Baud;
//	//��ʼ������
//    USART_Init(USART2,&USART_InitStructure);


//	//�����ж�
//    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;               //ͨ������Ϊ�����ж�
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;       //�ж�ռ�ȵȼ�
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;              //�ж���Ӧ���ȼ�
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                 //���ж�
//    NVIC_Init(&NVIC_InitStructure);

//	//����DMA��ʽ����
//	USART_DMACmd(USART2,USART_DMAReq_Tx,ENABLE);
//	//����DMA��ʽ����
//	USART_DMACmd(USART2,USART_DMAReq_Rx,ENABLE);

//	//�ж�����
//	USART_ITConfig(USART2,USART_IT_TC,DISABLE);
//	USART_ITConfig(USART2,USART_IT_RXNE,DISABLE);
//	USART_ITConfig(USART2,USART_IT_TXE,DISABLE);
//	USART_ITConfig(USART2,USART_IT_IDLE,ENABLE);


//    //��������
//    USART_Cmd(USART2, ENABLE);

//    //����IO��ʱ��
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
//	GPIO_PinAFConfig(GPIOD,GPIO_PinSource5,GPIO_AF_USART2);
//	GPIO_PinAFConfig(GPIOD,GPIO_PinSource6,GPIO_AF_USART2);

//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;	 //�ܽ�ģʽ:�����
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	    //����:����ģʽ
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	 //������������
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//IO���ٶ�
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;  //�ܽ�ָ��
//	GPIO_Init(GPIOD, &GPIO_InitStructure);//��ʼ��


//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;	//�ܽ�ģʽ:�����
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;	 //������������
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;    //�ܽ�ָ��
//	GPIO_Init(GPIOD, &GPIO_InitStructure);      //��ʼ��
//}
/*
********************************************************************************************************************
*                  void  uart2_init(uint32_t bound)
*
*Description : 串口2的初始化（激光雷达）
*							 USART2_RX---PD6
*							 USART2_TX---PD5
*Arguments   : bound波特率参数
*Returns     : none
*Notes       : 基于USART3/4激光雷达驱动移植
********************************************************************************************************************
*/
void uart2_init(uint32_t bound)
{  	 
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	//初始化串口参数
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_BaudRate = bound;
	//初始化串口
	USART_Init(USART2,&USART_InitStructure);
	//配置中断
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//中断使能
	USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
	//使能串口
	USART_Cmd(USART2, ENABLE);
	//使能IO时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource5,GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource6,GPIO_AF_USART2);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

// 保留原函数名兼容性
void USART2_Init(u32 bound)
{
	uart2_init(bound);
}
/*
********************************************************************************************************************
*                  void  USART2_IRQHandler(void)
*
*Description : ����3���ͺ��� 
*Arguments   : BufferSRC:�������ݴ�ŵ�ַ��BufferSize:���������ֽ���
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void USART2_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize)//UASRT DMA��������
{
	DMA_ClearFlag(DMA1_Stream3,DMA_FLAG_TCIF3);
	DMA1_Stream3->NDTR = BufferSize;    //����BUFFER��С
	DMA1_Stream3->M0AR = (uint32_t)BufferSRC;   //���õ�ַ
	DMA1_Stream3->CR |= DMA_SxCR_EN;	//��DMA,��ʼ����
}



/*
********************************************************************************************************************
*                  void  USART2_IRQHandler(void)
*
*Description : ����3���պ��� 
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
//void USART2_IRQHandler(void)
//{
//	static uint8_t Uart3_Rec_Len = 0;
//	int t;
//	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)  //�����ж�
//	{
//		t= USART_ReceiveData(USART2);//��ȡ���� ע�⣺������Ҫ�������ܹ�����жϱ�־λ��
//		Uart3_Rec_Len = U2_DATA_LEN - DMA1_Stream3->NDTR;	//����ӱ�֡���ݳ���

//		//***********֡���ݴ�������************//
////if((rec_data_u2[0]==0X55)&&(rec_data_u2[1]==0X53))//
////		{
//		
// //   }
//		  angle_1=rec_data_u2[7];
//			angle_2=rec_data_u2[6];
//			angle_z=(angle_1<<8)|angle_2;
//			yaw_2=angle_z/32768.0f*180.0f;

//		//*************************************//
//		USART_ClearITPendingBit(USART2, USART_IT_IDLE);         //����жϱ�־
//		DMA1_S1_Reset();                                       //�ָ�DMAָ�룬�ȴ���һ�εĽ���
//		
//  }

//		
//	else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)
//	{
//		USART_ClearITPendingBit(USART2, USART_IT_TC);

//		DMA1_Stream3->CR &= (uint32_t)(~DMA_SxCR_EN);   //�ر�DMA,�������
//	}
//	
//	
//}
/*
********************************************************************************************************************
*                  void  USART2_IRQHandler(void)
*
*Description : 串口2接收中断（激光雷达）
*Arguments   : none
*Returns     : none
*Notes       : 基于F103 STP-23激光雷达协议
********************************************************************************************************************
*/
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		static uint8_t state = 0;
		static uint8_t crc = 0;
		static uint8_t cnt = 0;
		static uint8_t PACK_FLAG = 0;
		static uint8_t data_len  = 0;
		static uint32_t timestamp = 0;
		static uint8_t state_flag = 1;
		uint8_t temp_data;
		
		Uart2_Receive = USART_ReceiveData(USART2);
		temp_data=Uart2_Receive;
		
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
							Pack2_Data[cnt].distance = (uint16_t)temp_data ;
							crc = crc + temp_data;
							state++;
							break;        
						case 1: 
							Pack2_Data[cnt].distance = ((u16)temp_data<<8) + Pack2_Data[cnt].distance;
							crc = crc + temp_data;
							state++;
							break; 
						case 2:
							Pack2_Data[cnt].noise = (u16)temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 3:
							Pack2_Data[cnt].noise = ((u16)temp_data<<8) + Pack2_Data[cnt].noise;
							crc = crc + temp_data;
							state++;
							break; 
						case 4:
							Pack2_Data[cnt].peak = (u32)temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 5:
							Pack2_Data[cnt].peak = ((u32)temp_data<<8) + Pack2_Data[cnt].peak;
							crc = crc + temp_data;
							state++;
							break; 
						case 6:
							Pack2_Data[cnt].peak = ((u32)temp_data<<16) + Pack2_Data[cnt].peak;	
							crc = crc + temp_data;
							state++;
							break; 
						case 7:
							Pack2_Data[cnt].peak = ((u32)temp_data<<24) + Pack2_Data[cnt].peak;
							crc = crc + temp_data;
							state++;
							break; 
						case 8:
							Pack2_Data[cnt].confidence = temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 9:
							Pack2_Data[cnt].intg = (u32)temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 10:
							Pack2_Data[cnt].intg = ((u32)temp_data<<8) + Pack2_Data[cnt].intg;
							crc = crc + temp_data;
							state++;
							break; 
						case 11:
							Pack2_Data[cnt].intg = ((u32)temp_data<<16) + Pack2_Data[cnt].intg;
							crc = crc + temp_data;
							state++;
							break; 
						case 12:
							Pack2_Data[cnt].intg = ((u32)temp_data<<24) + Pack2_Data[cnt].intg;
							crc = crc + temp_data;
							state++;
							break; 
						case 13:
							Pack2_Data[cnt].reftof = (int16_t)temp_data;
							crc = crc + temp_data;
							state++;
							break; 
						case 14:
							Pack2_Data[cnt].reftof = ((int16_t)temp_data<<8) +Pack2_Data[cnt].reftof;
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
						data2_process();
						receive_cnt2++;
					}
					distance2 = Pack2_Data[0].distance;
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
*                  void DMA1_Stream3_IRQHandler(void)
*
*Description : ����3�����жϺ��� 
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void DMA1_Stream3_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_Stream3,DMA_IT_TCIF3) != RESET)
    {
        /* �����־λ */
        DMA_ClearFlag(DMA1_Stream3,DMA_IT_TCIF3);
        /* �ر�DMA */
        DMA_Cmd(DMA1_Stream3,DISABLE);
        /* �򿪷�������ж�,ȷ�����һ���ֽڷ��ͳɹ� */
        USART_ITConfig(USART2,USART_IT_TC,ENABLE);
    }
}

/*
********************************************************************************************************************
*                  void DMA1_S1_Reset(void)
*
*Description : ����3�������� 
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/

void DMA1_S1_Reset(void)//DMA1_Stream6 ��������
{ 
	DMA_ClearFlag(DMA1_Stream1,DMA_IT_TCIF1|DMA_FLAG_HTIF1|DMA_FLAG_TEIF1|DMA_FLAG_DMEIF1|DMA_FLAG_FEIF1);  
	
	DMA_Cmd(DMA1_Stream1,DISABLE); //�ر�USART2 TX DMA1 ��ָʾ��ͨ��

 	//DMA_SetCurrDataCounter(DMA1_Channel5,Uart1_DMA_Len);//DMAͨ����DMA����Ĵ�С
	DMA1_Stream1->NDTR = U2_DATA_LEN;
 	//DMA_Cmd(DMA1_Channel5, ENABLE);                    
	DMA_Cmd(DMA1_Stream1,ENABLE); //ʹ��USART2 TX DMA1 ��ָʾ��ͨ��
	/* �����־λ */
   
}



/*****************************************************���� TF mini plus*//////////////////////////////////////////////////
/*****************************************************���� TF luna*//////////////////////////////////////////////////
/*****************************************************������115200*//////////////////////////////////////////////////

//u8 Receive_ok=0;

//void USART2_IRQHandler(void)
//{
//	static uint8_t Uart3_Rec_Len = 0;
//	static uint8_t i=0;
//	int t;
//	uint8_t sum=0;
//	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)  //�����ж�
//	{
//		t= USART_ReceiveData(USART2);//��ȡ���� ע�⣺������Ҫ�������ܹ�����жϱ�־λ��
//		Uart3_Rec_Len = U2_DATA_LEN - DMA1_Stream3->NDTR;	//����ӱ�֡���ݳ��
//		GPIO_ResetBits(GPIOE,GPIO_Pin_4 );//GPIOF9,F10���øߣ�����
//     uint16_t distance=0;
//   if(rec_data_u2[0]==0x59)
//		{
//			GPIO_ResetBits(GPIOE,GPIO_Pin_3 );
//			if(rec_data_u2[1]==0x59)
//			{
//				for(i=0;i<Uart3_Rec_Len-1;i++)	
//				{	
//					sum+=rec_data_u2[i];
//				}
//				distance=(rec_data_u2[3]<<8)|rec_data_u2[2];
//		   Receive_ok=0;//����������ϱ�־
//			OLED_Show3FNum(0,15,distance,4,2,12);			
//		  OLED_Refresh_Gram();
//			 
//			}
//		}
//     USART_ClearITPendingBit(USART2, USART_IT_IDLE);         //����жϱ�־
//		DMA1_S1_Reset();                                       //�ָ�DMAָ�룬�ȴ���һ�εĽ���
//    }
//	else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)
//	{
//	
//		USART_ClearITPendingBit(USART2, USART_IT_TC);
//		DMA1_Stream3->CR &= (uint32_t)(~DMA_SxCR_EN);   //�ر�DMA,�������

//	}
//}


/****************************************************ţ�Ƽ���*//////////////////////////////////////////////////////////////////////////
/*****************************************************������9600*//////////////////////////////////////////////////
/*****************************************************��ʾ���жϻ���*////////////////////////////////////////////////// 
//u8 Num = 0;              //�������ݵĵ�ǰλ��
//int a222=0;
//void Read_LaserDis(unsigned char ID, unsigned int *Data)       
//{	
//	unsigned char y=0;
//	unsigned int Receive_data [3] = { 0 };       //���ݻ�����

//	Num = 0; 
/////////////////////////////��ȡ���롢����������������ǿ��ֵ///////////////////////////////
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//	USART_SendData(USART2, 0x57);		

//	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//	USART_SendData(USART2, ID);																  //IDģ����
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
//	delay_ms(2);

//  while(1)
//  {
//		if(rec_data_u2[0] != 0x75) { Num = 0; }  //�ж�֡ͷ0x75,�������½���
//		if(Num == 8)
//		{
//			Num = 0;
//			if(rec_data_u2[7] == 0x07)  //�ж�֡β0x07,���߲���ֵ
//			{
//				Receive_data[0] = rec_data_u2[1];
//				Receive_data[0] <<= 8;
//				Receive_data[0] |= rec_data_u2[2];        
//				*Data = Receive_data[0];          //����
////				a222=rec_data_u2[1]*256+rec_data_u2[2];
////        OLED_ShowNum(50,42,a222,4,12);	
////    		OLED_Refresh_Gram();
////				
//				Receive_data[1] = rec_data_u2[3];
//				Receive_data[1] <<= 8;
//				Receive_data[1] |= rec_data_u2[4];
//				*(Data+1) = Receive_data[1];          //��������
//				
//				Receive_data[2] = rec_data_u2[5];
//				Receive_data[2] <<= 8;
//				Receive_data[2] |= rec_data_u2[6];
//				*(Data+2) = Receive_data[2];         //������ǿ        
//				
//				break;
//			}        
//			break;
//		}
//    else
//    {
//      delay_ms(1);y++;
//      if(y==10) { Num = 0;break; }
//    }
//  }
/////////////////////////////��ȡ���롢����������������ǿ��ֵ///////////////////////////////
//}

//void Set_LaserDis(unsigned char ID, unsigned char Fun,unsigned char Par)	       
//{	
// ///////////////////////////���ù��ܲ���///////////////////////////////	
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//	USART_SendData(USART2, 0x4C);																//������ʼ�ź�
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);	
//	
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//	USART_SendData(USART2, ID);																  //IDģ����
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
//	
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//	USART_SendData(USART2, Fun);																//������
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);	
//	
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//	USART_SendData(USART2, Par);																//����
//	while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);

/////////////////////////////���ù��ܲ���///////////////////////////////		
//}
//extern u16 openmv3[9];
//void USART2_IRQHandler(void)
//{
//	static uint8_t Uart3_Rec_Len = 0;
//   u8 Res;
//	
//	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)  //�����ж�
//	{
//		 Res= USART_ReceiveData(USART2);//��ȡ���� ע�⣺������Ҫ�������ܹ�����жϱ�־λ�	
//		 Uart3_Rec_Len = U2_DATA_LEN - DMA1_Stream3->NDTR;	//����ӱ�֡���ݳ��
//	   GPIO_ResetBits(GPIOE,GPIO_Pin_4 | GPIO_Pin_0|GPIO_Pin_1 | GPIO_Pin_3);//GPIOF9,F10���øߣ�����
//     
//   		openmv3[1]=rec_data_u2[1];
//      openmv3[2]=rec_data_u2[2];
//      openmv3[3]=rec_data_u2[3];
//      openmv3[4]=rec_data_u2[4];
//			openmv3[5]=rec_data_u2[5];
//      openmv3[6]=rec_data_u2[6];
//			openmv3[7]=rec_data_u2[7];

//     USART_ClearITPendingBit(USART2, USART_IT_IDLE);         //����жϱ�־
//		 DMA1_S1_Reset();                                       //�ָ�DMAָ�룬�ȴ���һ�εĽ���
//    }
//	else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)
//	{
//	
//		USART_ClearITPendingBit(USART2, USART_IT_TC);
//		DMA1_Stream3->CR &= (uint32_t)(~DMA_SxCR_EN);   //�ر�DMA,�������

//	}
//}

/*
********************************************************************************************************************
*                  void  data2_process(void)
*
*Description : 数据处理，接收到一帧之后可以进行数据处理
*Arguments   : none
*Returns     : none
*Notes       : 基于F103的数据处理逻辑，20帧数据取平均
********************************************************************************************************************
*/
void data2_process(void)
{
	u8 i;
	static u16 count = 0;
	LidarPointTypedef Pack2_sum;
	for(i=0;i<12;i++)
	{
		if(Pack2_Data[i].distance != 0)
		{
			count++;
			Pack2_sum.distance += Pack2_Data[i].distance;
			Pack2_sum.noise += Pack2_Data[i].noise;
			Pack2_sum.peak += Pack2_Data[i].peak;
			Pack2_sum.confidence += Pack2_Data[i].confidence;
			Pack2_sum.intg += Pack2_Data[i].intg;
			Pack2_sum.reftof += Pack2_Data[i].reftof;
		}
	}
	if(count !=0)
	{
		distance2 = Pack2_sum.distance/count;
		noise2 = Pack2_sum.noise/count;
		peak2 = Pack2_sum.peak/count;
		confidence2 = Pack2_sum.confidence/count;
		intg2 = Pack2_sum.intg/count;
		reftof2 = Pack2_sum.reftof/count;
		Pack2_sum.distance = 0;
		Pack2_sum.noise = 0;
		Pack2_sum.peak = 0;
		Pack2_sum.confidence = 0;
		Pack2_sum.intg = 0;
		Pack2_sum.reftof = 0;
		count = 0;
	}
}

void USART2_send_byte(uint8_t byte)
{
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);//�ȴ��������
	USART2->DR=byte;	
}

//���Ͷ��ֽ�����
void USART2_Send_bytes(uint8_t *Buffer, uint8_t Length)
{
	uint8_t i=0;
	while(i<Length)
	{
		USART2_send_byte(Buffer[i++]);
	}
}
//���Ͷ��ֽ�����+У���
void USART2_Send(uint8_t *Buffer, uint8_t Length)
{
	uint8_t i=0;
	while(i<Length)
	{
		if(i<(Length-1))
		Buffer[Length-1]+=Buffer[i];//�ۼ�Length-1ǰ������
		USART2_send_byte(Buffer[i++]);
	}
}

//void send_com(u8 data)
//{
//	u8 bytes[3]={0};
//	bytes[0]=0xa5;
//	bytes[1]=data;//�����ֽ�
//	USART2_Send(bytes,3);//����֡ͷ�������ֽڡ�У���
//}


///************************************************************
//*  ��ຯ�� 
//*  ע�⣺�ڳ�ʼ����ɱ���Ҫ����д���� �磺send_com(0x45);
//*   0x45: ���������������      
//*   0x50: ���������ģʽ
//*   0x51�����ٲ���ģʽ
//*   0x52���߾��Ȳ���ģʽ
//*   0x53: һ�����ģʽ
//************************************************************/
//void Mea_dis(void)
//{
//	u8 sum=0,i=0;
//	uint16_t distance=0;
//	uint16_t Strength=0;
//	double dis=0.0;
//	if(Receive_ok)//���ڽ������
//		{
//			distance=(rec_data_u2[3]<<8)|rec_data_u2[2];
//		   Receive_ok=0;//����������ϱ�־
//			OLED_Show3FNum(0,15,distance,4,2,12);			
//		  OLED_Refresh_Gram();
//			
//	 }
//}
/***************************************************************���� TF mini plus*/////////////////////////////////////////////////////////////////////////


/*
********************************************************************************************************************
*                  void DMA1_Stream3_IRQHandler(void)
*
*Description : ����3�����жϺ��� 
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/


