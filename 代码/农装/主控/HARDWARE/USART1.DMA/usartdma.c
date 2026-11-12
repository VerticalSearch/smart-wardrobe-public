/*
******************************************************************************************************************
*Filename      :usart1_dma.c
*Programmer(s) :嵌入式开发工程师
*Description   :主控板UART1 DMA通信实现
*               主板向从板发送指令，接收从板传感器数据
*               USART1_RX---PB7
*               USART1_TX---PB6
******************************************************************************************************************
*/
#include "usartdma.h"
#include "delay.h"

// 全局变量定义
u8 rec_data_u1[UART1_DATA_LEN];   // 接收缓冲区
u8 send_data_u1[UART1_DATA_LEN];  // 发送缓冲区
SensorData_t sensor_data = {0};   // 传感器数据
uint8_t cmd_received_flag = 0;    // 命令接收标志

/*
********************************************************************************************************************
*                  void USART1_Init(int Baud)
*
*Description : UART1初始化函数，配置DMA传输
*Arguments   : Baud - 波特率
*Returns     : none
*Notes       : 使用DMA2_Stream5接收，DMA2_Stream7发送
********************************************************************************************************************
*/
void USART1_Init(int Baud)
{
    NVIC_InitTypeDef NVIC_InitStructure;  // 中断初始化结构体
    GPIO_InitTypeDef GPIO_InitStructure;  // GPIO初始化结构体
    USART_InitTypeDef USART_InitStructure;// 串口初始化结构体
    DMA_InitTypeDef DMA_InitStructure;    // DMA初始化结构体

    // 使能USART1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    //**********************配置发送DMA，DMA2_Stream7**************************
    // 使能DMA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    // DMA发送中断配置
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // DMA发送通道配置
    DMA_DeInit(DMA2_Stream7);
    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)send_data_u1;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_InitStructure.DMA_BufferSize = UART1_DATA_LEN;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    // 初始化DMA发送通道
    DMA_Init(DMA2_Stream7, &DMA_InitStructure);
    // 使能发送中断
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
    // 使能串口的DMA发送接口
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    // 关闭发送通道
    DMA_Cmd(DMA2_Stream7, DISABLE);

    //**********************配置接收DMA，DMA2_Stream5**************************
    // DMA接收通道配置
    DMA_DeInit(DMA2_Stream5);
    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rec_data_u1;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = UART1_DATA_LEN;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;

    // 初始化DMA接收通道
    DMA_Init(DMA2_Stream5, &DMA_InitStructure);
    // 使能接收通道
    DMA_Cmd(DMA2_Stream5, ENABLE);
    // 使能串口的DMA接收
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

    // 初始化串口参数
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_BaudRate = Baud;
    // 初始化串口
    USART_Init(USART1, &USART_InitStructure);

    // 配置中断
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 使能DMA模式发送
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    // 使能DMA模式接收
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

    // 中断配置
    USART_ITConfig(USART1, USART_IT_TC, DISABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

    // 使能串口
    USART_Cmd(USART1, ENABLE);

    // 配置IO时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // 复用模式:串口
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   // 输出:推挽模式
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;     // 上拉下拉:上拉
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // IO速度
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;        // 复用引脚
    GPIO_Init(GPIOB, &GPIO_InitStructure);           // 初始化

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // 复用模式:串口
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // 上拉下拉:无
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;        // 复用引脚
    GPIO_Init(GPIOB, &GPIO_InitStructure);           // 初始化
}

