/*
******************************************************************************************************************
*Filename      :usart1_dma.c
*Programmer(s) :嵌入式开发工程师
*Description   :主控板板间通信DMA实现
*               网表显示主板1 USART1(PB6/PB7)接到板载U5，不能作为外接板间口。
*               主板1与主板2通信改走外露CN5：USART2_TX---PD5, USART2_RX---PD6。
******************************************************************************************************************
*/
#include "usartdma.h"
#include "delay.h"
#include "control.h"
#include "timer.h"
#include "board1_uart3_pi_gateway.h"

// 全局变量定义
u8 rec_data_u1[UART1_DATA_LEN];   // 接收缓冲区
u8 send_data_u1[UART1_DATA_LEN];  // 发送缓冲区
volatile uint32_t g_board1_link_tx_queue_count = 0U;
volatile uint32_t g_board1_link_tx_sent_count = 0U;
volatile uint32_t g_board1_link_tx_drop_count = 0U;
volatile uint8_t g_board1_link_last_tx_cmd = 0U;
volatile uint8_t g_board1_link_last_tx_channel = 0U;
volatile uint16_t g_board1_link_last_tx_angle = 0U;
volatile int16_t g_board1_link_last_tx_target = 0;

#define BOARD1_LINK_TX_QUEUE_LEN 16U

static u8 s_link_tx_queue[BOARD1_LINK_TX_QUEUE_LEN][BOARD_LINK_CMD_FRAME_LEN];
static volatile uint8_t s_link_tx_head = 0U;
static volatile uint8_t s_link_tx_tail = 0U;
static volatile uint8_t s_link_tx_count = 0U;
static volatile uint8_t s_link_tx_active = 0U;

/*
********************************************************************************************************************
 *                  void Board1_LinkUart_Init(int Baud)
*
*Description : 主板1板间USART2初始化函数，配置DMA传输
*Arguments   : Baud - 波特率
*Returns     : none
*Notes       : USART2_RX使用DMA1_Stream5，USART2_TX使用DMA1_Stream6
********************************************************************************************************************
*/
void Board1_LinkUart_Init(int Baud)
{
    NVIC_InitTypeDef NVIC_InitStructure;  // 中断初始化结构体
    GPIO_InitTypeDef GPIO_InitStructure;  // GPIO初始化结构体
    USART_InitTypeDef USART_InitStructure;// 串口初始化结构体
    DMA_InitTypeDef DMA_InitStructure;    // DMA初始化结构体

    // 使能USART2时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    //**********************配置发送DMA，DMA1_Stream6**************************
    // 使能DMA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    // DMA发送中断配置
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // DMA发送通道配置
    DMA_DeInit(DMA1_Stream6);
    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);
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
    DMA_Init(DMA1_Stream6, &DMA_InitStructure);
    // 使能发送中断
    DMA_ITConfig(DMA1_Stream6, DMA_IT_TC, ENABLE);
    // 使能串口的DMA发送接口
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
    // 关闭发送通道
    DMA_Cmd(DMA1_Stream6, DISABLE);

    //**********************配置接收DMA，DMA1_Stream5**************************
    // DMA接收通道配置
    DMA_DeInit(DMA1_Stream5);
    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);
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
    DMA_Init(DMA1_Stream5, &DMA_InitStructure);
    // 使能接收通道
    DMA_Cmd(DMA1_Stream5, ENABLE);
    // 使能串口的DMA接收
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);

    // 初始化串口参数
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_BaudRate = Baud;
    // 初始化串口
    USART_Init(USART2, &USART_InitStructure);

    // 配置中断
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 使能DMA模式发送
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
    // 使能DMA模式接收
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);

    // 中断配置
    USART_ITConfig(USART2, USART_IT_TC, DISABLE);
    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);

    // 使能串口
    USART_Cmd(USART2, ENABLE);

    // 配置IO时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // 复用模式:串口
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   // 输出:推挽模式
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;     // 上拉下拉:上拉
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // IO速度
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;        // USART2_TX, CN5.1
    GPIO_Init(GPIOD, &GPIO_InitStructure);           // 初始化

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // 复用模式:串口
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // 上拉下拉:无
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;        // USART2_RX, CN5.2
    GPIO_Init(GPIOD, &GPIO_InitStructure);           // 初始化
}

