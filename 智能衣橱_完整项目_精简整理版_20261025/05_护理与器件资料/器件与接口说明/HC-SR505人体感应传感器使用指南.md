# HC-SR505人体感应传感器使用指南

## 一、产品概述

### 1.1 产品信息

**型号**：HC-SR505迷你小型人体感应模块
- **类型**：被动红外（PIR）人体感应传感器
- **特点**：迷你小型，不可调，即插即用
- **价格**：约2-3元/个
- **尺寸**：约20×15mm（比HC-SR501更小）

### 1.2 主要特点

- **尺寸小巧**：比HC-SR501更小，适合空间受限场景
- **使用简单**：无需调节，即插即用
- **价格便宜**：比HC-SR501更便宜
- **功耗低**：适合电池供电应用
- **灵敏度高**：检测人体移动
- **稳定性好**：抗干扰能力强

### 1.3 技术参数

| 参数 | 数值 | 备注 |
|------|------|------|
| 工作电压 | 4.5V-20V | 推荐5V |
| 工作电流 | <60μA | 静态电流 |
| 输出电流 | <100μA | 输出高电平时 |
| 检测距离 | 3-5米 | 固定，不可调 |
| 检测角度 | ≤120° | 锥形检测范围 |
| 延时时间 | 3-5秒 | 固定，不可调 |
| 触发方式 | 可重复触发 | 检测到人体持续输出高电平 |
| 输出信号 | 数字信号（TTL） | 高电平3.3V，低电平0V |
| 响应时间 | <1秒 | |
| 工作温度 | -15~70°C | |
| 尺寸 | 约20×15mm | 迷你型 |

### 1.4 与HC-SR501对比

| 参数 | HC-SR505 | HC-SR501 |
|------|----------|----------|
| 尺寸 | 小（20×15mm） | 大（32×24mm） |
| 检测距离 | 3-5米（固定） | 3-7米（可调） |
| 延时时间 | 3-5秒（固定） | 0.3秒-5分钟（可调） |
| 灵敏度 | 固定 | 可调 |
| 价格 | 2-3元 | 3-5元 |
| 适用场景 | 固定需求，空间受限 | 需要灵活调节 |

## 二、工作原理

### 2.1 工作原理

**PIR（Passive Infrared）被动红外原理**：
- 人体会发出红外线（波长约9-10μm）
- 传感器检测红外线变化
- 当检测到人体移动时，输出高电平信号
- 人体离开后，延时3-5秒后输出低电平

**检测特点**：
- 检测的是**温度变化**，不是静态温度
- 需要**移动**才能检测到
- 对静止不动的人体不敏感

### 2.2 检测范围

**检测角度**：≤120°（锥形范围）

**检测距离**：3-5米（固定）

**检测示意图**：
```
        传感器
          │
          │
     ┌────┴────┐
     │  120°  │
     │  范围  │
     └────────┘
     3-5米距离
```

## 三、硬件连接

### 3.1 引脚定义

**HC-SR505模块引脚**（3引脚）：

| 引脚名称 | 功能说明 | 连接说明 |
|---------|---------|---------|
| VCC | 电源正极（4.5V-20V） | 接5V（推荐） |
| GND | 电源负极 | 接GND |
| OUT | 数字信号输出 | 接STM32 GPIO输入 |

**引脚排列**（从正面看）：
```
┌─────────┐
│  VCC    │  ← 电源正极
│  OUT    │  ← 信号输出
│  GND    │  ← 电源负极
└─────────┘
```

### 3.2 STM32连接方式

**推荐连接**：

```
HC-SR505模块  →    STM32F407VGT6
VCC          →     5V
GND          →     GND
OUT          →     PC0 (GPIO输入，配置为上拉输入)
```

**GPIO配置**：
- **模式**：GPIO_MODE_INPUT（输入模式）
- **上拉/下拉**：GPIO_PULLUP（上拉输入，推荐）或GPIO_NOPULL
- **速度**：GPIO_SPEED_FREQ_LOW（低速即可）

### 3.3 电源要求

**供电电压**：
- **范围**：4.5V-20V
- **推荐**：5V（最稳定）
- **注意**：不能低于4.5V，否则可能无法正常工作

**电流要求**：
- **静态电流**：<60μA（非常低）
- **工作电流**：<100μA
- **功耗**：极低，适合电池供电

## 四、STM32代码实现

### 4.1 GPIO初始化

```c
// HC-SR505 GPIO初始化（标准库）
void HC_SR505_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // 配置GPIO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;  // PC0
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  // 低速
    
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}
```

### 4.2 读取传感器状态

