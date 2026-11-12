/*
******************************************************************************************************************
*Filename      :usart1_dma.c
*Programmer(s) :嵌入式开发工程师
*Description   :从控板UART1 DMA通信实现
*               接收主板指令，回传陀螺仪数据
*               USART1_RX---PB7
*               USART1_TX---PB6
******************************************************************************************************************
*/
#include "usartdma.h"
#include "delay.h"
#include "steer.h"      // 舵机控制
#include "control.h"    // 舵机姿态调整函数
#include <string.h>     // 添加memcpy函数所需的头文件

// 全局变量定义
u8 rec_data_u1[UART1_DATA_LEN] = {0};   // 接收缓冲区（初始化为0）
u8 send_data_u1[UART1_DATA_LEN] = {0};  // 发送缓冲区（初始化为0）
Command_t received_cmd = {0};      // 接收到的命令
SlaveSensorData_t slave_sensor_data = {0}; // 从控传感器数据
uint8_t cmd_received_flag = 0;    // 命令接收标志

// 状态切换延迟相关变量（供TIM7中断使用）
uint16_t state_switch_delay0_counter = 0;  // 延迟0计数器：收到指令后到开始执行状态1的初始延迟（TIM7中断中递减）
uint16_t state_switch_delay1_counter = 0;  // 延迟1计数器：状态1到状态2的延迟（TIM7中断中递减）
uint16_t state_switch_delay2_counter = 0;  // 延迟2计数器：状态2到状态3的延迟（TIM7中断中递减）
uint16_t state_switch_delay3_counter = 0;  // 延迟3计数器：状态3到状态4的延迟（TIM7中断中递减）
uint16_t state_switch_delay4_counter = 0;  // 延迟4计数器：状态4到状态5的延迟（TIM7中断中递减）
uint8_t state_switch_delay0_timeout = 0;  // 延迟0超时标志：1=需要执行第一组动作
uint8_t state_switch_delay1_timeout = 0;  // 延迟1超时标志：1=需要执行第二组动作
uint8_t state_switch_delay2_timeout = 0;  // 延迟2超时标志：1=需要执行第三组动作
uint8_t state_switch_delay3_timeout = 0;  // 延迟3超时标志：1=需要执行第四组动作
uint8_t state_switch_delay4_timeout = 0;  // 延迟4超时标志：1=需要执行第五组动作
uint8_t action_group_executing = 0;  // 作业动作组执行标志：1=正在执行，禁止Steer_TurnRight/Left控制舵机3和4
uint8_t keep_steer34_state1 = 0;  // 保持舵机3和4在状态1的标志：1=在整个跨垄过程中保持状态1，直到跨垄结束
uint8_t is_fifth_backward = 0;  // 第五次后退标志：1=第五次后退，使用特殊的舵机控制函数
uint16_t left3_steer34_delay_counter = 0;  // 第三次跨左垄舵机3和4延时计数器（TIM7中断中递减，1000ms）
uint8_t skip_state5_flag = 0;  // 跳过状态5标志：1=第三次右跨垄，只执行状态1-4，不执行状态5

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
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
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
    DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7);
    DMA2_Stream7->NDTR = BufferSize;    // 设置BUFFER大小
    DMA2_Stream7->M0AR = (uint32_t)BufferSRC;   // 设置地址
    DMA2_Stream7->CR |= DMA_SxCR_EN;    // 使能DMA，开始传输
}

/*
********************************************************************************************************************
*                  void Slave_SendSensorData(void)
*
*Description : 从控主动发送传感器数据函数（陀螺仪数据）
*Arguments   : none
*Returns     : none
*Notes       : 从控在TIM7中断中每10ms自动调用此函数，主动向主控发送陀螺仪数据（100Hz更新率）
*              更新频率与主控速度控制周期（10ms）同步，确保角度修正响应及时
*              主控无需请求，直接接收即可
********************************************************************************************************************
*/
void Slave_SendSensorData(void)
{
    uint8_t data_packet[10];
    uint8_t index = 0;
    
    // 更新传感器数据
    Slave_UpdateSensorData();
    
    // 构建数据包
    data_packet[index++] = CMD_HEADER;  // 帧头
    // 添加陀螺仪数据（yaw，度*10，例如：15.5度 = 155）
    data_packet[index++] = (slave_sensor_data.yaw >> 8) & 0xFF;               // yaw高字节
    data_packet[index++] = slave_sensor_data.yaw & 0xFF;                      // yaw低字节
    
    // 计算校验和
    data_packet[index] = CalculateChecksum(data_packet, index);
    
    // 使用DMA发送数据
    memcpy(send_data_u1, data_packet, index + 1);
    USART1_DMATransfer((uint32_t*)send_data_u1, index + 1);
}

