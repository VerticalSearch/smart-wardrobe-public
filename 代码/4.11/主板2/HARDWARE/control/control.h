#ifndef __CONTROL_H
#define __CONTROL_H

#include "sys.h"
#include <stdint.h>
#include "stepper_multi.h"

/* ========================== 系统时基参数（可调） ========================== */
#define BOARD2_TICK_PERIOD_MS               1U      /* TIM7基础节拍，单位ms */
#define BOARD2_UART1_SERVICE_PERIOD_MS      10U     /* UART1命令服务周期，单位ms */
#define BOARD2_KEY_SCAN_PERIOD_MS           10U     /* 按键扫描周期，单位ms */
#define BOARD2_KEY_DEBOUNCE_COUNT           2U      /* 去抖计数阈值（越大越稳，响应越慢） */
#define BOARD2_KEY_REPEAT_ENABLE            0U      /* 连按开关：0关闭，1开启 */
#define BOARD2_KEY_REPEAT_PERIOD_MS         200U    /* 连按触发周期，单位ms */

/* ADC1~ADC4 复用作普通数字输入，硬件低有效，软件输出高有效 */
#define BOARD2_ADC_IO_COUNT                 4U
#define BOARD2_ADC_IO1_MASK                 0x01U   /* ADC1: PC0, 外部拉低后该bit为1 */
#define BOARD2_ADC_IO2_MASK                 0x02U   /* ADC2: PC1, 外部拉低后该bit为1 */
#define BOARD2_ADC_IO3_MASK                 0x04U   /* ADC3: PC2, 外部拉低后该bit为1 */
#define BOARD2_ADC_IO4_MASK                 0x08U   /* ADC4: PC3, 外部拉低后该bit为1 */

/* 加速度参数限幅到协议1字节范围（0~255） */
#define BOARD2_ACCEL_U8(x)                  ((uint8_t)(((x) > 255U) ? 255U : (x)))
#define BOARD2_OPPOSITE_DIR(dir)            ((uint8_t)(((dir) == 0U) ? 1U : 0U))

/* ========================== 轴通道映射（按硬件修改） ========================== */
#define BOARD2_AXIS_X_CHANNEL               STEPPER_CH1_UART2   /* X轴使用UART2 */
#define BOARD2_AXIS_Y_CHANNEL               STEPPER_CH2_UART3   /* Y轴使用UART3 */
#define BOARD2_AXIS_Z_CHANNEL               STEPPER_CH3_UART4   /* Z轴使用UART4 */
#define BOARD2_AXIS_TELESCOPE_CHANNEL       STEPPER_CH4_UART5   /* 伸缩杆使用UART5，地址2 */
#define BOARD2_AXIS_UART5_MOTOR1_CHANNEL    STEPPER_CH4_UART5   /* 刷毛电机使用UART5，地址1 */

/* ========================== 运动参数（重点改这里） ========================== */
#define BOARD2_AXIS_ENABLE_BEFORE_MOVE      1U      /* 运动前是否先发使能：1是，0否 */
#define BOARD2_AXIS_CLOSED_LOOP_ENABLE      1U      /* 1: wait for driver reached feedback before updating accumulated position */
#define BOARD2_AXIS_CLOSED_LOOP_TIMEOUT_MS  12000U  /* Timeout for manual/home driver reached feedback */

/* X轴：UART2双电机反向联动（ID1/ID2） */
#define BOARD2_AXIS_X_ADDR_ID1              1U      /* X轴电机1地址 */
#define BOARD2_AXIS_X_ADDR_ID2              2U      /* X轴电机2地址 */
#define BOARD2_AXIS_X_DIR_FWD               0U      /* X轴驱动器方向码0 */
#define BOARD2_AXIS_X_DIR_REV               1U      /* X轴驱动器方向码1 */
#define BOARD2_AXIS_X_POS_DIR               BOARD2_AXIS_X_DIR_FWD  /* KEY1按下后的方向就是X轴正方向 */
#define BOARD2_AXIS_X_NEG_DIR               BOARD2_OPPOSITE_DIR(BOARD2_AXIS_X_POS_DIR)
#define BOARD2_AXIS_X_KEY_DIR               BOARD2_AXIS_X_POS_DIR
#define BOARD2_AXIS_X_HOME_DIR              BOARD2_AXIS_X_NEG_DIR
#define BOARD2_AXIS_X_SPEED_RPM             800U    /* X轴速度，单位RPM */
#define BOARD2_AXIS_X_ACCEL                 50U     /* X轴加速度，建议0~255 */
#define BOARD2_AXIS_X_STEP_ANGLE_DEG        1350.0f   /* KEY1单次正向移动角度，累计量增加 */

