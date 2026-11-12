# STM32标准库开发说明

## 一、标准库简介

### 1.1 什么是标准库

**STM32标准外设库（Standard Peripheral Library，SPL）**：
- ST官方提供的固件库
- 封装了寄存器操作，提供函数接口
- 适用于STM32F1、F2、F4等系列
- 代码结构清晰，易于理解

### 1.2 标准库 vs HAL库

| 对比项 | 标准库（SPL） | HAL库 |
|--------|--------------|-------|
| 代码量 | 较少 | 较多 |
| 执行效率 | 较高 | 中等 |
| 可移植性 | 较低 | 较高 |
| 学习曲线 | 较陡 | 较平缓 |
| 适用系列 | F1/F2/F4 | 全系列 |

## 二、标准库基本使用

### 2.1 时钟使能

```c
// 使能GPIO时钟
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

// 使能外设时钟
RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);  // UART2
RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);  // UART1
RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);    // SPI1
```

### 2.2 GPIO操作

```c
// GPIO初始化
GPIO_InitTypeDef GPIO_InitStructure;

GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;  // 引脚
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 速度
GPIO_Init(GPIOA, &GPIO_InitStructure);

// GPIO读写
GPIO_SetBits(GPIOA, GPIO_Pin_0);   // 置高
GPIO_ResetBits(GPIOA, GPIO_Pin_0); // 置低
uint8_t state = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);  // 读取
```

### 2.3 UART操作

```c
// UART初始化
USART_InitTypeDef USART_InitStructure;

USART_InitStructure.USART_BaudRate = 115200;
USART_InitStructure.USART_WordLength = USART_WordLength_8b;
USART_InitStructure.USART_StopBits = USART_StopBits_1;
USART_InitStructure.USART_Parity = USART_Parity_No;
USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
USART_Init(USART1, &USART_InitStructure);
USART_Cmd(USART1, ENABLE);

// UART发送
void UART_SendByte(USART_TypeDef* USARTx, uint8_t byte) {
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
    USART_SendData(USARTx, byte);
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
}

// UART接收
uint8_t UART_ReceiveByte(USART_TypeDef* USARTx) {
    while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET);
    return USART_ReceiveData(USARTx);
}
```

### 2.4 SPI操作

```c
// SPI初始化
SPI_InitTypeDef SPI_InitStructure;

SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
SPI_Init(SPI1, &SPI_InitStructure);
SPI_Cmd(SPI1, ENABLE);

// SPI发送接收
uint8_t SPI_SendByte(SPI_TypeDef* SPIx, uint8_t byte) {
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPIx, byte);
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPIx);
}
```

### 2.5 I2C操作

```c
// I2C初始化
I2C_InitTypeDef I2C_InitStructure;

I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
I2C_InitStructure.I2C_OwnAddress1 = 0x00;
I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
I2C_InitStructure.I2C_ClockSpeed = 400000;  // 400kHz
I2C_Init(I2C1, &I2C_InitStructure);
I2C_Cmd(I2C1, ENABLE);
```

### 2.6 延时函数

```c
// 毫秒延时（需要配置SysTick）
void Delay_ms(uint32_t ms) {
    uint32_t temp;
    SysTick->LOAD = SystemCoreClock / 1000 * ms;
    SysTick->VAL = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    do {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL = 0X00;
}

// 微秒延时
void Delay_us(uint32_t us) {
    uint32_t temp;
    SysTick->LOAD = SystemCoreClock / 1000000 * us;
    SysTick->VAL = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    do {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL = 0X00;
}
```

### 2.7 系统时间获取

```c
// 获取系统运行时间（毫秒）
// 需要配置SysTick为1ms中断
volatile uint32_t systick_count = 0;

void SysTick_Handler(void) {
    systick_count++;
}

uint32_t GetTickCount(void) {
    return systick_count;
}

// SysTick初始化（1ms中断）
void SysTick_Init(void) {
    if (SysTick_Config(SystemCoreClock / 1000)) {
        while (1);  // 初始化失败
    }
}
```

