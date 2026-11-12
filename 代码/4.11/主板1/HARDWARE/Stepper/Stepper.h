/*
******************************************************************************
*Filename      :Stepper.h
*Programmer(s) :基于张大头Emm_V5.0步进电机控制代码移植到F407VGT6
*Description   :F407VGT6串口5 步进电机控制驱动 - 简化版
*Date         :2024
******************************************************************************
*/

#ifndef __STEPPER_H
#define __STEPPER_H	 

#include "sys.h"
#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

// 步进电机控制协议定义
#define STEPPER_CMD_LEN 16
#define STEPPER_CHECKSUM 0x6B

// 步进电机参数定义（与stepper.c中保持一致）
#define STEPPER_STEPS_PER_REVOLUTION 3200U  // 每圈脉冲数(16细分)
#define STEPPER_MAX_SPEED 10000            // 最大转速 RPM
#define STEPPER_MAX_ACCELERATION 255       // 最大加速度值 (0-255, 0=直接启动, 255=最慢加速)
#define STEPPER_DEFAULT_ACCELERATION 100   // 默认加速度值 (推荐值，可根据实际情况调整)

/*
******************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
*                                                   步进电机控制函数
******************************************************************************************************************
*/

// 步进电机初始化
void stepper_init(uint32_t baudrate);

// 基础控制函数：默认使用主板1 UART5，保留原有接口
void stepper_enable(uint8_t addr, bool enable);           // 使能/失能步进电机
void stepper_stop(uint8_t addr);                         // 立即停止运动

// 速度控制函数
void stepper_set_speed(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc);  // 设置转速、方向和加速度

// 位置控制函数
void stepper_move_angle(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, float angle);  // 按角度运动
void stepper_move_steps(uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, uint32_t steps);  // 按步数运动

// 加减速控制函数
void stepper_move_with_acceleration(uint8_t addr, uint8_t dir, uint16_t target_speed, uint8_t acc, float angle);  // 加减速运动到指定角度
void stepper_move_trapezoid(uint8_t addr, uint8_t dir, uint16_t target_speed, uint8_t acc, 
                            float accel_angle, float constant_angle, float decel_angle);  // 梯形加减速运动（三段式控制）

// 内部通信函数
void stepper_send_command(uint8_t* cmd, uint8_t length);  // 发送步进电机控制命令

// 指定串口控制函数：用于主板1 UART4 热风机移动轴等独立串口步进电机
void stepper_send_command_on_uart(USART_TypeDef* USARTxx, uint8_t* cmd, uint8_t length);
void stepper_enable_on_uart(USART_TypeDef* USARTxx, uint8_t addr, bool enable);
void stepper_stop_on_uart(USART_TypeDef* USARTxx, uint8_t addr);
void stepper_set_speed_on_uart(USART_TypeDef* USARTxx, uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc);
void stepper_move_angle_on_uart(USART_TypeDef* USARTxx, uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, float angle);
void stepper_move_steps_on_uart(USART_TypeDef* USARTxx, uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, uint32_t steps);

#endif