/* Y轴：单电机 */
#define BOARD2_AXIS_Y_ADDR                  1U      /* Y轴电机地址 */
#define BOARD2_AXIS_Y_DIR_FWD               0U      /* Y轴驱动器方向码0 */
#define BOARD2_AXIS_Y_DIR_REV               1U      /* Y轴驱动器方向码1 */
#define BOARD2_AXIS_Y_POS_DIR               BOARD2_AXIS_Y_DIR_REV  /* KEY2按下后的方向就是Y轴正方向 */
#define BOARD2_AXIS_Y_NEG_DIR               BOARD2_OPPOSITE_DIR(BOARD2_AXIS_Y_POS_DIR)
#define BOARD2_AXIS_Y_KEY_DIR               BOARD2_AXIS_Y_POS_DIR
#define BOARD2_AXIS_Y_HOME_DIR              BOARD2_AXIS_Y_NEG_DIR
#define BOARD2_AXIS_Y_SPEED_RPM             1200U    /* Y轴速度，单位RPM */
#define BOARD2_AXIS_Y_ACCEL                 50U      /* Y轴加速度，建议0~255 */
#define BOARD2_AXIS_Y_STEP_ANGLE_DEG        1200.0f   /* KEY2单次正向移动角度，累计量增加 */

/* Z轴：单电机 */
#define BOARD2_AXIS_Z_ADDR                  1U      /* Z轴电机地址 */
#define BOARD2_AXIS_Z_DIR_FWD               0U      /* Z轴驱动器方向码0 */
#define BOARD2_AXIS_Z_DIR_REV               1U      /* Z轴驱动器方向码1 */
#define BOARD2_AXIS_Z_POS_DIR               BOARD2_AXIS_Z_DIR_FWD  /* KEY3按下后的方向就是Z轴正方向 */
#define BOARD2_AXIS_Z_NEG_DIR               BOARD2_OPPOSITE_DIR(BOARD2_AXIS_Z_POS_DIR)
#define BOARD2_AXIS_Z_KEY_DIR               BOARD2_AXIS_Z_POS_DIR
#define BOARD2_AXIS_Z_HOME_DIR              BOARD2_AXIS_Z_NEG_DIR
#define BOARD2_AXIS_Z_SPEED_RPM             1200U   /* Z轴速度，单位RPM */
#define BOARD2_AXIS_Z_ACCEL                 100U     /* Z轴加速度，建议0~255 */
#define BOARD2_AXIS_Z_STEP_ANGLE_DEG        2400.0f /* KEY3单次正向移动角度，累计量增加 */

/* 刷毛电机：网页/K230通道5 -> 主板2 UART5 地址1 */
#define BOARD2_LINK_CHANNEL_UART5_MOTOR1    5U      /* 通道5：UART5地址1刷毛电机 */
#define BOARD2_AXIS_UART5_MOTOR1_ADDR       1U      /* 刷毛电机驱动器地址 */
#define BOARD2_AXIS_UART5_MOTOR1_DIR_FWD    0U      /* 刷毛电机方向码0 */
#define BOARD2_AXIS_UART5_MOTOR1_DIR_REV    1U      /* 刷毛电机方向码1 */
#define BOARD2_AXIS_UART5_MOTOR1_POS_DIR    BOARD2_AXIS_UART5_MOTOR1_DIR_FWD
#define BOARD2_AXIS_UART5_MOTOR1_NEG_DIR    BOARD2_OPPOSITE_DIR(BOARD2_AXIS_UART5_MOTOR1_POS_DIR)
#define BOARD2_AXIS_UART5_MOTOR1_KEY_DIR    BOARD2_AXIS_UART5_MOTOR1_POS_DIR
#define BOARD2_AXIS_UART5_MOTOR1_HOME_DIR   BOARD2_AXIS_UART5_MOTOR1_NEG_DIR
#define BOARD2_AXIS_UART5_MOTOR1_SPEED_RPM  800U    /* 刷毛电机速度，单位RPM */
#define BOARD2_AXIS_UART5_MOTOR1_ACCEL      80U     /* 刷毛电机加速度，建议0~255 */
#define BOARD2_AXIS_UART5_MOTOR1_STEP_ANGLE_DEG 1000.0f /* 刷毛电机单次调试角度 */