```c
// 读取HC-SR505状态（标准库）
uint8_t HC_SR505_Read(void) {
    // 读取GPIO电平
    // 高电平 = 检测到人体
    // 低电平 = 未检测到人体
    return GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
}

// 检查是否检测到人体
uint8_t HC_SR505_IsDetected(void) {
    return (HC_SR505_Read() == Bit_SET);
}
```

### 4.3 中断方式（推荐）

```c
// HC-SR505中断初始化（标准库）
void HC_SR505_Init_IT(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    
    // 配置GPIO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 配置EXTI（外部中断）
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource0);  // PC0映射到EXTI0
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  // 中断模式
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  // 上升沿和下降沿触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // 配置NVIC（中断优先级）
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// 外部中断服务函数
void EXTI0_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        if (HC_SR505_IsDetected()) {
            // 检测到人体
            LED_On();  // 开启照明
            printf("检测到人体，开启照明\n");
        } else {
            // 人体离开
            printf("人体离开，延时关闭照明\n");
            // 延时3-5秒后关闭（在任务中处理）
        }
        
        EXTI_ClearITPendingBit(EXTI_Line0);  // 清除中断标志
    }
}
```

### 4.4 状态机实现（推荐）

```c
// HC-SR505状态
typedef enum {
    HC_SR505_STATE_IDLE,      // 空闲状态
    HC_SR505_STATE_DETECTED,  // 检测到人体
    HC_SR505_STATE_DELAY      // 延时关闭状态
} HC_SR505_State_t;

HC_SR505_State_t hc_sr505_state = HC_SR505_STATE_IDLE;
uint32_t delay_start_time = 0;

// HC-SR505状态机处理
void HC_SR505_StateMachine(void) {
    uint8_t detected = HC_SR505_IsDetected();
    
    switch (hc_sr505_state) {
        case HC_SR505_STATE_IDLE:
            if (detected) {
                // 检测到人体，开启照明
                LED_On();
                hc_sr505_state = HC_SR505_STATE_DETECTED;
                printf("检测到人体，开启照明\n");
            }
            break;
            
        case HC_SR505_STATE_DETECTED:
            if (!detected) {
                // 人体离开，进入延时状态
                delay_start_time = GetTickCount();  // 标准库获取系统时间
                hc_sr505_state = HC_SR505_STATE_DELAY;
                printf("人体离开，开始延时\n");
            }
            break;
            
        case HC_SR505_STATE_DELAY:
            if (detected) {
                // 延时期间又检测到人体，保持开启
                hc_sr505_state = HC_SR505_STATE_DETECTED;
                printf("延时期间检测到人体，保持开启\n");
            } else if (GetTickCount() - delay_start_time >= 5000) {
                // 延时5秒后关闭
                LED_Off();
                hc_sr505_state = HC_SR505_STATE_IDLE;
                printf("延时结束，关闭照明\n");
            }
            break;
    }
}
```

### 4.5 完整使用示例

```c
// 主函数中使用（标准库）
int main(void) {
    // 系统初始化
    SystemInit();  // 系统时钟初始化
    Delay_Init();  // 延时函数初始化
    
    // 初始化
    HC_SR505_Init();
    LED_Init();
    
    while (1) {
        // 状态机处理（每100ms执行一次）
        HC_SR505_StateMachine();
        
        Delay_ms(100);  // 标准库延时函数
    }
}

// 或者使用中断方式
int main(void) {
    // 系统初始化
    SystemInit();
    Delay_Init();
    
    // 中断初始化
    HC_SR505_Init_IT();
    LED_Init();
    
    while (1) {
        // 主循环处理其他任务
        // 人体检测在中断中处理
        Delay_ms(1000);
    }
}
```

## 五、在智能衣柜中的应用

### 5.1 应用场景

**自动照明控制**：
- 用户打开衣柜门
- HC-SR505检测到人体
- 自动开启LED照明
- 用户离开后，延时3-5秒自动关闭

**安全保护**：
- UVC消毒时检测人体
- 检测到人体自动关闭UVC LED
- 保护用户安全

### 5.2 实现方案

**方案1：开门自动照明**

```c
// 开门自动照明
void Auto_Light_Control(void) {
    if (HC_SR505_IsDetected()) {
        // 检测到人体，开启照明
        WS2812B_SetBrightness(100);  // 100%亮度
        WS2812B_SetColor(255, 255, 255);  // 白光
        WS2812B_Update();
    } else {
        // 人体离开，延时关闭
        static uint32_t leave_time = 0;
        if (leave_time == 0) {
            leave_time = GetTickCount();  // 标准库获取系统时间
        }
        
        if (GetTickCount() - leave_time >= 5000) {  // 5秒后关闭
            WS2812B_SetBrightness(0);
            WS2812B_Update();
            leave_time = 0;
        }
    }
}
```