/*
********************************************************************************************************************
 *                  void Board1_LinkUart_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize)
*
*Description : 主板1板间USART2 DMA发送函数
*Arguments   : BufferSRC - 发送数据地址，BufferSize - 发送数据字节数
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void Board1_LinkUart_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize)
{
    // 检查DMA是否正在发送（非阻塞检查）
    if(DMA1_Stream6->CR & DMA_SxCR_EN) {
        // DMA还在发送中，直接返回，不发送新命令
        return;
    }
    
    s_link_tx_active = 1U;
    DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6);
    DMA1_Stream6->NDTR = BufferSize;    // 设置BUFFER大小
    DMA1_Stream6->M0AR = (uint32_t)BufferSRC;   // 设置地址
    DMA1_Stream6->CR |= DMA_SxCR_EN;    // 使能DMA，开始传输
}

static uint8_t Board1_LinkTxBusy(void)
{
    if (s_link_tx_active != 0U)
    {
        return 1U;
    }

    if ((DMA1_Stream6->CR & DMA_SxCR_EN) != 0U)
    {
        return 1U;
    }

    return 0U;
}

static void Board1_LinkClearPendingQueue(void)
{
    __disable_irq();
    s_link_tx_head = 0U;
    s_link_tx_tail = 0U;
    s_link_tx_count = 0U;
    __enable_irq();
}

static uint8_t Board1_LinkQueueFrame(const u8 *frame)
{
    uint8_t i;

    if (frame == 0)
    {
        return 0U;
    }

    __disable_irq();
    if (s_link_tx_count >= BOARD1_LINK_TX_QUEUE_LEN)
    {
        g_board1_link_tx_drop_count++;
        __enable_irq();
        return 0U;
    }

    for (i = 0U; i < BOARD_LINK_CMD_FRAME_LEN; i++)
    {
        s_link_tx_queue[s_link_tx_tail][i] = frame[i];
    }
    s_link_tx_tail = (uint8_t)((s_link_tx_tail + 1U) % BOARD1_LINK_TX_QUEUE_LEN);
    s_link_tx_count++;
    g_board1_link_tx_queue_count++;
    __enable_irq();
    return 1U;
}

void Board1_LinkService(void)
{
    uint8_t i;

    if (Board1_LinkTxBusy() != 0U)
    {
        return;
    }

    __disable_irq();
    if (s_link_tx_count == 0U)
    {
        __enable_irq();
        return;
    }

    for (i = 0U; i < BOARD_LINK_CMD_FRAME_LEN; i++)
    {
        send_data_u1[i] = s_link_tx_queue[s_link_tx_head][i];
    }
    g_board1_link_last_tx_cmd = send_data_u1[1];
    g_board1_link_last_tx_channel = send_data_u1[2];
    g_board1_link_last_tx_angle = (uint16_t)(((uint16_t)send_data_u1[4] << 8) | send_data_u1[5]);
    s_link_tx_head = (uint8_t)((s_link_tx_head + 1U) % BOARD1_LINK_TX_QUEUE_LEN);
    s_link_tx_count--;
    __enable_irq();

    Board1_LinkUart_DMATransfer((uint32_t*)send_data_u1, BOARD_LINK_CMD_FRAME_LEN);
    g_board1_link_tx_sent_count++;
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
 *                  static void Board1_LinkSendCommandFrame(...)
*
*Description : 组装并发送主板1到主板2的7字节命令帧
*Arguments   : cmd_type - 命令类型，channel/control/angle - 电机控制参数
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
static uint8_t Board1_LinkSendCommandFrame(uint8_t cmd_type,
                                           uint8_t channel,
                                           uint8_t control,
                                           uint16_t angle_deg)
{
    u8 frame[BOARD_LINK_CMD_FRAME_LEN];

    frame[0] = CMD_HEADER;
    frame[1] = cmd_type;
    frame[2] = channel;
    frame[3] = control;
    frame[4] = (uint8_t)((angle_deg >> 8) & 0xFFU);
    frame[5] = (uint8_t)(angle_deg & 0xFFU);
    frame[6] = CalculateChecksum(frame, BOARD_LINK_CMD_FRAME_LEN - 1U);

    return Board1_LinkQueueFrame(frame);
}

static uint8_t Board1_LinkStepperChannelValid(uint8_t channel)
{
    if ((channel >= BOARD1_LINK_STEPPER_CHANNEL_MIN) &&
        (channel <= BOARD1_LINK_STEPPER_CHANNEL_MAX))
    {
        return 1U;
    }

    return 0U;
}

void Board1_LinkSetStepperEnable(uint8_t channel, uint8_t enable)
{
    if (Board1_LinkStepperChannelValid(channel) == 0U)
    {
        return;
    }

    Board1_LinkSendCommandFrame(CMD_STEPPER_ENABLE,
                                channel,
                                (enable != 0U) ? 1U : 0U,
                                0U);
}

void Board1_LinkMoveStepperAngle(uint8_t channel,
                                 uint8_t dir,
                                 uint16_t angle_deg)
{
    if (Board1_LinkStepperChannelValid(channel) == 0U)
    {
        return;
    }

    Board1_LinkSendCommandFrame(CMD_STEPPER_MOVE_ANGLE,
                                channel,
                                dir,
                                angle_deg);
}

void Board1_LinkGotoStepperAngle(uint8_t channel, int16_t target_deg)
{
    if (Board1_LinkStepperChannelValid(channel) == 0U)
    {
        return;
    }

    g_board1_link_last_tx_target = target_deg;
    Board1_LinkSendCommandFrame(CMD_STEPPER_GOTO_ANGLE,
                                channel,
                                0U,
                                (uint16_t)target_deg);
}

void Board1_LinkEmergencyStop(void)
{
    Board1_LinkClearPendingQueue();
    Board1_LinkSendCommandFrame(CMD_EMERGENCY, 0U, 0U, 0U);
}

void Board1_LinkSetServoAngle(uint8_t channel, uint16_t angle_deg)
{
    Board1_LinkSendCommandFrame(CMD_SERVO_SET_ANGLE,
                                channel,
                                0U,
                                angle_deg);
}

uint8_t Board1_LinkStartAction(uint8_t action_id, uint8_t target_c)
{
    return Board1_LinkSendCommandFrame(CMD_ACTION_START,
                                       action_id,
                                       target_c,
                                       0U);
}

static void Board1_LinkHandleReceivedFrame(uint8_t rec_len)
{
    if ((rec_len != BOARD_LINK_CMD_FRAME_LEN) || (rec_data_u1[0] != CMD_HEADER))
    {
        return;
    }

    if (CalculateChecksum(rec_data_u1, (uint8_t)(BOARD_LINK_CMD_FRAME_LEN - 1U)) !=
        rec_data_u1[BOARD_LINK_CMD_FRAME_LEN - 1U])
    {
        return;
    }

    switch (rec_data_u1[1])
    {
    case CMD_PI_GATEWAY_CAPTURE:
        g_board1_camera_trigger_side_id = rec_data_u1[2];
        g_board1_camera_trigger_placeholder = 1U;
        break;

    case CMD_PHOTO_FILL_LIGHT:
        Board1_FillLight_Set(rec_data_u1[3]);
        break;

    case CMD_ACTION_STATUS:
        board1_uart3_pi_gateway_handle_board2_status(
            rec_data_u1[2],
            rec_data_u1[3],
            (uint16_t)(((uint16_t)rec_data_u1[4] << 8) | rec_data_u1[5]));
        break;

    default:
        break;
    }
}

void Master_SendCommand(uint8_t cmd_type, uint8_t param1, uint8_t param2)
{
    if (cmd_type == CMD_STEPPER_ENABLE)
    {
        Board1_LinkSetStepperEnable(param1, param2);
    }
    else if (cmd_type == CMD_STEPPER_MOVE_ANGLE)
    {
        Board1_LinkMoveStepperAngle(param1,
                                    BOARD_LINK_DIR_FORWARD,
                                    (uint16_t)param2);
    }
    else if (cmd_type == CMD_STEPPER_GOTO_ANGLE)
    {
        Board1_LinkGotoStepperAngle(param1, (int16_t)(uint16_t)param2);
    }
    else if (cmd_type == CMD_SERVO_SET_ANGLE)
    {
        Board1_LinkSetServoAngle(param1, (uint16_t)param2);
    }
    else
    {
        Board1_LinkSendCommandFrame(cmd_type, param1, param2, 0U);
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
 *                  void USART2_IRQHandler(void)
*
*Description : 主板1板间USART2中断处理函数
*Arguments   : none
*Returns     : none
*Notes       : 当前主板2不回传传感器数据，仅复位RX DMA，保留后续扩展入口
********************************************************************************************************************
*/
void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)  // 空闲中断
    {
        uint8_t rec_len;
        USART_ReceiveData(USART2);  // 读取数据，清除中断标志
        rec_len = (uint8_t)(UART1_DATA_LEN - DMA1_Stream5->NDTR);
        Board1_LinkHandleReceivedFrame(rec_len);
        DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5);  // 清除接收DMA标志
        Board1_LinkRxDma_Reset(); // 恢复DMA指针，等待下一次接收
    }
    else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(USART2, USART_IT_TC);
        USART_ITConfig(USART2, USART_IT_TC, DISABLE);
        DMA1_Stream6->CR &= (uint32_t)(~DMA_SxCR_EN);   // 关闭DMA，发送完成
        s_link_tx_active = 0U;
    }
}