/*
********************************************************************************************************************
*                  void Slave_CheckStateSwitchDelay(void)
*
*Description : 检查状态切换延迟是否完成，如果完成则执行下一组动作
*Arguments   : none
*Returns     : none
*Notes       : 非阻塞延迟检查函数，需要在主循环中调用
*              延迟计数在TIM7中断（1ms周期）中递减
*              流程：初始延时 -> 状态1 -> 延时1 -> 状态2 -> 延时2 -> 状态3 -> 延时3 -> 状态4
********************************************************************************************************************
*/
void Slave_CheckStateSwitchDelay(void)
{
    extern int angle1, angle2;  // 声明外部变量
    
    // 检查延迟0超时（收到指令 -> 状态1）
    if(state_switch_delay0_timeout)
    {
        // 执行第一组动作
        angle1 = STEER1_State1_Angle;
        angle2 = STEER2_State1_Angle;
        Steer1_4_PWM_Control(1, angle1);
        Steer1_4_PWM_Control(2, angle2);
        
        // 启动延迟1计数器
        state_switch_delay1_counter = STATE_SWITCH_Delay1_Ms;
        
        // 清除延迟0超时标志
        state_switch_delay0_timeout = 0;
    }
    
    // 检查延迟1超时（状态1 -> 状态2）
    if(state_switch_delay1_timeout)
    {
        // 执行第二组动作
        angle1 = STEER1_State2_Angle;
        angle2 = STEER2_State2_Angle;
        Steer1_4_PWM_Control(1, angle1);
        Steer1_4_PWM_Control(2, angle2);
        
        // 启动延迟2计数器
        state_switch_delay2_counter = STATE_SWITCH_Delay2_Ms;
        
        // 清除延迟1超时标志
        state_switch_delay1_timeout = 0;
    }
    
    // 检查延迟2超时（状态2 -> 状态3）
    if(state_switch_delay2_timeout)
    {
        // 执行第三组动作
        angle1 = STEER1_State3_Angle;
        angle2 = STEER2_State3_Angle;
        Steer1_4_PWM_Control(1, angle1);
        Steer1_4_PWM_Control(2, angle2);
        
        // 启动延迟3计数器
        state_switch_delay3_counter = STATE_SWITCH_Delay3_Ms;
        
        // 清除延迟2超时标志
        state_switch_delay2_timeout = 0;
    }
    
    // 检查延迟3超时（状态3 -> 状态4）
    if(state_switch_delay3_timeout)
    {
        // 执行第四组动作
        angle1 = STEER1_State4_Angle;
        angle2 = STEER2_State4_Angle;
        Steer1_4_PWM_Control(1, angle1);
        Steer1_4_PWM_Control(2, angle2);
        
        // 清除延迟3超时标志
        state_switch_delay3_timeout = 0;
        
        // 如果是第三次右跨垄（skip_state5_flag == 1），不启动延迟4计数器，直接完成动作组
        if(skip_state5_flag == 1)
        {
            // 第三次右跨垄：不执行状态5，直接完成动作组
            // 确保延迟4计数器被清除，防止状态5被执行
            state_switch_delay4_counter = 0;
            state_switch_delay4_timeout = 0;
            action_group_executing = 0;  // 作业动作组执行完成
            skip_state5_flag = 0;  // 清除跳过状态5标志
        }
        else
        {
            // 正常情况：启动延迟4计数器，准备执行状态5
            state_switch_delay4_counter = STATE_SWITCH_Delay4_Ms;
        }
    }
    
    // 检查延迟4超时（状态4 -> 状态5）
    // 注意：如果skip_state5_flag == 1，延迟4计数器不会被启动，所以这里不会执行
    if(state_switch_delay4_timeout && skip_state5_flag == 0)
    {
        // 执行第五组动作
        angle1 = STEER1_State5_Angle;
        angle2 = STEER2_State5_Angle;
        Steer1_4_PWM_Control(1, angle1);
        Steer1_4_PWM_Control(2, angle2);
        
        // 清除延迟4超时标志
        state_switch_delay4_timeout = 0;
        
        // 作业动作组执行完成，清除标志，允许Steer_TurnRight/Left控制舵机3和4
        action_group_executing = 0;
    }
}

