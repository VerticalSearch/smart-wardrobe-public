#include "usart3dma.h"
#include "usart4dma.h"
#include "delay.h"

//雷达4接收数据
uint8_t Uart4_Receive;
//激光雷达处理后的各个参数
uint16_t receive_cnt4;
uint16_t distance4;
LiDARFrameTypeDef Pack4_Data;/* 雷达接收到的数据存储，12个点 */


/*
********************************************************************************************************************
*                  void  uart4_init(uint32_t bound)
*
*Description : 串口4的初始化 
*							 UART4_RX---PC11
*							 UART4_TX---PC10
*Arguments   : bound波特率参数
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void uart4_init(uint32_t bound)
{  	 
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
		
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);	 //使能GPIOC时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE); //使能UART4时钟
	
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4); //PC10复用为UART4_TX
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART4); //PC11复用为UART4_RX
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;            //复用模式
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;          //推挽输出
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;       //速度50MHZ
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;            //上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);  		          //初始化
	
  //Uart4 NVIC配置
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;
	//响应优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		
	//使能IRQ通道
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;	
  //用指定的参数初始化VIC寄存器
	NVIC_Init(&NVIC_InitStructure);
	
  //UART4 初始化设置
	USART_InitStructure.USART_BaudRate = bound; //波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1; //一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No; //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx;	//接收模式
  USART_Init(UART4, &USART_InitStructure);      //初始化串口4
  USART_ITConfig(UART4, USART_IT_RXNE, ENABLE); //开启串口接受中断
  USART_Cmd(UART4, ENABLE);                     //使能串口4 
}

/*
********************************************************************************************************************
*                  void  UART4_IRQHandler(void)
*
*Description : 串口4接收中断
*Arguments   : none
*Returns     : none
*Notes       : 基于F103 STP-23激光雷达协议
********************************************************************************************************************
*/
void UART4_IRQHandler(void)
{	
	if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET) //判断是否接收到数据
	{	  
		static uint8_t state = 0;	//状态位	
		static uint8_t crc = 0;		//校验位
		static uint8_t cnt = 0;		//接收一帧12个点的计数
		uint8_t temp_data;
		
		Uart4_Receive = USART_ReceiveData(UART4); 
		temp_data = Uart4_Receive;
		USART_ClearITPendingBit(UART4,USART_IT_RXNE);
		
		if (state > 5)
		{
			if(state < 42)
			{
				if(state%3 == 0)//一帧数据中的第6,9.....39个数据，距离值低8位
				{
					Pack4_Data.point[cnt].distance = (uint16_t)temp_data;
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
				}
				else if(state%3 == 1)//一帧数据中的第7,10.....40个数据，距离值高8位
				{
					Pack4_Data.point[cnt].distance = ((uint16_t)temp_data<<8)+Pack4_Data.point[cnt].distance;
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
				}
				else//一帧数据中的第8,11.....41个数据，信号强度
				{
					Pack4_Data.point[cnt].intensity = temp_data;
					cnt++;	
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
				}
			}
			else 
			{
				switch(state)
				{
					case 42:
						Pack4_Data.end_angle = (uint16_t)temp_data;//结束角度的8位
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
						break;
					case 43:
						Pack4_Data.end_angle = ((uint16_t)temp_data<<8)+Pack4_Data.end_angle;//结束角度高8位
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
						break;
					case 44:
						Pack4_Data.timestamp = (uint16_t)temp_data;//时间戳低8位
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
						break;
					case 45:
						Pack4_Data.timestamp = ((uint16_t)temp_data<<8)+Pack4_Data.timestamp;//时间戳高8位
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
						break;
					case 46:
						Pack4_Data.crc8 = temp_data;//雷达传输校验位
						if(Pack4_Data.crc8 == crc)//校验正确
						{
							data4_process();//接收到一帧，校验正确，可以进行数据处理
							receive_cnt4++;//统计接收到正确数据的次数
						}
						else
						{
						//校验不正确
						}
						crc = 0;
						state = 0;
						cnt = 0;//复位
					default: break;
				}
			}
		}
		else 
		{
			switch(state)
			{
				case 0:
					if(temp_data == HEADER)//头固定
					{
						Pack4_Data.header = temp_data;
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];//开始计算校验
					} else {
						state = 0;
						crc = 0;
					}
					break;
				case 1:
					if(temp_data == VERLEN)//数据位的点数，目前固定
					{
						Pack4_Data.ver_len = temp_data;
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
					} else {
						state = 0;
						crc = 0;
					}
					break;
				case 2:
					Pack4_Data.temperature = (uint16_t)temp_data;//温度的8位，一个16位ADC，0--4096对应温度
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
					break;
				case 3:
					Pack4_Data.temperature = ((uint16_t)temp_data<<8)+Pack4_Data.temperature;//温度高8位
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
					break;
				case 4:
					Pack4_Data.start_angle = (uint16_t)temp_data;//开始角度的8位，放大100倍
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
					break;
				case 5:
					Pack4_Data.start_angle = ((uint16_t)temp_data<<8)+Pack4_Data.start_angle;
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
					break;
				default: break;
			}
		}
	}		
} 

/*
********************************************************************************************************************
*                  void  data4_process(void)
*
*Description : 数据处理，接收到一帧之后可以进行数据处理
*Arguments   : none
*Returns     : none
*Notes       : 基于F103的数据处理逻辑，20帧数据取平均
********************************************************************************************************************
*/
void data4_process(void)
{
	static uint8_t cnt = 0;
	uint8_t i;
	static uint16_t count = 0;
	static uint32_t sum = 0;
	
	for(i=0;i<12;i++)//12个点取平均
	{
		if(Pack4_Data.point[i].distance != 0)//去掉0的点
		{
			count++;
			sum += Pack4_Data.point[i].distance;
		}
	}
	
	if(++cnt == 20)//20帧数据取一次距离
	{
		if(count > 0)
		{
			distance4 = sum/count;
		}
		sum = 0;
		count = 0;
		cnt = 0;
	}
}