/*
********************************************************************************************************************
 *                  void DMA1_Stream6_IRQHandler(void)
*
*Description : DMA1_Stream6发送完成中断处理函数
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void DMA1_Stream6_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_Stream6, DMA_IT_TCIF6) != RESET)
    {
        /* 清除标志位 */
        DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6);
        /* 关闭DMA */
        DMA_Cmd(DMA1_Stream6, DISABLE);
        /* 打开发送完成中断，确保最后一个字节发送成功 */
        USART_ITConfig(USART2, USART_IT_TC, ENABLE);
    }
}

/*
********************************************************************************************************************
 *                  void Board1_LinkRxDma_Reset(void)
*
*Description : DMA1_Stream5接收重置函数
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void Board1_LinkRxDma_Reset(void)
{ 
    DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5|DMA_FLAG_HTIF5|DMA_FLAG_TEIF5|DMA_FLAG_DMEIF5|DMA_FLAG_FEIF5);
    
    DMA_Cmd(DMA1_Stream5, DISABLE); // 关闭USART2 RX DMA通道

    DMA1_Stream5->NDTR = UART1_DATA_LEN;
    DMA_Cmd(DMA1_Stream5, ENABLE); // 使能USART2 RX DMA通道
}

void USART1_Init(int Baud)
{
    Board1_LinkUart_Init(Baud);
}

void USART1_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize)
{
    Board1_LinkUart_DMATransfer(BufferSRC, BufferSize);
}

void DMA2_S5_Reset(void)
{
    Board1_LinkRxDma_Reset();
}