/*
********************************************************************************************************************
*                  void Slave_ProcessReceivedCommand(void)
*
*Description : 从控处理接收到的命令
*Arguments   : none
*Returns     : none
*Notes       : none
********************************************************************************************************************
*/
void Slave_ProcessReceivedCommand(void)
{
    if(cmd_received_flag)
    {
        cmd_received_flag = 0;
        
        // 处理命令
        switch(received_cmd.cmd_type)
        {
            case CMD_GET_SENSOR:
                // 收到获取传感器数据命令（可选，现在从控主动发送，此命令可保留但不常用）
                // Slave_SendSensorData();  // 已改为主动发送，不再响应请求
                break;
                
            case CMD_SET_DIRECTION:
                // 设置舵机方向
                {
                    extern int steer_direction;  // 声明外部变量
                    extern int steer34_state;     // 声明外部变量
                    if(received_cmd.param1 <= 3)
                    {
                        steer_direction = received_cmd.param1;
                        
                        // 根据方向设置舵机3和4的状态
                        if(received_cmd.param1 == 1)  // 右行（右跨垄）
                        {
                            steer34_state = 3;  // 状态3：右跨垄（左降右升）
                            Reset_Distance_PID_Disabled_Flags();  // 进入新的跨垄状态，重置PID禁用标志
                            is_fifth_backward = 0;  // 清除第五次后退标志
                            left3_steer34_delay_counter = 0;  // 清除第三次跨左垄延时计数器
                        }
                        else if(received_cmd.param1 == 2)  // 左行（左跨垄）
                        {
                            // 通过param2区分三个左跨垄状态
                            if(received_cmd.param2 == 0)
                            {
                                steer34_state = 2;  // 状态2：第一次左跨垄（左升右降）
                                left3_steer34_delay_counter = 0;  // 清除第三次跨左垄延时计数器
                            }
                            else if(received_cmd.param2 == 2)
                            {
                                steer34_state = 5;  // 状态5：第二次左跨垄（左升右降）
                                left3_steer34_delay_counter = 0;  // 清除第三次跨左垄延时计数器
                            }
                            else if(received_cmd.param2 == 1)
                            {
                                steer34_state = 4;  // 状态4：第三次跨左垄（同升）
                                // 启动第三次跨左垄舵机3和4延时计数器（1000ms）
                                left3_steer34_delay_counter = 1000;  // 1000ms延时
                            }
                            else
                            {
                                steer34_state = 2;  // 默认：第一次左跨垄
                                left3_steer34_delay_counter = 0;  // 清除第三次跨左垄延时计数器
                            }
                            Reset_Distance_PID_Disabled_Flags();  // 进入新的跨垄状态，重置PID禁用标志
                            is_fifth_backward = 0;  // 清除第五次后退标志
                        }
                        else if(received_cmd.param1 == 3)  // 倒退（编码器后退）
                        {
                            steer34_state = 1;  // 状态1：编码器后退（同降）
                            // 跨垄结束，清除保持舵机3和4在状态1的标志
                            keep_steer34_state1 = 0;
                            left3_steer34_delay_counter = 0;  // 清除第三次跨左垄延时计数器
                            
                            // 第五次后退特殊处理：param2=5表示第五次后退（最后一次）
                            if(received_cmd.param2 == 5)
                            {
                                // 设置第五次后退标志，主循环中会持续调用Steer_Control_5_8_Vertical()
                                is_fifth_backward = 1;
                            }
                            else
                            {
                                // 其他次后退，清除第五次后退标志
                                is_fifth_backward = 0;
                            }
                        }
                        else  // 直行（param1 == 0）
                        {
                            steer34_state = 1;  // 状态1：直行（同降）
                            // 跨垄结束，清除保持舵机3和4在状态1的标志
                            keep_steer34_state1 = 0;
                            is_fifth_backward = 0;  // 清除第五次后退标志
                            left3_steer34_delay_counter = 0;  // 清除第三次跨左垄延时计数器
                        }
                    }
                }
                break;
                
            case CMD_STATE_SWITCH_1:
                // 状态切换1：先延时，然后控制舵机1和2依次执行状态1、状态2、状态3、状态4、状态5
                {
                    // 检查param2是否为3，如果是则表示第三次右跨垄，只执行状态1-4
                    if(received_cmd.param2 == 3) {
                        skip_state5_flag = 1;  // 设置跳过状态5标志
                    } else {
                        skip_state5_flag = 0;  // 清除跳过状态5标志
                    }
                    
                    // 启动初始延迟：设置延迟0计数器（TIM7中断中递减）
                    state_switch_delay0_counter = STATE_SWITCH_Delay0_Ms;
                    // 清除所有延迟计数器和超时标志（防止之前的状态影响）
                    state_switch_delay1_counter = 0;
                    state_switch_delay1_timeout = 0;
                    state_switch_delay2_counter = 0;
                    state_switch_delay2_timeout = 0;
                    state_switch_delay3_counter = 0;
                    state_switch_delay3_timeout = 0;
                    state_switch_delay4_counter = 0;
                    state_switch_delay4_timeout = 0;
                    
                    // 设置作业动作组执行标志，禁止Steer_TurnRight/Left控制舵机3和4
                    action_group_executing = 1;
                    
                    // 设置保持舵机3和4在状态1的标志，在整个跨垄过程中保持状态1
                    keep_steer34_state1 = 1;
                    
                    // 立即控制舵机3和4到状态1（同降：直行/编码器后退）
                    Steer1_4_PWM_Control(3, STEER3_State1_Angle);  // 舵机3：状态1（左）
                    Steer1_4_PWM_Control(4, STEER4_State1_Angle);  // 舵机4：状态1（右）
                }
                break;
                
            case CMD_STATE_SWITCH_2:
                // 状态切换2：控制舵机1和2转到预设角度2
                {
                    extern int angle1, angle2;  // 声明外部变量
                    // 设置预设角度值
                    angle1 = STEER1_State2_Angle;
                    angle2 = STEER2_State2_Angle;
                    
                    // 立即控制舵机转到预设角度
                    Steer1_4_PWM_Control(1, angle1);
                    Steer1_4_PWM_Control(2, angle2);
                }
                break;
                
            case CMD_STATE_SWITCH_3:
                // 状态切换3：控制舵机1和2转到预设角度3
                {
                    extern int angle1, angle2;  // 声明外部变量
                    // 设置预设角度值
                    angle1 = STEER1_State3_Angle;
                    angle2 = STEER2_State3_Angle;
                    
                    // 立即控制舵机转到预设角度
                    Steer1_4_PWM_Control(1, angle1);
                    Steer1_4_PWM_Control(2, angle2);
                }
                break;
                
            default:
                // 未知命令
                break;
        }
    }
}

