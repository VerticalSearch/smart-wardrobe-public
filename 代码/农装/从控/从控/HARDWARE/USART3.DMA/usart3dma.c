#include "usart3dma.h"
#include "delay.h"
#include "laser_common.h"

//雷达3接收数据
uint8_t Uart3_Receive;
//激光雷达处理后的各个参数
uint16_t receive_cnt3;
uint16_t distance3;
LiDARFrameTypeDef Pack3_Data;/* 雷达接收到的数据存储，12个点 */

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
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
		
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);	 //使能GPIOD时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); //使能USART3时钟
	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_USART3); //PD8复用为USART3_TX
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_USART3); //PD9复用为USART3_RX
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;            //复用模式
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;          //推挽输出
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;      //速度100MHz（高波特率需要更快的GPIO速度）
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;            //上拉
	GPIO_Init(GPIOD, &GPIO_InitStructure);  		          //初始化
	
  //Uart3 NVIC配置
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;
	//响应优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		
	//使能IRQ通道
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;	
  //用指定的参数初始化VIC寄存器
	NVIC_Init(&NVIC_InitStructure);
	
  //USART3 初始化设置
	USART_InitStructure.USART_BaudRate = bound; //波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1; //一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No; //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx;	//接收模式
  USART_Init(USART3, &USART_InitStructure);      //初始化串口3
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); //开启串口接受中断
  USART_Cmd(USART3, ENABLE);                     //使能串口3 
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
*                  void  uart3_send_test_data(void)
*
*Description : 串口3发送测试数据
*Arguments   : none
*Returns     : none
*Notes       : 发送固定的测试数据序列
********************************************************************************************************************
*/
void uart3_send_test_data(void)
{
	static uint8_t test_counter = 0;
	
	// 发送简化的测试数据序列
	uart3_send_byte(0x54);  // 帧头
	uart3_send_byte(0x2C);  // 版本长度
	uart3_send_byte(test_counter);  // 测试计数
	
	test_counter++;
	if(test_counter > 250) test_counter = 0; // 防止溢出
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
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) //判断是否接收到数据
	{	  
		static uint8_t state = 0;	//状态位	
		static uint8_t crc = 0;		//校验位
		static uint8_t cnt = 0;		//接收一帧12个点的计数
		uint8_t temp_data;
		
		Uart3_Receive = USART_ReceiveData(USART3); 
		temp_data = Uart3_Receive;
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);
		
		if (state > 5)
		{
			if(state < 42)
			{
				if(state%3 == 0)//一帧数据中的第6,9.....39个数据，距离值低8位
				{
					Pack3_Data.point[cnt].distance = (uint16_t)temp_data;
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
				}
				else if(state%3 == 1)//一帧数据中的第7,10.....40个数据，距离值高8位
				{
					Pack3_Data.point[cnt].distance = ((uint16_t)temp_data<<8)+Pack3_Data.point[cnt].distance;
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
				}
				else//一帧数据中的第8,11.....41个数据，信号强度
				{
					Pack3_Data.point[cnt].intensity = temp_data;
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
						Pack3_Data.end_angle = (uint16_t)temp_data;//结束角度的8位
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
						break;
					case 43:
						Pack3_Data.end_angle = ((uint16_t)temp_data<<8)+Pack3_Data.end_angle;//结束角度高8位
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
						break;
					case 44:
						Pack3_Data.timestamp = (uint16_t)temp_data;//时间戳低8位
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
						break;
					case 45:
						Pack3_Data.timestamp = ((uint16_t)temp_data<<8)+Pack3_Data.timestamp;//时间戳高8位
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
						break;
					case 46:
						Pack3_Data.crc8 = temp_data;//雷达传输校验位
						if(Pack3_Data.crc8 == crc)//校验正确
						{
							data3_process();//接收到一帧，校验正确，可以进行数据处理
							receive_cnt3++;//统计接收到正确数据的次数
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
						Pack3_Data.header = temp_data;
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
						Pack3_Data.ver_len = temp_data;
						state++;
						crc = CrcTable[(crc^temp_data) & 0xff];
					} else {
						state = 0;
						crc = 0;
					}
					break;
				case 2:
					Pack3_Data.temperature = (uint16_t)temp_data;//温度的8位，一个16位ADC，0--4096对应温度
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
					break;
				case 3:
					Pack3_Data.temperature = ((uint16_t)temp_data<<8)+Pack3_Data.temperature;//温度高8位
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
					break;
				case 4:
					Pack3_Data.start_angle = (uint16_t)temp_data;//开始角度的8位，放大100倍
					state++;
					crc = CrcTable[(crc^temp_data) & 0xff];
					break;
				case 5:
					Pack3_Data.start_angle = ((uint16_t)temp_data<<8)+Pack3_Data.start_angle;
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
	static uint8_t cnt = 0;
	uint8_t i;
	static uint16_t count = 0;
	static uint32_t sum = 0;
	
	for(i=0;i<12;i++)//12个点取平均
	{
		if(Pack3_Data.point[i].distance != 0)//去掉0的点
		{
			count++;
			sum += Pack3_Data.point[i].distance;
		}
	}
	
	if(++cnt == 20)//20帧数据取一次距离
	{
		if(count > 0)
		{
			distance3 = sum/count;
		}
		sum = 0;
		count = 0;
		cnt = 0;
	}
}


