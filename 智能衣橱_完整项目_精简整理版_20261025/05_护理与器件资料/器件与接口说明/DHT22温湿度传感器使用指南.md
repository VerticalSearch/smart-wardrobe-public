# DHT22温湿度传感器使用指南

## 一、产品概述

### 1.1 产品信息

**型号**：DHT22（AM2302）
- **全称**：DHT22数字温湿度传感器模块
- **别名**：AM2302（带线版本）
- **价格**：约6-8元/个
- **品牌**：Aosong（奥松）

### 1.2 主要特点

- **数字输出**：单总线数字信号输出
- **高精度**：温度±0.5°C，湿度±2%RH
- **宽量程**：温度-40~80°C，湿度0~100%RH
- **低功耗**：工作电流1.5mA，待机电流60μA
- **长期稳定**：响应快，抗干扰能力强
- **校准**：出厂已校准，无需额外校准

### 1.3 技术参数

| 参数 | 数值 | 备注 |
|------|------|------|
| 温度精度 | ±0.5°C | 25°C时 |
| 温度分辨率 | 0.1°C | 16位 |
| 温度量程 | -40~80°C | |
| 湿度精度 | ±2%RH | 25°C时 |
| 湿度分辨率 | 0.1%RH | 16位 |
| 湿度量程 | 0~100%RH | |
| 响应时间 | <2秒 | |
| 采样周期 | ≥2秒 | 建议间隔 |
| 工作电压 | 3.3V-5.5V | DC |
| 工作电流 | 1.5mA | 测量时 |
| 待机电流 | 60μA | |
| 接口 | 单总线（1-Wire） | |
| 引脚数 | 3个（VCC、GND、DATA） | 或4个（带NC） |

## 二、硬件连接

### 2.1 引脚定义

**DHT22模块引脚**（3引脚版本）：

| 引脚名称 | 功能说明 | 颜色（带线版） |
|---------|---------|--------------|
| VCC | 电源正极（3.3V-5V） | 红色 |
| DATA | 数据线（单总线） | 黄色/白色 |
| GND | 电源负极 | 黑色 |

**4引脚版本**（带NC空引脚）：
- VCC、DATA、NC（空）、GND

### 2.2 STM32连接方式

**连接方案（推荐）**：

```
DHT22模块    →    STM32F407VGT6
VCC         →     3.3V或5V（推荐5V，更稳定）
GND         →     GND
DATA        →     PA0/PA1/PA2 (GPIO输入，配置为上拉输入)
```

**多个DHT22连接**（多点监测）：
```
DHT22-1 DATA → PA0 (GPIO)
DHT22-2 DATA → PA1 (GPIO)
DHT22-3 DATA → PA2 (GPIO)
VCC和GND共用
```

### 2.3 上拉电阻

**重要**：DATA引脚需要上拉电阻（通常模块已内置）

- **模块内置**：大多数DHT22模块已内置4.7kΩ-10kΩ上拉电阻
- **外部添加**：如果没有内置，需要在DATA和VCC之间接4.7kΩ-10kΩ上拉电阻
- **STM32内部上拉**：可以启用STM32 GPIO内部上拉（约40kΩ），但建议使用外部上拉电阻

**上拉电阻连接**：
```
VCC ──[4.7kΩ]── DATA ── STM32 GPIO
```

## 三、通信协议

### 3.1 单总线协议

DHT22使用单总线（1-Wire）协议，只需要一根数据线即可通信。

**协议特点**：
- 单线双向通信
- 需要严格的时序控制
- 数据格式：40位（5字节）

### 3.2 通信时序

**启动信号**（主机→DHT22）：
1. 主机拉低DATA线至少18ms
2. 主机拉高DATA线20-40μs
3. DHT22响应：拉低80μs，然后拉高80μs

**数据读取**（DHT22→主机）：
- 数据"0"：低电平50μs，高电平26-28μs
- 数据"1"：低电平50μs，高电平70μs

**时序图**：
```
主机:  ────┐                    ┌──
           │                    │
           └──────18ms──────────┘
           
DHT22:                    ┌─┐ ┌─┐
                          │ │ │ │
                          └─┘ └─┘
                          80μs 80μs
```

### 3.3 数据格式

**40位数据格式**：

| 字节 | 位 | 数据内容 |
|------|---|---------|
| 字节0 | 8位 | 湿度高字节（整数部分） |
| 字节1 | 8位 | 湿度低字节（小数部分） |
| 字节2 | 8位 | 温度高字节（整数部分，最高位为符号位） |
| 字节3 | 8位 | 温度低字节（小数部分） |
| 字节4 | 8位 | 校验和（前4字节之和） |

