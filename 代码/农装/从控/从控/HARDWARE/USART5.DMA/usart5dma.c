/*
******************************************************************************************************************
*Filename      :usart5_dma.c
*Programmer(s) :嵌入式开发工程师
*Description   :从控板UART5 DMA通信实现
*               接收OPENMV发送的X、Y坐标和面积数据
*               UART5_RX---PC12
*               UART5_TX---PD2
******************************************************************************************************************
*/
#include "usart5dma.h"
#include "delay.h"

// 函数声明
void DMA1_S0_Reset(void);

// 全局变量定义
u8 rec_data_u5[UART5_DATA_LEN];   // 接收缓冲区
u8 send_data_u5[UART5_DATA_LEN];  // 发送缓冲区
OpenMVData_t openmv_data = {0};   // OPENMV数据
uint8_t openmv_data_ready = 0;    // 数据就绪标志

/*
********************************************************************************************************************
*                  void USART5_Init(int Baud)
*
*Description : UART5初始化函数，配置DMA传输
*Arguments   : Baud - 波特率
*Returns     : none
*Notes       : 使用DMA1_Stream0接收，DMA1_Stream7发送
********************************************************************************************************************
*/
void USART5_Init(int Baud)
{
    NVIC_InitTypeDef NVIC_InitStructure;  // 中断初始化结构体
    GPIO_InitTypeDef GPIO_InitStructure;  // GPIO初始化结构体
    USART_InitTypeDef USART_InitStructure;// 串口初始化结构体
    DMA_InitTypeDef DMA_InitStructure;    // DMA初始化结构体

    // 使能UART5时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

    //**********************配置发送DMA，DMA1_Stream7**************************
    // 使能DMA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    // DMA发送中断配置
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // DMA发送通道配置
    DMA_DeInit(DMA1_Stream7);
    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&UART5->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)send_data_u5;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_InitStructure.DMA_BufferSize = UART5_DATA_LEN;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    // 初始化DMA发送通道
    DMA_Init(DMA1_Stream7, &DMA_InitStructure);
    // 使能发送中断
    DMA_ITConfig(DMA1_Stream7, DMA_IT_TC, ENABLE);
    // 使能串口的DMA发送接口
    USART_DMACmd(UART5, USART_DMAReq_Tx, ENABLE);
    // 关闭发送通道
    DMA_Cmd(DMA1_Stream7, DISABLE);

    //**********************配置接收DMA，DMA1_Stream0**************************
    // DMA接收通道配置
    DMA_DeInit(DMA1_Stream0);
    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&UART5->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rec_data_u5;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = UART5_DATA_LEN;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;

    // 初始化DMA接收通道
    DMA_Init(DMA1_Stream0, &DMA_InitStructure);
    // 使能接收通道
    DMA_Cmd(DMA1_Stream0, ENABLE);
    // 使能串口的DMA接收
    USART_DMACmd(UART5, USART_DMAReq_Rx, ENABLE);

    // 初始化串口参数
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_BaudRate = Baud;
    // 初始化串口
    USART_Init(UART5, &USART_InitStructure);

    // 配置中断
    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 使能DMA模式发送
    USART_DMACmd(UART5, USART_DMAReq_Tx, ENABLE);
    // 使能DMA模式接收
    USART_DMACmd(UART5, USART_DMAReq_Rx, ENABLE);

    // 中断配置
    USART_ITConfig(UART5, USART_IT_TC, DISABLE);
    USART_ITConfig(UART5, USART_IT_RXNE, DISABLE);
    USART_ITConfig(UART5, USART_IT_TXE, DISABLE);
    USART_ITConfig(UART5, USART_IT_IDLE, ENABLE);

    // 使能串口
    USART_Cmd(UART5, ENABLE);

    // 配置IO时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_UART5);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_UART5);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // 复用模式:串口
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   // 输出:推挽模式
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;     // 上拉下拉:上拉
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // IO速度
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;        // 复用引脚
    GPIO_Init(GPIOD, &GPIO_InitStructure);           // 初始化

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // 复用模式:串口
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // 上拉下拉:无
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;       // 复用引脚
    GPIO_Init(GPIOC, &GPIO_InitStructure);           // 初始化
}

