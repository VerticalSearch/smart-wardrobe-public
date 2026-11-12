#include "steer.h" 
#include "delay.h" 
#include "control.h"

extern int angle;

// TIM1定时器初始化函数，用于控制舵机1、2、3、4
void TIM1_PWM_Init(u16 arr, u16 psc)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure;  // BDTR结构体（高级定时器必需）
    
    // 使能GPIO和TIM时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);     //使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);      //使能TIM1时钟（TIM1在APB2总线上）
    
    // 配置PA8, PA9, PA10, PA11为复用功能
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);   //PA8复用为TIM1_CH1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);   //PA9复用为TIM1_CH2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);  //PA10复用为TIM1_CH3
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_TIM1);  //PA11复用为TIM1_CH4
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;              //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;        //速度100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;            //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;              //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 初始化TIM1时基单元
    TIM_TimeBaseStructure.TIM_Period = arr;                   //自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = psc;                //预分频值
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//向上计数模式
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
    
    // 初始化TIM1通道1 PWM模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;          //PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//输出使能
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;  //输出极性高
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;//空闲状态为低电平
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);                   //初始化通道1
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);          //使能通道1预装载
    
    // 初始化TIM1通道2 PWM模式
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);                   //初始化通道2
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);          //使能通道2预装载
    
    // 初始化TIM1通道3 PWM模式
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);                   //初始化通道3
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);          //使能通道3预装载
    
    // 初始化TIM1通道4 PWM模式
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);                   //初始化通道4
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);          //使能通道4预装载
    
    // 使能自动重装载
    TIM_ARRPreloadConfig(TIM1, ENABLE);                        //使能自动重装载
    
    // 配置BDTR寄存器（高级定时器必需）
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;      //运行模式下的关闭状态选择
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;      //空闲模式下的关闭状态选择
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF;         //锁定级别
    TIM_BDTRInitStructure.TIM_DeadTime = 0;                         //死区时间（舵机不需要死区）
    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;             //刹车功能禁用
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low; //刹车极性
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable; //自动输出禁用
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);               //配置BDTR寄存器
    
    // 使能TIM1（必须在使能PWM输出之前）
    TIM_Cmd(TIM1, ENABLE);
    
    // 使能PWM输出（高级定时器必须调用此函数，会设置MOE位）
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

void TIM3_PWM_Init(u16 arr, u16 psc)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 使能GPIO和TIM时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);     //使能GPIOC时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);      //使能TIM3时钟
    
    // 配置PC6, PC7, PC8, PC9为复用功能
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);   //PC6复用为TIM3_CH1
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM3);   //PC7复用为TIM3_CH2
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM3);   //PC8复用为TIM3_CH3
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM3);   //PC9复用为TIM3_CH4
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;              //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;        //速度100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;            //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;              //上拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 初始化TIM3时基单元
    TIM_TimeBaseStructure.TIM_Period = arr;                   //自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = psc;                //预分频值
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//向上计数模式
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // 初始化TIM3通道1 PWM模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;          //PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//输出使能
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;  //输出极性高
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);                   //初始化通道1
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);          //使能通道1预装载
    
    // 初始化TIM3通道2 PWM模式
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);                   //初始化通道2
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);          //使能通道2预装载
    
    // 初始化TIM3通道3 PWM模式
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);                   //初始化通道3
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);          //使能通道3预装载
    
    // 初始化TIM3通道4 PWM模式
    TIM_OC4Init(TIM3, &TIM_OCInitStructure);                   //初始化通道4
    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);          //使能通道4预装载
    
    // 使能自动重装载
    TIM_ARRPreloadConfig(TIM3, ENABLE);                        //使能自动重装载
    
    // 使能PWM输出（关键！）
    TIM_CtrlPWMOutputs(TIM3, ENABLE);
    
    // 使能TIM3
    TIM_Cmd(TIM3, ENABLE);                //使能TIM3
}



// 270度舵机PWM值转角度函数（实际测试版）
int PWM_To_Angle(u16 pwm)
{
    int angle;
    
    // PWM范围限制
    if(pwm < 300) pwm = 300;
    if(pwm > 1600) pwm = 1600;
    
    // 根据实际测试的PWM范围：300-1600对应0-270度
    // 线性转换: angle = (pwm - 300) * 270 / 1300
    angle = ((pwm - 300) * 270) / 1300;
    
    // 角度范围检查
    if(angle < 0) angle = 0;
    if(angle > 270) angle = 270;
    
    return angle;
}


//(0.3ms----1.6ms)/20ms			1.5%--------8%
//				20000           0----------270
//								300--------1600

// 舵机1、2、3、4 PWM控制函数（使用TIM1）
void Steer1_4_PWM_Control(u8 which, int PWM)
{
    switch(which)
    {
        case 1: TIM_SetCompare1(TIM1, PWM); break;  // 舵机1 (TIM1_CH1)
        case 2: TIM_SetCompare2(TIM1, PWM); break;  // 舵机2 (TIM1_CH2)
        case 3: TIM_SetCompare3(TIM1, PWM); break;  // 舵机3 (TIM1_CH3)
        case 4: TIM_SetCompare4(TIM1, PWM); break;  // 舵机4 (TIM1_CH4)
        default: break;
    }
}

// 舵机5、6、7、8 PWM控制函数（使用TIM3）
void Steer5_8_PWM_Control(u8 which, int PWM)
{
    switch(which)
    {
        case 5: TIM_SetCompare1(TIM3, PWM); break;  // 舵机5 (TIM3_CH1)
        case 6: TIM_SetCompare2(TIM3, PWM); break;  // 舵机6 (TIM3_CH2)
        case 7: TIM_SetCompare3(TIM3, PWM); break;  // 舵机7 (TIM3_CH3)
        case 8: TIM_SetCompare4(TIM3, PWM); break;  // 舵机8 (TIM3_CH4)
        default: break;
    }
}

// 统一控制所有舵机5、6、7、8的PWM值
void ALL_Steer5_8_PWM_Control(int PWM5, int PWM6, int PWM7, int PWM8)
{
	Steer5_8_PWM_Control(5, PWM5);   
	Steer5_8_PWM_Control(6, PWM6);
	Steer5_8_PWM_Control(7, PWM7);
	Steer5_8_PWM_Control(8, PWM8);
}

// 统一控制所有舵机1、2、3、4的PWM值
void ALL_Steer1_4_PWM_Control(int PWM1, int PWM2, int PWM3, int PWM4)
{
	Steer1_4_PWM_Control(1, PWM1);   
	Steer1_4_PWM_Control(2, PWM2);
	Steer1_4_PWM_Control(3, PWM3);
	Steer1_4_PWM_Control(4, PWM4);
}

// 初始化舵机1、2、3、4到中间位置
void Steer1_4_Init_Position(void)
{
    Steer1_4_PWM_Control(1, 5100);  // 舵机1初始位置
    Steer1_4_PWM_Control(2, 2900);  // 舵机2初始位置
    Steer1_4_PWM_Control(3, 2200);  // 舵机3初始位置
    Steer1_4_PWM_Control(4, 5020);  // 舵机4初始位置
}

// 初始化舵机5、6、7、8到中间位置
void Steer5_8_Init_Position(void)
{
    Steer5_8_PWM_Control(5, STEER5_Vertical_Base);  // 舵机5	2950
    Steer5_8_PWM_Control(6, STEER6_Vertical_Base);  // 舵机6 3450
    Steer5_8_PWM_Control(7, STEER7_Vertical_Base);  // 舵机7 3080
    Steer5_8_PWM_Control(8, STEER8_Vertical_Base);  // 舵机8 2900
}

