//#include "usart5dma.h"
//#include "oled.h"
//#include "delay.h"
//#include "usart4dma.h"
////串口5接收数据
//uint8_t Uart5_Receive;
////测距模块检测处理后的各个数据
//uint16_t receive_cnt5;
//uint8_t confidence5;
//uint16_t distance5,noise5,reftof5;
//uint32_t peak5,intg5;
//LidarPointTypedef Pack5_Data[12];/* 雷达接收的数据储存在这个变量之中 */
//void USART5_Init(int Baud)
//{
//	NVIC_InitTypeDef NVIC_InitStructure ;//定义中断结构体
//	GPIO_InitTypeDef GPIO_InitStructure;//定义IO初始化结构体
//	USART_InitTypeDef USART_InitStructure;//定义串口结构体
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);//打开串口对应的外设时钟
//	//初始化串口参数
//	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//	USART_InitStructure.USART_StopBits = USART_StopBits_1;
//	USART_InitStructure.USART_Parity = USART_Parity_No;
//	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
//	USART_InitStructure.USART_BaudRate = Baud;
//	//初始化串口
//	USART_Init(UART5,&USART_InitStructure);
//	//配置中断
//	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;               //通道设置为串口中断
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;       //中断占先等级
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;              //中断响应优先级
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                 //打开中断
//	NVIC_Init(&NVIC_InitStructure);
//	//中断配置
//	USART_ITConfig(UART5,USART_IT_RXNE,ENABLE);
//	//启动串口
//	USART_Cmd(UART5, ENABLE);
//	//设置IO口时钟
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
//    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
//	GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_UART5);
//	GPIO_PinAFConfig(GPIOD,GPIO_PinSource2,GPIO_AF_UART5);

//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;	 //管脚模式:输出口
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	    //类型:推挽模式
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	 //上拉下拉设置
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//IO口速度
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;  //管脚指定
//	GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化

//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;	//管脚模式:输入口
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;	 //上拉下拉设置
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;    //管脚指定
//	GPIO_Init(GPIOC, &GPIO_InitStructure);      //初始化
//}

//void UART5_IRQHandler(void)
//{	
//	if(USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)
//	{
//		static uint8_t state = 0;			//状态位	
//		static uint8_t crc = 0;				//校验和
//		static uint8_t cnt = 0;				//用于一帧12个点的计数
//		static uint8_t PACK_FLAG = 0;  //命令标志位
//		static uint8_t data_len  = 0;  //数据长度
//		static uint32_t timestamp = 0; //时间戳
//		static uint8_t state_flag = 1; //转入数据接收标志位
//		uint8_t temp_data;
//		
//		Uart5_Receive = USART_ReceiveData(UART5); //Read the data //读取数据
//		temp_data=Uart5_Receive;
//		
//		if(state< 4) 																					 /* 起始符验证 前4个数据均为0xAA */
//		{                                          
//				if(temp_data == HEADER) 
//				{
//					state ++;
//				}
//				else 
//				{
//					state = 0;
//				}
//		}
//		else if(state<10&&state>3)
//		{
//				switch(state)
//				{
//						case 4:   
//							if(temp_data == device_address)              /* 设备地址验证 */
//							{							
//											state ++;
//											crc = crc + temp_data;									
//											break;        
//							} 
//							else state = 0,crc = 0;
//						case 5:   
//							if(temp_data == PACK_GET_DISTANCE)					 /* 获取测量数据命令 */
//							{  
//											PACK_FLAG = PACK_GET_DISTANCE;
//											state ++;	
//											crc = crc + temp_data;	
//											break;									
//							}		 

