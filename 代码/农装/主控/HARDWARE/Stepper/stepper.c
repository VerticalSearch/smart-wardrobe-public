#include "Stepper.h"
#include "delay.h"

// 步进电机参数定义
#define STEPPER_STEPS_PER_REVOLUTION 3200  // 每圈脉冲数(16细分)
#define STEPPER_MAX_SPEED 10000            // 最大转速 RPM
#define STEPPER_MAX_ACCELERATION 255       // 最大加速度值 (0-255, 0=直接启动, 255=最慢加速)
#define STEPPER_DEFAULT_ACCELERATION 100   // 默认加速度值 (推荐值，可根据实际情况调整)

/*
********************************************************************************************************************
*                  void  stepper_init(uint32_t baudrate)
*
*Description : 步进电机初始化
*Arguments   : baudrate 波特率
*Returns     : none
*Notes       : 初始化串口5用于步进电机通信
********************************************************************************************************************
*/
void stepper_init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);	 //使能GPIO时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE); //使能UART时钟
    
    GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART5);	
    GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_UART5);	 
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;            //复用模式
    GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;          //推挽输出
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;       //速度50MHZ
    GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;            //上拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);  		          //初始化
    
    //UsartNVIC配置
    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;	
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能串口接收中断和空闲中断
    USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
    USART_ITConfig(UART5, USART_IT_IDLE, ENABLE);
    
    //USART 初始化设置
    USART_InitStructure.USART_BaudRate = baudrate; //波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1; //一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No; //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
    USART_Init(UART5, &USART_InitStructure);      //初始化串口5
    USART_Cmd(UART5, ENABLE);                     //使能串口5 
}

/*
********************************************************************************************************************
*                  void  stepper_enable(uint8_t addr, bool enable)
*
*Description : 使能/失能步进电机
*Arguments   : addr 设备地址, enable 使能状态
*Returns     : none
*Notes       : 控制步进电机的使能状态
********************************************************************************************************************
*/
void stepper_enable(uint8_t addr, bool enable)
{
    uint8_t cmd[16] = {0};
    
    // 装载命令
    cmd[0] = addr;                       // 地址
    cmd[1] = 0xF3;                       // 命令码
    cmd[2] = 0xAB;                       // 命令码
    cmd[3] = (uint8_t)enable;            // 使能状态
    cmd[4] = 0;                          // 不同步
    cmd[5] = 0x6B;                       // 校验字节
    
    // 发送命令
    stepper_send_command(cmd, 6);
}

/*
********************************************************************************************************************
*                  void  stepper_stop(uint8_t addr)
*
*Description : 立即停止运动
*Arguments   : addr 设备地址
*Returns     : none
*Notes       : 立即停止步进电机运动
********************************************************************************************************************
*/
void stepper_stop(uint8_t addr)
{
    uint8_t cmd[16] = {0};
    
    // 装载命令
    cmd[0] = addr;                       // 地址
    cmd[1] = 0xFE;                       // 命令码
    cmd[2] = 0x98;                       // 命令码
    cmd[3] = 0;                          // 不同步
    cmd[4] = 0x6B;                       // 校验字节
    
    // 发送命令
    stepper_send_command(cmd, 5);
}

/*
********************************************************************************************************************
*                  void  stepper_set_speed(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc)
*
*Description : 设置转速、方向和加速度
*Arguments   : addr 设备地址, dir 方向(0=CW, 1=CCW), speed 转速(RPM), acc 加速度(0-255, 0=直接启动)
*Returns     : none
*Notes       : 速度模式控制，持续旋转
********************************************************************************************************************
*/
void stepper_set_speed(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc)
{
    uint8_t cmd[16] = {0};

    // 限制速度范围
    if(speed > STEPPER_MAX_SPEED) speed = STEPPER_MAX_SPEED;
    
    // 限制加速度范围 (0-255, 0=直接启动, 值越大加速越慢)
    if(acc > STEPPER_MAX_ACCELERATION) acc = STEPPER_MAX_ACCELERATION;

    // 装载命令
    cmd[0] = addr;                       // 地址
    cmd[1] = 0xF6;                       // 命令码
    cmd[2] = dir;                        // 方向
    cmd[3] = (uint8_t)(speed >> 8);      // 速度(RPM)高8位字节
    cmd[4] = (uint8_t)(speed >> 0);      // 速度(RPM)低8位字节
    cmd[5] = acc;                        // 加速度(0-255, 0=直接启动)
    cmd[6] = 0;                          // 不同步
    cmd[7] = 0x6B;                       // 校验字节
    
    // 发送命令
    stepper_send_command(cmd, 8);
}

/*
********************************************************************************************************************
*                  void  stepper_move_angle(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, float angle)
*
*Description : 按角度运动
*Arguments   : addr 设备地址, dir 方向(0=CW, 1=CCW), speed 转速(RPM), acc 加速度(0-255), angle 角度(度)
*Returns     : none
*Notes       : 位置模式控制，按指定角度运动
********************************************************************************************************************
*/
void stepper_move_angle(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, float angle)
{
    // 将角度转换为脉冲数
    uint32_t steps = (uint32_t)((angle / 360.0f) * STEPPER_STEPS_PER_REVOLUTION);
    
    // 调用按步数运动函数
    stepper_move_steps(addr, dir, speed, acc, steps);
}

