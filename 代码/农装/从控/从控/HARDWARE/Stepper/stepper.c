//#include "stm32f4xx.h"
//#include "stepper.h"
//#include "math.h"
//#include "delay.h"

//// 步进电机参数
//#define STEPS_PER_REVOLUTION 200    // 每圈步数 (1.8度步进电机)
//#define PWM_VALUE 4200              // PWM占空比 (50%)
//#define STEP_DELAY 100              // 步进延时 (系统时间单位)

//// 步进电机状态
//static struct {
//    float current_angle;        // 当前角度
//    float target_angle;         // 目标角度
//    uint8_t is_moving;          // 运动状态
//    uint32_t last_step_time;    // 上次步进时间
//} stepper = {0.0f, 0.0f, 0, 0};

//// 步进电机初始化
//void Stepper_pwm_Init(u32 arr, u32 psc)
//{
//	GPIO_InitTypeDef GPIO_InitStructure;  
//	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
//	TIM_OCInitTypeDef TIM_OCInitStructure;

//	// 使能时钟
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
//	
//	// 配置PB6为PWM输出 (TIM4_CH1)
//	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_TIM4);
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	
//	// 配置PB1为方向引脚
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	
//	// 配置PB10和PB11为高电平输出引脚
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	
//	// 设置PB10和PB11为高电平
//	GPIO_SetBits(GPIOB, GPIO_Pin_10 | GPIO_Pin_11);
//	
//	// 配置定时器
//	TIM_TimeBaseStructure.TIM_Prescaler = psc;
//	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
//	TIM_TimeBaseStructure.TIM_Period = arr;
//	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
//	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
//	
//	// 配置PWM输出 (TIM4_CH1)
//	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
//	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
//	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
//	TIM_OC1Init(TIM4, &TIM_OCInitStructure);
//	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);

//	// 使能定时器
//	TIM_ARRPreloadConfig(TIM4, ENABLE);
//	TIM_Cmd(TIM4, ENABLE);
//}

//// 步进电机控制函数 (PB6 - TIM4_CH1)
//void Stepper1_Ctrl(int Compare)
//{
//    TIM_SetCompare1(TIM4, Compare);
//}

//// 步进电机方向控制
//void Stepper_SetDirection(uint8_t direction)
//{
//    if(direction) {
//        GPIO_SetBits(GPIOB, GPIO_Pin_1);   // PB1方向引脚设为高电平 (正转)
//    } else {
//        GPIO_ResetBits(GPIOB, GPIO_Pin_1); // PB1方向引脚设为低电平 (反转)
//    }
//}

//// 设置步进电机目标角度
//void Stepper_SetAngle(float angle)
//{
//    stepper.target_angle = angle;
//    stepper.is_moving = 1;
//    stepper.last_step_time = 0;
//}

//// 步进电机更新函数 (在主循环中调用)
//void Stepper_Update(void)
//{
//    static uint32_t system_time = 0;
//    static uint8_t pulse_state = 0;  // 脉冲状态：0=低电平，1=高电平
//    system_time++;
//    
//    if(!stepper.is_moving) {
//        // 停止时确保输出低电平
//        GPIO_ResetBits(GPIOB, GPIO_Pin_6);
//        pulse_state = 0;
//        return;
//    }
//    
//    // 计算角度差
//    float angle_diff = stepper.target_angle - stepper.current_angle;
//    
//    // 检查是否到达目标角度 (允许0.5度的误差)
//    if(fabs(angle_diff) < 0.5f) {
//        stepper.is_moving = 0;
//        GPIO_ResetBits(GPIOB, GPIO_Pin_6);  // 停止输出
//        pulse_state = 0;
//        return;
//    }
//    
//    // 步进控制 (每100个系统时间单位执行一步)
//    if((system_time - stepper.last_step_time) >= STEP_DELAY) {
//        stepper.last_step_time = system_time;
//        
//        // 计算步进方向
//        if(angle_diff > 0) {
//            // 正转
//            Stepper_SetDirection(1);  // 设置方向为正转
//            stepper.current_angle += (360.0f / STEPS_PER_REVOLUTION);
//        } else {
//            // 反转
//            Stepper_SetDirection(0);  // 设置方向为反转
//            stepper.current_angle -= (360.0f / STEPS_PER_REVOLUTION);
//        }
//        
//        // 输出步进脉冲 - 产生一个完整的脉冲序列
//        if(!pulse_state) {
//            GPIO_SetBits(GPIOB, GPIO_Pin_6);    // 输出高电平
//            pulse_state = 1;
//        } else {
//            GPIO_ResetBits(GPIOB, GPIO_Pin_6);  // 输出低电平
//            pulse_state = 0;
//        }
//    }
//}

//// 检查步进电机是否正在运动
//uint8_t Stepper_IsMoving(void)
//{
//    return stepper.is_moving;
//}

//// 停止步进电机
//void Stepper_Stop(void)
//{
//    stepper.is_moving = 0;
//    
//    // 停止PWM输出
//    Stepper1_Ctrl(0);
//}

//// 获取当前角度
//float Stepper_GetCurrentAngle(void)
//{
//    return stepper.current_angle;
//}

//// 设置当前角度 (用于初始化)
//void Stepper_SetCurrentAngle(float angle)
//{
//    stepper.current_angle = angle;
//}

//// 步进电机测试函数
//void Stepper_Test(void)
//{
//    // 使能步进电机
//    // Stepper_Enable(1);
//    
//    // 设置正转
//    Stepper_SetDirection(1);
//    
//    // 输出PWM测试
//    Stepper1_Ctrl(PWM_VALUE);
//    
//    // 延时1秒
//    // delay_ms(1000);
//    
//    // 停止PWM
//    Stepper1_Ctrl(0);
//    
//    // 禁用步进电机
//    // Stepper_Enable(0);
//}
