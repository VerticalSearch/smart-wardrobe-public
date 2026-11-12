/**
  ******************************************************************************
  * @file    env_control.h
  * @brief   智能衣柜环境控制系统头文件
  ******************************************************************************
  */

#ifndef __ENV_CONTROL_H
#define __ENV_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

/* 环境控制状态 */
typedef enum {
    ENV_STATE_NORMAL = 0,          // 正常状态
    ENV_STATE_HUMIDITY_HIGH,       // 湿度偏高
    ENV_STATE_DEHUMIDIFYING,       // 正在除湿
    ENV_STATE_VENTILATING,         // 正在通风
    ENV_STATE_ALARM,               // 报警状态
    ENV_STATE_MANUAL               // 手动模式
} EnvState_t;

/* 风扇类型 */
typedef enum {
    FAN_INTAKE = 0,    // 进风扇
    FAN_EXHAUST,       // 排风扇
    FAN_BOTH           // 两个风扇
} FanType_t;

/* 环境参数结构体 */
typedef struct {
    float temperature;     // 温度 (°C)
    float humidity;         // 湿度 (%RH)
    uint16_t air_quality;   // 空气质量 (0-1000, 可选)
    uint32_t timestamp;     // 时间戳
} EnvData_t;

/* 环境控制参数 */
typedef struct {
    float target_humidity;      // 目标湿度 (%RH)
    float humidity_threshold_high;  // 湿度上限阈值 (%RH)
    float humidity_threshold_low;   // 湿度下限阈值 (%RH)
    float temp_threshold_high;   // 温度上限阈值 (°C)
    float temp_threshold_low;    // 温度下限阈值 (°C)
    uint8_t auto_mode;          // 自动模式使能
    uint8_t ventilation_duration; // 通风时长 (分钟)
} EnvConfig_t;

/* 控制输出结构体 */
typedef struct {
    uint8_t dehumidifier_enable;  // 除湿器使能
    uint8_t dehumidifier_power;   // 除湿器功率 (0-100%)
    uint8_t fan_intake_enable;    // 进风扇使能
    uint8_t fan_intake_speed;     // 进风扇速度 (0-100%)
    uint8_t fan_exhaust_enable;   // 排风扇使能
    uint8_t fan_exhaust_speed;    // 排风扇速度 (0-100%)
    uint8_t led_color;            // LED颜色 (0-7)
} ControlOutput_t;

/* 函数声明 */

/**
  * @brief  初始化环境控制系统
  */
void EnvControl_Init(void);

/**
  * @brief  环境控制主任务（在FreeRTOS任务中调用）
  */
void EnvControl_Task(void);

/**
  * @brief  获取当前环境数据
  * @param  data: 环境数据指针
  * @retval 0: 成功, 其他: 失败
  */
uint8_t EnvControl_GetData(EnvData_t *data);

/**
  * @brief  获取当前控制状态
  * @retval 环境控制状态
  */
EnvState_t EnvControl_GetState(void);

/**
  * @brief  设置环境控制参数
  * @param  config: 配置参数指针
  */
void EnvControl_SetConfig(EnvConfig_t *config);

/**
  * @brief  获取环境控制参数
  * @param  config: 配置参数指针
  */
void EnvControl_GetConfig(EnvConfig_t *config);

/**
  * @brief  手动控制除湿器
  * @param  enable: 使能/禁用
  * @param  power: 功率 (0-100%)
  */
void EnvControl_ManualDehumidifier(uint8_t enable, uint8_t power);

/**
  * @brief  手动控制风扇
  * @param  fan_type: 风扇类型
  * @param  enable: 使能/禁用
  * @param  speed: 速度 (0-100%)
  */
void EnvControl_ManualFan(FanType_t fan_type, uint8_t enable, uint8_t speed);

/**
  * @brief  设置自动模式
  * @param  enable: 使能/禁用自动模式
  */
void EnvControl_SetAutoMode(uint8_t enable);

/**
  * @brief  获取报警状态
  * @retval 0: 正常, 1: 报警
  */
uint8_t EnvControl_GetAlarmStatus(void);

/**
  * @brief  清除报警
  */
void EnvControl_ClearAlarm(void);

/**
  * @brief  保存环境数据到Flash
  */
void EnvControl_SaveData(void);

/**
  * @brief  获取历史环境数据
  * @param  data: 数据缓冲区
  * @param  count: 数据数量
  * @param  start_time: 起始时间戳
  * @retval 实际读取的数据数量
  */
uint16_t EnvControl_GetHistoryData(EnvData_t *data, uint16_t count, uint32_t start_time);

#endif /* __ENV_CONTROL_H */