/*
********************************************************************************************************************
*                  void USART1_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize)
*
*Description : UART1 DMA发送函数
*Arguments   : BufferSRC - 发送数据地址，BufferSize - 发送数据字节数
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void USART1_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize)
{
    // 检查DMA是否正在发送（非阻塞检查）
    if(DMA2_Stream7->CR & DMA_SxCR_EN) {
        // DMA还在发送中，直接返回，不发送新命令
        return;
    }
    
    DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7);
    DMA2_Stream7->NDTR = BufferSize;    // 设置BUFFER大小
    DMA2_Stream7->M0AR = (uint32_t)BufferSRC;   // 设置地址
    DMA2_Stream7->CR |= DMA_SxCR_EN;    // 使能DMA，开始传输
}

/*
********************************************************************************************************************
*                  void USART_Senddatas(USART_TypeDef* USARTxx, u8* addr, int size)
*
*Description : 串口发送数据函数
*Arguments   : USARTxx - 串口，addr - 数据地址，size - 数据大小
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void USART_Senddatas(USART_TypeDef* USARTxx, u8* addr, int size)
{
    while(size--) // 判断数据发送完没有
    {
        while(USART_GetFlagStatus(USARTxx, USART_FLAG_TC) == RESET); // 等待上一个byte数据发送完成
        USART_SendData(USARTxx, *addr); // 调用STM32标准库函数发送数据
        addr++; // 地址偏移
    }
}

/*
********************************************************************************************************************
*                  void Master_SendCommand(uint8_t cmd_type, uint8_t param1, uint8_t param2)
*
*Description : 主控发送命令函数
*Arguments   : cmd_type - 命令类型，param1 - 参数1，param2 - 参数2
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void Master_SendCommand(uint8_t cmd_type, uint8_t param1, uint8_t param2)
{
    // 直接构造命令数据包（5字节），避免结构体对齐问题
    send_data_u1[0] = CMD_HEADER;      // 帧头
    send_data_u1[1] = cmd_type;         // 命令类型
    send_data_u1[2] = param1;           // 参数1
    send_data_u1[3] = param2;           // 参数2
    send_data_u1[4] = CalculateChecksum(send_data_u1, 4);  // 校验和
    
    // 使用DMA发送命令（固定5字节）
    USART1_DMATransfer((uint32_t*)send_data_u1, 5);
}

/*
********************************************************************************************************************
*                  void Master_ProcessReceivedData(void)
*
*Description : 主控处理接收到的传感器数据
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void Master_ProcessReceivedData(void)
{
    if(cmd_received_flag)
    {
        cmd_received_flag = 0;
        
        // 解析从控发送的传感器数据（仅陀螺仪数据）
        // 解析陀螺仪数据（yaw，度*10）
        // 先组合成uint16_t，再转换为int16_t，确保符号扩展正确
        sensor_data.yaw = (int16_t)((uint16_t)rec_data_u1[1] << 8 | rec_data_u1[2]);
        
        // 这里可以添加数据处理逻辑
        // 例如：根据陀螺仪数据进行角度修正
    }
}

/*
********************************************************************************************************************
*                  uint8_t CalculateChecksum(uint8_t* data, uint8_t len)
*
*Description : 计算校验和
*Arguments   : data - 数据指针，len - 数据长度
*Returns     : 校验和
*Notes       : none
********************************************************************************************************************
*/
uint8_t CalculateChecksum(uint8_t* data, uint8_t len)
{
    uint8_t checksum = 0;
    for(uint8_t i = 0; i < len; i++)
    {
        checksum ^= data[i];
    }
    return checksum;
}

/*
********************************************************************************************************************
*                  void USART1_IRQHandler(void)
*
*Description : UART1中断处理函数
*Arguments   : none
*Returns     : none
*Notes       : 处理IDLE中断，接收从板传感器数据
********************************************************************************************************************
*/
void USART1_IRQHandler(void)
{
    static uint8_t UART1_Rec_Len = 0;

    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  // 空闲中断
    {
        USART_ReceiveData(USART1);  // 读取数据，清除中断标志
        UART1_Rec_Len = UART1_DATA_LEN - DMA2_Stream5->NDTR;   // 计算接收数据长度
        
        // 检查数据完整性
        if(UART1_Rec_Len >= 4) // 传感器数据包长度（帧头1字节 + yaw 2字节 + 校验和1字节 = 4字节）
        {
            // 验证帧头
            if(rec_data_u1[0] == CMD_HEADER)
            {
                // 验证校验和
                uint8_t checksum = CalculateChecksum(rec_data_u1, UART1_Rec_Len - 1);
                if(checksum == rec_data_u1[UART1_Rec_Len - 1])
                {
                    cmd_received_flag = 1; // 设置接收标志
                }
            }
        }
        
        DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5);  // 清除接收DMA标志
        DMA2_S5_Reset(); // 恢复DMA指针，等待下一次接收
    }
    else if(USART_GetITStatus(USART1, USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_TC);
        DMA2_Stream7->CR &= (uint32_t)(~DMA_SxCR_EN);   // 关闭DMA，发送完成
    }
}

/*
********************************************************************************************************************
*                  void DMA2_Stream7_IRQHandler(void)
*
*Description : DMA2_Stream7发送完成中断处理函数
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void DMA2_Stream7_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) != RESET)
    {
        /* 清除标志位 */
        DMA_ClearFlag(DMA2_Stream7, DMA_IT_TCIF7);
        /* 关闭DMA */
        DMA_Cmd(DMA2_Stream7, DISABLE);
        /* 打开发送完成中断，确保最后一个字节发送成功 */
        USART_ITConfig(USART1, USART_IT_TC, ENABLE);
    }
}

/*
********************************************************************************************************************
*                  void DMA2_S5_Reset(void)
*
*Description : DMA2_Stream5接收重置函数
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void DMA2_S5_Reset(void)
{ 
    DMA_ClearFlag(DMA2_Stream5, DMA_IT_TCIF5|DMA_FLAG_HTIF5|DMA_FLAG_TEIF5|DMA_FLAG_DMEIF5|DMA_FLAG_FEIF5);  
    
    DMA_Cmd(DMA2_Stream5, DISABLE); // 关闭USART1 RX DMA通道

    DMA2_Stream5->NDTR = UART1_DATA_LEN;
    DMA_Cmd(DMA2_Stream5, ENABLE); // 使能USART1 RX DMA通道
}