/*
********************************************************************************************************************
*                  void Slave_UpdateSensorData(void)
*
*Description : 从控更新传感器数据
*Arguments   : none
*Returns     : none
*Notes       : 读取陀螺仪数据
********************************************************************************************************************
*/
void Slave_UpdateSensorData(void)
{
    // 更新陀螺仪数据
    extern float yaw;  // 从control.c中获取yaw值
    // 将float转换为int16_t（度*10，保留1位小数精度）
    slave_sensor_data.yaw = (int16_t)(yaw * 10.0f);
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
*Notes       : 处理IDLE中断，接收主控命令
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
        if(UART1_Rec_Len >= 5) // 命令数据包最小长度
        {
            // 验证帧头
            if(rec_data_u1[0] == CMD_HEADER)
            {
                // 验证校验和
                uint8_t checksum = CalculateChecksum(rec_data_u1, UART1_Rec_Len - 1);
                if(checksum == rec_data_u1[UART1_Rec_Len - 1])
                {
                    // 解析命令
                    received_cmd.header = rec_data_u1[0];
                    received_cmd.cmd_type = rec_data_u1[1];
                    received_cmd.param1 = rec_data_u1[2];
                    received_cmd.param2 = rec_data_u1[3];
                    received_cmd.checksum = rec_data_u1[4];
                    
                    cmd_received_flag = 1; // 设置接收标志
                }
            }
        }
        
        DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5);  // 修复：清除接收DMA标志
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
    DMA2_Stream5->M0AR = (uint32_t)rec_data_u1;  // 重新设置接收缓冲区地址
    DMA_Cmd(DMA2_Stream5, ENABLE); // 使能USART1 RX DMA通道
}