## 三、中断配置

### 3.1 外部中断

```c
// 外部中断初始化
void EXTI_Init_Example(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    
    // 配置GPIO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置EXTI
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// 中断服务函数
void EXTI0_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        // 处理中断
        EXTI_ClearITPendingBit(EXTI_Line0);  // 清除中断标志
    }
}
```

### 3.2 UART中断

```c
// UART中断服务函数
void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        // 接收中断
        uint8_t data = USART_ReceiveData(USART1);
        // 处理数据
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
    
    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
        // 发送中断
        // 处理发送
        USART_ClearITPendingBit(USART1, USART_IT_TXE);
    }
}
```

## 四、常用函数对照表

### 4.1 GPIO函数对照

| HAL库 | 标准库 |
|-------|--------|
| HAL_GPIO_Init() | GPIO_Init() |
| HAL_GPIO_WritePin() | GPIO_SetBits() / GPIO_ResetBits() |
| HAL_GPIO_ReadPin() | GPIO_ReadInputDataBit() |
| HAL_GPIO_TogglePin() | GPIO_WriteBit() (使用XOR) |

### 4.2 UART函数对照

| HAL库 | 标准库 |
|-------|--------|
| HAL_UART_Init() | USART_Init() + USART_Cmd() |
| HAL_UART_Transmit() | USART_SendData() + 等待标志 |
| HAL_UART_Receive() | USART_ReceiveData() + 等待标志 |
| HAL_UART_Transmit_IT() | USART_ITConfig() + 中断处理 |

### 4.3 SPI函数对照

| HAL库 | 标准库 |
|-------|--------|
| HAL_SPI_Init() | SPI_Init() + SPI_Cmd() |
| HAL_SPI_Transmit() | SPI_I2S_SendData() + 等待标志 |
| HAL_SPI_Receive() | SPI_I2S_ReceiveData() + 等待标志 |

### 4.4 延时函数对照

| HAL库 | 标准库 |
|-------|--------|
| HAL_Delay() | Delay_ms() (需要自己实现) |
| HAL_GetTick() | GetTickCount() (需要自己实现) |

## 五、标准库项目结构

### 5.1 文件结构

```
项目目录/
├── Core/
│   ├── inc/
│   │   ├── stm32f4xx.h
│   │   ├── system_stm32f4xx.h
│   │   └── main.h
│   └── src/
│       ├── main.c
│       ├── system_stm32f4xx.c
│       └── stm32f4xx_it.c (中断服务函数)
├── Libraries/
│   ├── CMSIS/
│   ├── STM32F4xx_StdPeriph_Driver/
│   │   ├── inc/
│   │   └── src/
│   └── misc/
└── User/
    ├── dht22.c
    ├── hc_sr505.c
    └── ...
```

### 5.2 头文件包含

```c
// main.c
#include "stm32f4xx.h"
#include "system_stm32f4xx.h"
#include "misc.h"  // 中断优先级分组

// 外设库头文件
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_rcc.h"
```

## 六、注意事项

### 6.1 时钟配置

```c
// 系统初始化
int main(void) {
    SystemInit();  // 系统时钟初始化（72MHz或168MHz）
    // 其他初始化...
}
```

### 6.2 中断优先级分组

```c
// 中断优先级分组（在main函数开始处）
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 2位抢占优先级，2位响应优先级
```

### 6.3 延时函数实现

标准库不提供延时函数，需要自己实现：
- 使用SysTick实现
- 或使用定时器实现
- 或使用循环延时（不精确）

## 七、总结

标准库开发要点：
1. **时钟使能**：使用RCC_APBxPeriphClockCmd()
2. **GPIO操作**：使用GPIO_Init()、GPIO_SetBits()等
3. **外设初始化**：使用xxx_Init() + xxx_Cmd()
4. **中断配置**：使用EXTI_Init()、NVIC_Init()
5. **延时函数**：需要自己实现SysTick延时

标准库代码更简洁，执行效率更高，适合对性能要求较高的项目。