/*
********************************************************************************************************************
*                  void USART5_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize)
*
*Description : UART5 DMA发送函数
*Arguments   : BufferSRC - 发送数据地址，BufferSize - 发送数据字节数
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void USART5_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize)
{
    DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7);
    DMA1_Stream7->NDTR = BufferSize;    // 设置BUFFER大小
    DMA1_Stream7->M0AR = (uint32_t)BufferSRC;   // 设置地址
    DMA1_Stream7->CR |= DMA_SxCR_EN;    // 使能DMA，开始传输
}

/*
********************************************************************************************************************
*                  void OpenMV_ProcessReceivedData(void)
*
*Description : 处理接收到的OPENMV数据
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void OpenMV_ProcessReceivedData(void)
{
    if(openmv_data_ready)
    {
        openmv_data_ready = 0;
    }
}

/*
********************************************************************************************************************
*                  void OpenMV_ResetData(void)
*
*Description : 重置OPENMV数据
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void OpenMV_ResetData(void)
{
    openmv_data.x_coordinate = 0;
    openmv_data.y_coordinate = 0;
    openmv_data.area = 0;
    openmv_data.data_valid = 0;
    openmv_data.timestamp = 0;
}

/*
********************************************************************************************************************
*                  uint8_t OpenMV_ParseData(uint8_t* buffer, uint8_t len)
*
*Description : 解析OPENMV数据
*Arguments   : buffer - 数据缓冲区，len - 数据长度
*Returns     : 1-解析成功，0-解析失败
*Notes       : 解析格式 "X:123,Y:456,A:7890\n"
********************************************************************************************************************
*/
uint8_t OpenMV_ParseData(uint8_t* buffer, uint8_t len)
{
    uint8_t i = 0;
    uint16_t x_val = 0, y_val = 0, area_val = 0;
    uint8_t x_found = 0, y_found = 0, area_found = 0;
    
    // 查找X坐标
    while(i < len - 1)
    {
        if(buffer[i] == 'X' && buffer[i+1] == ':')
        {
            i += 2;
            while(i < len && buffer[i] >= '0' && buffer[i] <= '9')
            {
                x_val = x_val * 10 + (buffer[i] - '0');
                i++;
            }
            x_found = 1;
            break;
        }
        i++;
    }
    
    // 查找Y坐标
    while(i < len - 1)
    {
        if(buffer[i] == 'Y' && buffer[i+1] == ':')
        {
            i += 2;
            while(i < len && buffer[i] >= '0' && buffer[i] <= '9')
            {
                y_val = y_val * 10 + (buffer[i] - '0');
                i++;
            }
            y_found = 1;
            break;
        }
        i++;
    }
    
    // 查找面积
    while(i < len - 1)
    {
        if(buffer[i] == 'A' && buffer[i+1] == ':')
        {
            i += 2;
            while(i < len && buffer[i] >= '0' && buffer[i] <= '9')
            {
                area_val = area_val * 10 + (buffer[i] - '0');
                i++;
            }
            area_found = 1;
            break;
        }
        i++;
    }
    
    // 检查是否找到所有数据
    if(x_found && y_found && area_found)
    {
        openmv_data.x_coordinate = x_val;
        openmv_data.y_coordinate = y_val;
        openmv_data.area = area_val;
        openmv_data.data_valid = 1;
        openmv_data.timestamp = 0; // 可以添加时间戳功能
        return 1;
    }
    
    return 0;
}

/*
********************************************************************************************************************
*                  void UART5_IRQHandler(void)
*
*Description : UART5中断处理函数
*Arguments   : none
*Returns     : none
*Notes       : 处理IDLE中断，接收OPENMV数据
********************************************************************************************************************
*/
void UART5_IRQHandler(void)
{
    static uint8_t UART5_Rec_Len = 0;

    if(USART_GetITStatus(UART5, USART_IT_IDLE) != RESET)  // 空闲中断
    {
        USART_ReceiveData(UART5);  // 读取数据，清除中断标志
        UART5_Rec_Len = UART5_DATA_LEN - DMA1_Stream0->NDTR;   // 计算接收数据长度
        
        // 检查数据完整性
        if(UART5_Rec_Len > 10) // OPENMV数据包最小长度
        {
            // 查找换行符，确定完整数据包
            uint8_t i;
            for(i = 0; i < UART5_Rec_Len; i++)
            {
                if(rec_data_u5[i] == '\n')
                {
                    // 找到完整数据包，解析数据
                    if(OpenMV_ParseData(rec_data_u5, i + 1))
                    {
                        openmv_data_ready = 1; // 设置数据就绪标志
                    }
                    break;
                }
            }
        }
        
        DMA_ClearFlag(DMA1_Stream0, DMA_FLAG_TCIF0);
        DMA1_S0_Reset(); // 恢复DMA指针，等待下一次接收
    }
    else if(USART_GetITStatus(UART5, USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(UART5, USART_IT_TC);
        DMA1_Stream7->CR &= (uint32_t)(~DMA_SxCR_EN);   // 关闭DMA，发送完成
    }
}

/*
********************************************************************************************************************
*                  void DMA1_Stream7_IRQHandler(void)
*
*Description : DMA1_Stream7发送完成中断处理函数
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void DMA1_Stream7_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_Stream7, DMA_IT_TCIF7) != RESET)
    {
        /* 清除标志位 */
        DMA_ClearFlag(DMA1_Stream7, DMA_IT_TCIF7);
        /* 关闭DMA */
        DMA_Cmd(DMA1_Stream7, DISABLE);
        /* 打开发送完成中断，确保最后一个字节发送成功 */
        USART_ITConfig(UART5, USART_IT_TC, ENABLE);
    }
}

/*
********************************************************************************************************************
*                  void DMA1_S0_Reset(void)
*
*Description : DMA1_Stream0接收重置函数
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void DMA1_S0_Reset(void)
{ 
    DMA_ClearFlag(DMA1_Stream0, DMA_IT_TCIF0|DMA_FLAG_HTIF0|DMA_FLAG_TEIF0|DMA_FLAG_DMEIF0|DMA_FLAG_FEIF0);  
    
    DMA_Cmd(DMA1_Stream0, DISABLE); // 关闭UART5 RX DMA通道

    DMA1_Stream0->NDTR = UART5_DATA_LEN;
    DMA_Cmd(DMA1_Stream0, ENABLE); // 使能UART5 RX DMA通道
}