**方案2：UVC安全保护**

```c
// UVC安全保护
void UVC_Safety_Check(void) {
    if (UVC_IsRunning()) {
        // UVC运行时，检测人体
        if (HC_SR505_IsDetected()) {
            // 检测到人体，立即关闭UVC
            UVC_Stop();
            Buzzer_Beep(3);  // 蜂鸣器报警3次
            printf("警告：检测到人体，UVC已关闭\n");
        }
    }
}
```

**方案3：组合控制**

```c
// 综合控制：照明 + 安全保护
void Smart_Wardrobe_Control(void) {
    uint8_t human_detected = HC_SR505_IsDetected();
    
    // 1. 自动照明
    if (human_detected) {
        LED_Strip_On();
    } else {
        LED_Strip_Delay_Off(5000);  // 5秒后关闭
    }
    
    // 2. UVC安全保护
    if (human_detected && UVC_IsRunning()) {
        UVC_Stop();
        Safety_Alarm();
    }
    
    // 3. 其他功能（如语音识别激活等）
    if (human_detected) {
        Voice_Module_WakeUp();
    }
}
```

## 六、安装和调试

### 6.1 安装位置

**推荐位置**：
- **衣柜内部**：顶部或侧壁，朝向衣柜门
- **检测范围**：覆盖衣柜前方区域
- **高度**：1.5-2米（人体活动范围）

**安装示意图**：
```
┌─────────────────┐
│   衣柜顶部      │
│  ┌───────────┐ │
│  │ HC-SR505  │ │  ← 传感器位置
│  └───────────┘ │
│        │        │
│   120°检测范围  │
│        │        │
│    [用户区域]   │
└─────────────────┘
```

### 6.2 调试方法

**方法1：串口输出调试**

```c
// 调试输出
void HC_SR505_Debug(void) {
    static uint8_t last_state = 0;
    uint8_t current_state = HC_SR505_IsDetected();
    
    if (current_state != last_state) {
        if (current_state) {
            printf("HC-SR505: 检测到人体\n");
        } else {
            printf("HC-SR505: 人体离开\n");
        }
        last_state = current_state;
    }
}
```

**方法2：LED指示**

```c
// LED指示（标准库）
void HC_SR505_LED_Indicator(void) {
    if (HC_SR505_IsDetected()) {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);  // LED亮
    } else {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);  // LED灭
    }
}
```

**方法3：示波器/逻辑分析仪**

- 观察OUT引脚波形
- 检测到人体时：高电平（3.3V）
- 未检测到人体：低电平（0V）

### 6.3 测试步骤

1. **基础测试**：
   - 连接VCC、GND、OUT
   - 上电后等待30-60秒（传感器初始化）
   - 在传感器前挥手，观察输出变化

2. **距离测试**：
   - 测试不同距离的检测效果
   - 确认3-5米检测距离是否满足需求

3. **角度测试**：
   - 测试不同角度的检测范围
   - 确认120°检测角度是否覆盖所需区域

4. **延时测试**：
   - 测试延时时间（3-5秒）
   - 确认是否符合应用需求

## 七、常见问题

### 7.1 无响应

**问题**：传感器无响应，始终输出低电平

**解决方案**：
1. **检查电源**：
   - 确认VCC是否接5V
   - 确认GND是否共地
   - 测量实际电压是否≥4.5V

2. **等待初始化**：
   - 上电后需要等待30-60秒初始化
   - 初始化期间可能无响应

3. **检查接线**：
   - 确认OUT引脚是否接对
   - 确认GPIO配置是否正确

4. **环境因素**：
   - 避免强光直射
   - 避免温度剧烈变化
   - 避免强电磁干扰

### 7.2 误触发

**问题**：没有人体时也触发

**解决方案**：
1. **环境干扰**：
   - 避免热源干扰（如暖气、空调出风口）
   - 避免阳光直射
   - 避免小动物经过

2. **安装位置**：
   - 调整安装位置
   - 避免朝向热源

3. **灵敏度**：
   - HC-SR505不可调灵敏度
   - 如果误触发严重，考虑更换位置或使用HC-SR501

### 7.3 检测不到

**问题**：人体在检测范围内但检测不到

**解决方案**：
1. **距离检查**：
   - 确认距离是否在3-5米范围内
   - 距离过远可能检测不到