**数据解析**：
```c
// 湿度计算
float humidity = (data[0] * 256 + data[1]) / 10.0;  // 单位：%RH

// 温度计算
float temperature = ((data[2] & 0x7F) * 256 + data[3]) / 10.0;  // 单位：°C
if (data[2] & 0x80) {
    temperature = -temperature;  // 负温度
}

// 校验
uint8_t checksum = data[0] + data[1] + data[2] + data[3];
if (checksum != data[4]) {
    // 数据错误
}
```

## 四、STM32代码实现

### 4.1 GPIO配置

```c
// DHT22 GPIO配置（标准库）
void DHT22_GPIO_Init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    if (GPIOx == GPIOA) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    } else if (GPIOx == GPIOB) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    } else if (GPIOx == GPIOC) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    }
    
    // 配置为推挽输出（用于发送启动信号）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  // 低速
    GPIO_Init(GPIOx, &GPIO_InitStructure);
    
    // 初始状态：输出高电平
    GPIO_SetBits(GPIOx, GPIO_Pin);
}
```

### 4.2 微秒级延时函数

```c
// 微秒级延时（标准库，基于SysTick）
// 需要先配置SysTick为1MHz（1us计数一次）
void DHT22_Delay_us(uint16_t us) {
    uint32_t temp;
    SysTick->LOAD = SystemCoreClock / 1000000 * us;  // 设置重装载值
    SysTick->VAL = 0x00;  // 清空当前值
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  // 启动计数器
    do {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));  // 等待计数到0
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;  // 关闭计数器
    SysTick->VAL = 0X00;  // 清空当前值
}

// 或者使用简单的循环延时（不精确，但简单）
void DHT22_Delay_us_Simple(uint16_t us) {
    uint32_t delay = us * (SystemCoreClock / 1000000) / 4;
    while (delay--) {
        __NOP();
    }
}
```

### 4.3 启动信号发送

```c
// 发送启动信号（标准库）
void DHT22_Start_Signal(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 配置为输出模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOx, &GPIO_InitStructure);
    
    // 拉低至少18ms
    GPIO_ResetBits(GPIOx, GPIO_Pin);
    Delay_ms(20);  // 20ms，确保足够（标准库延时函数）
    
    // 拉高20-40μs
    GPIO_SetBits(GPIOx, GPIO_Pin);
    DHT22_Delay_us(30);
    
    // 切换为输入模式（带上拉）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_Init(GPIOx, &GPIO_InitStructure);
}
```

### 4.4 等待响应

```c
// 等待DHT22响应（标准库）
uint8_t DHT22_Wait_Response(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t level, uint16_t timeout) {
    uint16_t count = 0;
    uint8_t pin_state;
    
    while (1) {
        pin_state = GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
        
        if (level == 0) {
            // 等待低电平
            if (pin_state == Bit_RESET) {
                return 1;  // 成功
            }
        } else {
            // 等待高电平
            if (pin_state == Bit_SET) {
                return 1;  // 成功
            }
        }
        
        if (++count > timeout) {
            return 0;  // 超时
        }
        DHT22_Delay_us(1);
    }
}
```

### 4.5 读取一个字节

```c
// 读取一个字节（8位，标准库）
uint8_t DHT22_Read_Byte(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    uint8_t byte = 0;
    
    for (int i = 0; i < 8; i++) {
        // 等待低电平结束（50μs）
        DHT22_Wait_Response(GPIOx, GPIO_Pin, 0, 100);
        
        // 延时30μs后采样
        DHT22_Delay_us(30);
        
        // 读取数据位
        byte <<= 1;
        if (GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) == Bit_SET) {
            byte |= 1;  // 数据"1"
        }
        
        // 等待高电平结束
        DHT22_Wait_Response(GPIOx, GPIO_Pin, 1, 100);
    }
    
    return byte;
}
```

### 4.6 完整读取函数