#define BOARD2_LINK_CHANNEL_TELESCOPE       4U      /* 通道4：UART5地址2伸缩杆 */
#define BOARD2_AXIS_TELESCOPE_ADDR          2U      /* 原本伸缩杆改为UART5地址2 */
#define BOARD2_AXIS_TELESCOPE_DIR_FWD       0U      /* 串口5驱动器方向码0 */
#define BOARD2_AXIS_TELESCOPE_DIR_REV       1U      /* 串口5驱动器方向码1 */
#define BOARD2_AXIS_TELESCOPE_POS_DIR       BOARD2_AXIS_TELESCOPE_DIR_FWD  /* USART6数字5触发的方向为伸缩杆正方向 */
#define BOARD2_AXIS_TELESCOPE_NEG_DIR       BOARD2_OPPOSITE_DIR(BOARD2_AXIS_TELESCOPE_POS_DIR)
#define BOARD2_AXIS_TELESCOPE_KEY_DIR       BOARD2_AXIS_TELESCOPE_POS_DIR
#define BOARD2_AXIS_TELESCOPE_HOME_DIR      BOARD2_AXIS_TELESCOPE_NEG_DIR
#define BOARD2_AXIS_TELESCOPE_SPEED_RPM     1200U   /* 串口5速度，单位RPM */
#define BOARD2_AXIS_TELESCOPE_ACCEL         100U    /* 串口5加速度，建议0~255 */
#define BOARD2_AXIS_TELESCOPE_STEP_ANGLE_DEG 18000.0f  /* 数字5单次正向移动角度，累计量增加 */

/* ========================== KEY4回零策略（可调） ========================== */
#define BOARD2_HOME_USE_ACCUMULATED_ANGLE   1U      /* 1按累计角度回零，0按固定角度回零 */
#define BOARD2_HOME_SPEED_RPM               1000U   /* 回零速度，单位RPM */
#define BOARD2_HOME_ACCEL                   50U   /* 回零加速度（会被限幅到255） */
#define BOARD2_HOME_FIXED_X_ANGLE_DEG       6000.0f /* 固定回零角度X（仅USE_ACCUMULATED=0生效） */
#define BOARD2_HOME_FIXED_Y_ANGLE_DEG       6000.0f /* 固定回零角度Y（仅USE_ACCUMULATED=0生效） */
#define BOARD2_HOME_FIXED_Z_ANGLE_DEG       6000.0f /* 固定回零角度Z（仅USE_ACCUMULATED=0生效） */
#define BOARD2_HOME_FIXED_UART5_MOTOR1_ANGLE_DEG 60.0f /* 固定回零角度刷毛电机（仅USE_ACCUMULATED=0生效） */
#define BOARD2_HOME_FIXED_TELESCOPE_ANGLE_DEG 60.0f /* 固定回零角度串口5（仅USE_ACCUMULATED=0生效） */

extern volatile uint32_t g_board2_tick_ms;
extern volatile float g_board2_x_accum_deg;
extern volatile float g_board2_y_accum_deg;
extern volatile float g_board2_z_accum_deg;
extern volatile float g_board2_uart5_motor1_accum_deg;
extern volatile float g_board2_telescope_accum_deg;
extern volatile uint32_t g_board2_x_send_count;
extern volatile uint32_t g_board2_y_send_count;
extern volatile uint32_t g_board2_z_send_count;
extern volatile uint32_t g_board2_uart5_motor1_send_count;
extern volatile uint32_t g_board2_telescope_send_count;
extern volatile uint8_t g_board2_last_key_event;
extern volatile uint8_t g_board2_pending_key_event;
extern volatile uint8_t g_board2_adc_io_active_mask;
extern volatile uint8_t g_board2_adc_io_last_active_mask;
extern volatile uint32_t g_board2_adc_io_change_count;

void Control_Init(void);
void Board2_RunPendingKeyEvent(void);
void Board2_MoveTelescopeStep(void);
void Board2_AdcIo_Init(void);
void Board2_AdcIo_Update(void);
uint8_t Board2_AdcIo_Read(uint8_t index);
uint8_t Board2_AdcIo_GetActiveMask(void);

#endif