2. **移动检查**：
   - 传感器检测的是移动，不是静态
   - 需要人体移动才能检测到

3. **角度检查**：
   - 确认是否在120°检测角度内
   - 角度偏差可能检测不到

4. **环境温度**：
   - 环境温度与人体温度接近时，检测灵敏度降低
   - 建议环境温度<30°C

### 7.4 延时时间不合适

**问题**：3-5秒延时时间不合适

**解决方案**：
- **HC-SR505延时固定**，无法调节
- 如果延时不合适，可以：
  1. 使用软件延时（在STM32中实现）
  2. 更换为HC-SR501（可调延时）

**软件延时实现**：
```c
// 软件延时控制
void HC_SR505_Software_Delay(uint32_t delay_ms) {
    static uint32_t leave_time = 0;
    
    if (HC_SR505_IsDetected()) {
        leave_time = 0;  // 检测到人体，重置计时
        LED_On();
    } else {
        if (leave_time == 0) {
            leave_time = GetTickCount();  // 记录离开时间（标准库）
        }
        
        if (GetTickCount() - leave_time >= delay_ms) {
            LED_Off();  // 延时后关闭
        }
    }
}
```

## 八、注意事项

### 8.1 使用注意事项

1. **初始化时间**：
   - 上电后需要30-60秒初始化
   - 初始化期间可能无响应或误触发

2. **检测特点**：
   - 检测的是**移动**，不是静态
   - 静止不动的人体可能检测不到
   - 需要温度变化才能检测

3. **环境要求**：
   - 避免强光直射
   - 避免温度剧烈变化
   - 避免强电磁干扰
   - 工作温度：-15~70°C

4. **安装要求**：
   - 安装高度：1.5-2米
   - 朝向：朝向检测区域
   - 固定：牢固固定，避免振动

### 8.2 局限性

1. **不可调节**：
   - 检测距离固定（3-5米）
   - 延时时间固定（3-5秒）
   - 灵敏度不可调

2. **检测限制**：
   - 只能检测移动的人体
   - 静止不动检测不到
   - 环境温度影响灵敏度

3. **误触发**：
   - 可能受热源干扰
   - 可能受小动物干扰
   - 强光可能影响

### 8.3 适用场景

**适合**：
- ✅ 固定延时需求（3-5秒）
- ✅ 空间受限场景
- ✅ 成本敏感项目
- ✅ 简单应用场景

**不适合**：
- ❌ 需要可调延时
- ❌ 需要可调灵敏度
- ❌ 需要精确控制
- ❌ 复杂应用场景

## 九、优化建议

### 9.1 软件优化

**防抖处理**：
```c
// 防抖处理（避免误触发）
uint8_t HC_SR505_Read_WithDebounce(void) {
    static uint8_t last_state = 0;
    static uint32_t stable_time = 0;
    uint8_t current_state = HC_SR505_Read();
    
    if (current_state == last_state) {
        stable_time = HAL_GetTick();
    } else {
        // 状态变化，等待稳定
        if (HAL_GetTick() - stable_time > 100) {  // 100ms稳定
            last_state = current_state;
        }
    }
    
    return last_state;
}
```

**多次采样**：
```c
// 多次采样取平均值
uint8_t HC_SR505_Read_Multiple(uint8_t samples) {
    uint8_t count = 0;
    for (int i = 0; i < samples; i++) {
        if (HC_SR505_Read()) {
            count++;
        }
        HAL_Delay(10);
    }
    return (count > samples / 2);  // 超过一半为高电平
}
```

### 9.2 硬件优化

**滤波电路**：
- 在OUT引脚添加RC滤波电路
- 减少干扰和误触发

**屏蔽**：
- 使用屏蔽线连接
- 减少电磁干扰

## 十、总结

HC-SR505是一款迷你小型人体感应传感器，适合智能衣柜项目使用。

**优点**：
- ✅ 尺寸小，安装方便
- ✅ 价格便宜（2-3元）
- ✅ 使用简单，无需调节
- ✅ 功耗低，适合电池供电
- ✅ 固定延时满足基本需求

**关键点**：
1. 上电后需要30-60秒初始化
2. 检测的是移动，不是静态
3. 延时时间固定（3-5秒），不可调
4. 注意环境因素影响

**在智能衣柜中的应用**：
- 开门自动照明
- UVC安全保护
- 语音模块唤醒
- 其他人体感应功能

**推荐使用场景**：
- 固定延时需求（3-5秒足够）
- 空间受限场景
- 成本敏感项目
- 简单应用场景

如果需要对延时和灵敏度进行灵活调节，建议使用HC-SR501。