```c
// DHT22数据结构
typedef struct {
    float temperature;  // 温度（°C）
    float humidity;     // 湿度（%RH）
    uint8_t valid;      // 数据有效标志
} DHT22_Data_t;

// 读取DHT22数据
uint8_t DHT22_Read(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, DHT22_Data_t *data) {
    uint8_t buffer[5] = {0};
    
    // 1. 发送启动信号
    DHT22_Start_Signal(GPIOx, GPIO_Pin);
    
    // 2. 等待DHT22响应（拉低80μs）
    if (!DHT22_Wait_Response(GPIOx, GPIO_Pin, GPIO_PIN_RESET, 100)) {
        return 0;  // 响应超时
    }
    
    // 3. 等待DHT22拉高（80μs）
    if (!DHT22_Wait_Response(GPIOx, GPIO_Pin, GPIO_PIN_SET, 100)) {
        return 0;
    }
    
    // 4. 读取40位数据（5字节）
    for (int i = 0; i < 5; i++) {
        buffer[i] = DHT22_Read_Byte(GPIOx, GPIO_Pin);
    }
    
    // 5. 校验数据
    uint8_t checksum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    if (checksum != buffer[4]) {
        return 0;  // 校验失败
    }
    
    // 6. 解析数据
    data->humidity = (buffer[0] * 256.0 + buffer[1]) / 10.0;
    
    // 温度（注意符号位）
    int16_t temp_raw = (buffer[2] & 0x7F) * 256 + buffer[3];
    data->temperature = temp_raw / 10.0;
    if (buffer[2] & 0x80) {
        data->temperature = -data->temperature;  // 负温度
    }
    
    data->valid = 1;
    return 1;  // 读取成功
}
```

### 4.7 使用示例

```c
// 初始化（标准库）
void DHT22_Init(void) {
    DHT22_GPIO_Init(GPIOA, GPIO_Pin_0);  // PA0连接DHT22
}

// 主函数中使用
int main(void) {
    DHT22_Data_t dht22_data;
    
    // 系统初始化（标准库）
    SystemInit();  // 系统时钟初始化
    Delay_Init();  // 延时函数初始化（需要自己实现或使用标准库延时）
    
    DHT22_Init();
    
    while (1) {
        // 读取数据（注意：间隔至少2秒）
        if (DHT22_Read(GPIOA, GPIO_Pin_0, &dht22_data)) {
            printf("温度: %.1f°C\n", dht22_data.temperature);
            printf("湿度: %.1f%%RH\n", dht22_data.humidity);
        } else {
            printf("读取失败\n");
        }
        
        Delay_ms(2000);  // 等待2秒后再读取（标准库延时函数）
    }
}
```

## 五、多点监测实现

### 5.1 多个DHT22连接

```c
// 定义多个DHT22的GPIO
#define DHT22_1_PORT  GPIOA
#define DHT22_1_PIN   GPIO_PIN_0

#define DHT22_2_PORT  GPIOA
#define DHT22_2_PIN   GPIO_PIN_1

#define DHT22_3_PORT  GPIOA
#define DHT22_3_PIN   GPIO_PIN_2

// 读取所有传感器（标准库）
void DHT22_Read_All(DHT22_Data_t *data_array) {
    DHT22_Read(DHT22_1_PORT, DHT22_1_PIN, &data_array[0]);  // 上
    Delay_ms(100);
    DHT22_Read(DHT22_2_PORT, DHT22_2_PIN, &data_array[1]);  // 中
    Delay_ms(100);
    DHT22_Read(DHT22_3_PORT, DHT22_3_PIN, &data_array[2]);  // 下
}

// 计算平均值
void DHT22_Get_Average(DHT22_Data_t *data_array, uint8_t count, float *avg_temp, float *avg_humidity) {
    float sum_temp = 0, sum_humidity = 0;
    uint8_t valid_count = 0;
    
    for (int i = 0; i < count; i++) {
        if (data_array[i].valid) {
            sum_temp += data_array[i].temperature;
            sum_humidity += data_array[i].humidity;
            valid_count++;
        }
    }
    
    if (valid_count > 0) {
        *avg_temp = sum_temp / valid_count;
        *avg_humidity = sum_humidity / valid_count;
    }
}
```

## 六、在智能衣柜中的应用

### 6.1 应用场景

**多点监测**：
- **位置1**：衣柜上部（监测顶部环境）
- **位置2**：衣柜中部（主要监测区域）
- **位置3**：衣柜下部（监测底部环境）

**功能**：
- 实时监测温湿度
- 智能除湿控制（湿度>60%开启除湿）
- 环境异常报警
- 数据记录和分析

### 6.2 控制逻辑

