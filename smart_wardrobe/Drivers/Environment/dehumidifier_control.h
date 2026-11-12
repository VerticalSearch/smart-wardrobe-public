/**
  ******************************************************************************
  * @file    dehumidifier_control.h
  * @brief   除湿器控制系统头文件
  ******************************************************************************
  */

#ifndef __DEHUMIDIFIER_CONTROL_H
#define __DEHUMIDIFIER_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

/* 除湿器状态 */
typedef struct {
    uint8_t enable;        // 使能状态
    uint8_t power;         // 功率 (0-100%)
    float current_temp;    // 当前温度
    uint32_t run_time;     // 运行时间 (秒)
} DehumidifierStatus_t;

/* 函数声明 */

/**
  * @brief  初始化除湿器控制系统
  */
void Dehumidifier_Init(void);

/**
  * @brief  使能/禁用除湿器
  * @param  enable: 使能/禁用
  */
void Dehumidifier_Enable(uint8_t enable);

/**
  * @brief  设置除湿器功率
  * @param  power: 功率 (0-100%)
  */
void Dehumidifier_SetPower(uint8_t power);

/**
  * @brief  根据湿度差值自动调节功率
  * @param  humidity_diff: 湿度差值 (当前湿度 - 目标湿度)
  */
void Dehumidifier_AutoControl(float humidity_diff);

/**
  * @brief  获取除湿器状态
  * @param  status: 状态指针
  */
void Dehumidifier_GetStatus(DehumidifierStatus_t *status);

/**
  * @brief  除湿器保护（防止过热）
  */
void Dehumidifier_Protection(void);

#endif /* __DEHUMIDIFIER_CONTROL_H */