/*
********************************************************************************************************************
*                  void  stepper_move_steps(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, uint32_t steps)
*
*Description : 按步数运动
*Arguments   : addr 设备地址, dir 方向(0=CW, 1=CCW), speed 转速(RPM), acc 加速度(0-255), steps 脉冲数
*Returns     : none
*Notes       : 位置模式控制，按指定脉冲数运动
********************************************************************************************************************
*/
void stepper_move_steps(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, uint32_t steps)
{
    uint8_t cmd[16] = {0};

    // 限制速度范围
    if(speed > STEPPER_MAX_SPEED) speed = STEPPER_MAX_SPEED;
    
    // 限制加速度范围 (0-255, 0=直接启动, 值越大加速越慢)
    if(acc > STEPPER_MAX_ACCELERATION) acc = STEPPER_MAX_ACCELERATION;

    // 装载命令
    cmd[0]  = addr;                      // 地址
    cmd[1]  = 0xFD;                      // 命令码
    cmd[2]  = dir;                       // 方向
    cmd[3]  = (uint8_t)(speed >> 8);    // 速度(RPM)高8位字节
    cmd[4]  = (uint8_t)(speed >> 0);    // 速度(RPM)低8位字节 
    cmd[5]  = acc;                       // 加速度(0-255, 0=直接启动)
    cmd[6]  = (uint8_t)(steps >> 24);    // 脉冲数(bit24 - bit31)
    cmd[7]  = (uint8_t)(steps >> 16);    // 脉冲数(bit16 - bit23)
    cmd[8]  = (uint8_t)(steps >> 8);     // 脉冲数(bit8  - bit15)
    cmd[9]  = (uint8_t)(steps >> 0);     // 脉冲数(bit0  - bit7 )
    cmd[10] = 0;                         // 相对运动
    cmd[11] = 0;                         // 不同步
    cmd[12] = 0x6B;                      // 校验字节
    
    // 发送命令
    stepper_send_command(cmd, 13);
}

/*
********************************************************************************************************************
*                  void  stepper_send_command(uint8_t* cmd, uint8_t length)
*
*Description : 发送步进电机控制命令
*Arguments   : cmd - 命令数据指针, length - 命令长度
*Returns     : none
*Notes       : 通过串口5发送控制命令
********************************************************************************************************************
*/
void stepper_send_command(uint8_t* cmd, uint8_t length)
{
    // 确保发送缓冲区为空
    while(USART_GetFlagStatus(UART5, USART_FLAG_TXE) == RESET);
    
    // 发送所有字节
    for(uint8_t i = 0; i < length; i++)
    {
        USART_SendData(UART5, cmd[i]);
        // 等待发送寄存器空，确保数据被正确发送
        while(USART_GetFlagStatus(UART5, USART_FLAG_TXE) == RESET);
    }

    // 等待传输完成
    while(USART_GetFlagStatus(UART5, USART_FLAG_TC) == RESET);
    
    // 添加延时确保命令被处理（减少延时以提高响应速度）
    delay_ms(10);
}

/*
********************************************************************************************************************
*                  void  stepper_move_with_acceleration(uint8_t addr, uint8_t dir, uint16_t target_speed, uint8_t acc, float angle)
*
*Description : 加减速运动到指定角度
*Arguments   : addr 设备地址, dir 方向(0=CW, 1=CCW), target_speed 目标转速(RPM), acc 加速度(0-255), angle 角度(度)
*Returns     : none
*Notes       : 使用驱动器内置加减速功能运动到指定角度
********************************************************************************************************************
*/
void stepper_move_with_acceleration(uint8_t addr, uint8_t dir, uint16_t target_speed, uint8_t acc, float angle)
{
    // 直接使用位置控制函数，驱动器会自动处理加减速
    stepper_move_angle(addr, dir, target_speed, acc, angle);
}

/*
********************************************************************************************************************
*                  void  stepper_move_trapezoid(uint8_t addr, uint8_t dir, uint16_t target_speed, uint8_t acc, 
*                                               float accel_angle, float constant_angle, float decel_angle)
*
*Description : 梯形加减速运动（三段式控制：加速->匀速->减速）
*Arguments   : addr 设备地址
*              dir 方向(0=CW, 1=CCW)
*              target_speed 目标转速(RPM)
*              acc 加速度(0-255, 0=直接启动, 值越大加速越慢)
*              accel_angle 加速阶段角度(度)
*              constant_angle 匀速阶段角度(度)
*              decel_angle 减速阶段角度(度)
*Returns     : none
*Notes       : 分三个阶段控制步进电机运动
*              1. 加速阶段：从0加速到target_speed，转动accel_angle角度
*              2. 匀速阶段：以target_speed速度转动constant_angle角度（加速度设为0）
*              3. 减速阶段：从target_speed减速到0，转动decel_angle角度
********************************************************************************************************************
*/
void stepper_move_trapezoid(uint8_t addr, uint8_t dir, uint16_t target_speed, uint8_t acc, 
                            float accel_angle, float constant_angle, float decel_angle)
{
    // 限制速度范围
    if(target_speed > STEPPER_MAX_SPEED) target_speed = STEPPER_MAX_SPEED;
    
    // 限制加速度范围
    uint8_t accel_value = acc;
    if(accel_value > STEPPER_MAX_ACCELERATION) accel_value = STEPPER_MAX_ACCELERATION;
    
    // 第一阶段：加速运动
    if(accel_angle > 0.0f)
    {
        stepper_move_angle(addr, dir, target_speed, accel_value, accel_angle);
        delay_ms(50);  // 等待加速阶段完成
    }
    
    // 第二阶段：匀速运动（加速度设为0，直接启动）
    if(constant_angle > 0.0f)
    {
        stepper_move_angle(addr, dir, target_speed, 0, constant_angle);
        delay_ms(50);  // 等待匀速阶段完成
    }
    
    // 第三阶段：减速运动
    if(decel_angle > 0.0f)
    {
        stepper_move_angle(addr, dir, target_speed, accel_value, decel_angle);
    }
}