```c
// 环境控制逻辑
void Env_Control_Task(void) {
    DHT22_Data_t dht22_data[3];
    float avg_humidity, avg_temp;
    
    // 读取所有传感器
    DHT22_Read_All(dht22_data);
    
    // 计算平均值
    DHT22_Get_Average(dht22_data, 3, &avg_temp, &avg_humidity);
    
    // 控制决策
    if (avg_humidity > 70.0) {
        // 湿度过高，开启高速除湿
        Dehumidifier_SetPower(100);
        Fan_SetSpeed(100);
    } else if (avg_humidity > 60.0) {
        // 湿度偏高，开启中速除湿
        Dehumidifier_SetPower(60);
        Fan_SetSpeed(60);
    } else {
        // 湿度正常，关闭除湿
        Dehumidifier_SetPower(0);
        Fan_SetSpeed(0);
    }
    
    // 温度控制
    if (avg_temp > 30.0) {
        // 温度过高，开启通风降温
        Fan_Enable();
    }
}
```

## 七、常见问题

### 7.1 读取失败

**问题**：DHT22无响应或读取失败

**解决方案**：
1. **检查接线**：
   - VCC是否接3.3V或5V
   - GND是否共地
   - DATA线是否接对

2. **检查上拉电阻**：
   - 确认模块是否有内置上拉电阻
   - 如果没有，添加4.7kΩ-10kΩ上拉电阻

3. **检查时序**：
   - 启动信号拉低时间是否足够（≥18ms）
   - 微秒延时是否准确

4. **检查采样间隔**：
   - 两次读取间隔是否≥2秒
   - 避免频繁读取

### 7.2 数据异常

**问题**：读取的温度或湿度值异常

**解决方案**：
1. **校验和检查**：确认校验和是否正确
2. **多次读取**：连续读取3次，取平均值
3. **环境因素**：
   - 避免强风直吹
   - 避免阳光直射
   - 避免靠近热源

### 7.3 响应慢

**问题**：读取响应时间长

**解决方案**：
1. **增加超时时间**：适当增加等待超时时间
2. **检查电源**：确保电源稳定，推荐5V供电
3. **检查线长**：数据线不要过长（建议<20米）

## 八、注意事项

### 8.1 使用注意事项

1. **采样间隔**：
   - 两次读取间隔必须≥2秒
   - 建议间隔3-5秒

2. **环境要求**：
   - 避免强风直吹传感器
   - 避免阳光直射
   - 避免靠近热源或冷源
   - 保持传感器表面清洁

3. **电源要求**：
   - 推荐使用5V供电（更稳定）
   - 电源纹波要小
   - 注意电源去耦

4. **接线要求**：
   - 数据线不要过长
   - 避免强干扰源
   - 注意共地

### 8.2 精度说明

- **温度精度**：±0.5°C（25°C时）
- **湿度精度**：±2%RH（25°C时）
- **长期稳定性**：温度±0.1°C/年，湿度±1%RH/年

### 8.3 存储和保养

- **存储**：干燥环境，避免高温高湿
- **清洁**：用软布轻轻擦拭传感器表面
- **保护**：避免物理损坏

## 九、调试方法

### 9.1 串口调试

```c
// 添加调试输出
void DHT22_Debug_Print(DHT22_Data_t *data) {
    if (data->valid) {
        printf("DHT22读取成功:\n");
        printf("  温度: %.1f°C\n", data->temperature);
        printf("  湿度: %.1f%%RH\n", data->humidity);
    } else {
        printf("DHT22读取失败\n");
    }
}
```

### 9.2 示波器调试

- 使用示波器观察DATA线波形
- 检查启动信号时序
- 检查数据位时序
- 确认高低电平时间是否符合规范

### 9.3 测试步骤

1. **基础测试**：
   - 测试GPIO读写是否正常
   - 测试微秒延时是否准确

2. **通信测试**：
   - 发送启动信号
   - 检查DHT22响应

3. **数据测试**：
   - 读取数据
   - 校验数据
   - 对比实际环境

## 十、总结

DHT22是一款高精度的数字温湿度传感器，适合智能衣柜项目使用。

**优点**：
- ✅ 精度高（温度±0.5°C，湿度±2%RH）
- ✅ 接口简单（单总线）
- ✅ 价格便宜（6-8元）
- ✅ 使用方便（出厂已校准）

**关键点**：
1. 采样间隔必须≥2秒
2. 需要上拉电阻（通常模块已内置）
3. 时序要求严格，需要精确的微秒延时
4. 注意环境因素影响

**在智能衣柜中的应用**：
- 多点监测（上、中、下）
- 智能除湿控制
- 环境异常报警
- 数据记录和分析