//							else if(temp_data == PACK_RESET_SYSTEM) 		 /* 复位命令 */
//							{
//											PACK_FLAG = PACK_RESET_SYSTEM;
//											state ++; 
//											crc = crc + temp_data;	
//											break;	
//							}
//							else if(temp_data == PACK_STOP)							 /* 停止测量数据传输命令 */
//							{ 
//											PACK_FLAG = PACK_STOP;
//											state ++; 
//											crc = crc + temp_data;	
//											break;
//							}
//							else if(temp_data == PACK_ACK)							 /* 应答码命令 */
//							{  
//											PACK_FLAG = PACK_ACK;
//											state ++;
//											crc = crc + temp_data;	
//											break;
//							}			 				 
//							else if(temp_data == PACK_VERSION)					 /* 获取传感器信息命令 */
//							{ 
//											PACK_FLAG = PACK_VERSION,
//											state ++,
//											crc = crc + temp_data;	   	     
//											break;
//							}
//							else state = 0,crc = 0;
//						case 6: if(temp_data == chunk_offset)          /* 偏移地址 */
//										{  
//											state ++;
//											crc = crc + temp_data;
//											break; 	  
//										}	
//										else state = 0,crc = 0;
//						case 7: if(temp_data == chunk_offset)
//										{  
//											state ++;
//											crc = crc + temp_data;
//											break;
//										}
//										else state = 0,crc = 0;
//						case 8: 
//								data_len = (u16)temp_data;								 /* 数据长度低八位 */
//								state ++; 
//								crc = crc + temp_data;
//								break;																			 
//						case 9: 
//								data_len = data_len + ((u16)temp_data<<8); 			 /* 数据长度高八位 */
//								state ++;
//								crc = crc + temp_data;
//								break; 
//						default: break;
//				}
//		}
//		else if(state == 10 ) state_flag = 0;                    /*由switch跳出来时state为10，但temp_data仍为距离长度高八位数据，需跳过一次中断*/
//		if(PACK_FLAG == PACK_GET_DISTANCE&&state_flag == 0)      /* 获取一帧数据并校验 */
//		{
//				if(state>9)
//				{
//						if(state<190)
//						{
//								static uint8_t state_num;
//								state_num = (state-10)%15;
//								switch(state_num)
//								{
//										case 0: 
//											Pack5_Data[cnt].distance = (uint16_t)temp_data ;				 /* 距离数据低八位 */
//											crc = crc + temp_data;
//											state++;
//											break;        
//										case 1: 
//											Pack5_Data[cnt].distance = ((u16)temp_data<<8) + Pack5_Data[cnt].distance;	 /* 距离数据 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 2:
//											Pack5_Data[cnt].noise = (u16)temp_data;				 /* 环境噪音低八位 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 3:
//											Pack5_Data[cnt].noise = ((u16)temp_data<<8) + Pack5_Data[cnt].noise;				 /* 环境噪音 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 4:
//											Pack5_Data[cnt].peak = (u32)temp_data;				 										 /* 接受强度信息低八位 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 5:
//											Pack5_Data[cnt].peak = ((u32)temp_data<<8) + Pack5_Data[cnt].peak;
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 6:
//											Pack5_Data[cnt].peak = ((u32)temp_data<<16) + Pack5_Data[cnt].peak;	
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 7:
//											Pack5_Data[cnt].peak = ((u32)temp_data<<24) + Pack5_Data[cnt].peak;				    /* 接受强度信息 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 8:
//											Pack5_Data[cnt].confidence = temp_data;				 /* 置信度 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 9:
//											Pack5_Data[cnt].intg = (u32)temp_data;															/* 积分次数低八位 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 10:
//											Pack5_Data[cnt].intg = ((u32)temp_data<<8) + Pack5_Data[cnt].intg;
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 11:
//											Pack5_Data[cnt].intg = ((u32)temp_data<<16) + Pack5_Data[cnt].intg;
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 12:
//											Pack5_Data[cnt].intg = ((u32)temp_data<<24) + Pack5_Data[cnt].intg;				  	 /* 积分次数 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 13:
//											Pack5_Data[cnt].reftof = (int16_t)temp_data;				 								 /* 温度表征值低八位 */
//											crc = crc + temp_data;
//											state++;
//											break; 
//										case 14:
//											Pack5_Data[cnt].reftof = ((int16_t)temp_data<<8) +Pack5_Data[cnt].reftof;			/* 温度表征值 */
//											crc = crc + temp_data;
//											state++;
//											cnt++;							 /* 进入下一个测量点 */
//											break; 
//										default: break;
//								}
//					}
//								/* 时间戳 */
//								if(state == 190) timestamp = temp_data,state++,crc = crc + temp_data;
//								else if(state == 191) timestamp = ((u32)temp_data<<8) + timestamp,state++,crc = crc + temp_data; 
//								else if(state == 192) timestamp = ((u32)temp_data<<16) + timestamp,state++,crc = crc + temp_data;
//								else if(state == 193) timestamp = ((u32)temp_data<<24) + timestamp,state++,crc = crc + temp_data; 
//								else if(state==194)
//								{
//											if(temp_data == crc)   /* 校验成功 */
//											{
//													data5_process();  	 /* 数据处理函数，完成一帧之后可进行数据处理 */
//													receive_cnt5++;	 	 /* 输出接收到正确数据的次数 */
//											}
//											distance5 = Pack5_Data[0].distance;
//											crc = 0;
//											state = 0;
//											state_flag = 1;
//											cnt = 0; 							 /* 复位*/
//								}
//					
//				}
//		}
//	}
//	
//}
//void data5_process(void)/*数据处理函数，完成一帧之后可进行数据处理*/
//{
//		/* 计算距离 */
////		static u8 cnt = 0;
//		u8 i;
//		static u16 count = 0;
////		static u32 sum = 0;
//		LidarPointTypedef Pack5_sum;
//		for(i=0;i<12;i++)									/* 12个点取平均 */
//		{
//				if(Pack5_Data[i].distance != 0)  /* 去除0的点 */
//				{
//						count++;
//						Pack5_sum.distance += Pack5_Data[i].distance;
//						Pack5_sum.noise += Pack5_Data[i].noise;
//						Pack5_sum.peak += Pack5_Data[i].peak;
//						Pack5_sum.confidence += Pack5_Data[i].confidence;
//						Pack5_sum.intg += Pack5_Data[i].intg;
//						Pack5_sum.reftof += Pack5_Data[i].reftof;
//				}
//		}
//		if(count !=0)
//		{

//					distance5 = Pack5_sum.distance/count;
//					noise5 = Pack5_sum.noise/count;
//					peak5 = Pack5_sum.peak/count;
//					confidence5 = Pack5_sum.confidence/count;
//					intg5 = Pack5_sum.intg/count;
//					reftof5 = Pack5_sum.reftof/count;
//					Pack5_sum.distance = 0;
//					Pack5_sum.noise = 0;
//					Pack5_sum.peak = 0;
//					Pack5_sum.confidence = 0;
//					Pack5_sum.intg = 0;
//					Pack5_sum.reftof = 0;
//					count = 0;
//		}
//}
