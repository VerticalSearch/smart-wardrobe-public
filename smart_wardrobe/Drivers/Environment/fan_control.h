/**
  ******************************************************************************
  * @file    fan_control.h
  * @brief   风扇控制系统头文件
  ******************************************************************************
  */

#ifndef __FAN_CONTROL_H
#define __FAN_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

/* 风扇类型 */
typedef enum {
    FAN_INTAKE = 0,    // 进风扇
    FAN_EXHAUST        // 排风扇
} FanType_t;

/* 风扇状态 */
typedef struct {
    uint8_t enable;        // 使能状态
    uint8_t speed;         // 速度 (0-100%)
    uint8_t direction;     // 方向 (0: 正转, 1: 反转)
} FanStatus_t;

/* 函数声明 */

/**
  * @brief  初始化风扇控制系统
  */
void Fan_Init(void);

/**
  * @brief  使能/禁用风扇
  * @param  fan_type: 风扇类型
  * @param  enable: 使能/禁用
  */
void Fan_Enable(FanType_t fan_type, uint8_t enable);

/**
  * @brief  设置风扇速度
  * @param  fan_type: 风扇类型
  * @param  speed: 速度 (0-100%)
  */
void Fan_SetSpeed(FanType_t fan_type, uint8_t speed);

/**
  * @brief  设置风扇方向
  * @param  fan_type: 风扇类型
  * @param  direction: 方向 (0: 正转, 1: 反转)
  */
void Fan_SetDirection(FanType_t fan_type, uint8_t direction);

/**
  * @brief  获取风扇状态
  * @param  fan_type: 风扇类型
  * @param  status: 状态指针
  */
void Fan_GetStatus(FanType_t fan_type, FanStatus_t *status);

/**
  * @brief  同时控制两个风扇
  * @param  intake_speed: 进风扇速度 (0-100%)
  * @param  exhaust_speed: 排风扇速度 (0-100%)
  */
void Fan_ControlBoth(uint8_t intake_speed, uint8_t exhaust_speed);

/**
  * @brief  关闭所有风扇
  */
void Fan_DisableAll(void);

#endif /* __FAN_CONTROL_H */
