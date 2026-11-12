#ifndef __CONTROL_H
#define __CONTROL_H

#include "sys.h"

// 跑垄控制函数声明
void Run_Mound_Task(void);
void Run_Straight_Forward(void);
void Run_Across_Left(void);
void Run_Across_Right(void);
void Run_Encoder_Back(void);
int Limit_Pwm_moter(int pwm, int min_range, int max_range);

// 外部变量声明
extern int car_state;
extern int mound_count;
extern int encoder_distance_target;
extern int encoder_distance_reached;
extern int last_slave_mode;
extern long total_encoder_back_distance_2;  // 编码器B后退距离累计
extern long total_encoder_back_distance_4;  // 编码器D后退距离累计

// 作业触发距离阈值
#define ACTION_TRIGGER_DISTANCE_RIGHT  670  // 右跨垄作业触发距离（mm），distance3小于此值时触发
#define ACTION_TRIGGER_DISTANCE_LEFT   720  // 左跨垄作业触发距离（mm），distance2小于此值时触发

// PWM值定义（直接使用PWM值，不再转换）
#define BaseSpeed_Forward      6000   // 直行前进PWM值
#define BaseSpeed_Back         6000   // 编码器后退最大PWM值

// 编码器后退加减速参数定义（PWM值）
#define ENCODER_BACK_Speed_Min     2500     // 编码器后退初始/结束PWM值
#define ENCODER_BACK_Speed_Max     BaseSpeed_Back  // 编码器后退最大PWM值
#define ENCODER_BACK_Accel_Ratio   0.25f     // 加速阶段占总距离的比例
#define ENCODER_BACK_Decel_Ratio   0.5f     // 减速阶段占总距离的比例

// 第一次右跨垄速度参数
#define CROSS_SPEED_MAX_RIGHT_1        3200    // 第一次右跨垄最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_RIGHT_1     3000    // 第一次右跨垄正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_RIGHT_1   1800    // 第一次右跨垄出垄最小PWM值（降低以减少车身偏移）

// 第二次右跨垄速度参数
#define CROSS_SPEED_MAX_RIGHT_2        3200    // 第二次右跨垄最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_RIGHT_2     3000    // 第二次右跨垄正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_RIGHT_2   1800    // 第二次右跨垄出垄最小PWM值

// 第三次右跨垄速度参数
#define CROSS_SPEED_MAX_RIGHT_3        3200    // 第三次右跨垄最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_RIGHT_3     3000    // 第三次右跨垄正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_RIGHT_3   1800    // 第三次右跨垄出垄最小PWM值

// 第一次左跨垄速度参数 - 电机1、2
#define CROSS_SPEED_MAX_LEFT_1_M12         3000    // 第一次左跨垄电机1、2最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_LEFT_1_M12      3000    // 第一次左跨垄电机1、2正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_LEFT_1_M12    1500    // 第一次左跨垄电机1、2出垄最小PWM值

// 第一次左跨垄速度参数 - 电机3、4
#define CROSS_SPEED_MAX_LEFT_1_M34         3000    // 第一次左跨垄电机3、4最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_LEFT_1_M34      3000    // 第一次左跨垄电机3、4正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_LEFT_1_M34    1600    // 第一次左跨垄电机3、4出垄最小PWM值

// 第二次左跨垄速度参数 - 电机1、2
#define CROSS_SPEED_MAX_LEFT_2_M12         3000    // 第二次左跨垄电机1、2最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_LEFT_2_M12      3000    // 第二次左跨垄电机1、2正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_LEFT_2_M12    1500    // 第二次左跨垄电机1、2出垄最小PWM值

// 第二次左跨垄速度参数 - 电机3、4
#define CROSS_SPEED_MAX_LEFT_2_M34         3000    // 第二次左跨垄电机3、4最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_LEFT_2_M34      3000    // 第二次左跨垄电机3、4正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_LEFT_2_M34    1600    // 第二次左跨垄电机3、4出垄最小PWM值

// 第三次左跨垄速度参数 - 电机1、2
#define CROSS_SPEED_MAX_LEFT_3_M12         3000    // 第三次左跨垄电机1、2最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_LEFT_3_M12      3000    // 第三次左跨垄电机1、2正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_LEFT_3_M12    1500    // 第三次左跨垄电机1、2出垄最小PWM值

// 第三次左跨垄速度参数 - 电机3、4
#define CROSS_SPEED_MAX_LEFT_3_M34         3000    // 第三次左跨垄电机3、4最大PWM值（垄中）
#define CROSS_SPEED_NORMAL_LEFT_3_M34      3000    // 第三次左跨垄电机3、4正常PWM值（垄两侧）
#define CROSS_SPEED_EXIT_MIN_LEFT_3_M34    1600    // 第三次左跨垄电机3、4出垄最小PWM值

// 第一次左跨垄电机控制参数
#define LEFT_CROSS_MOTOR12_FACTOR_1    1.0f    // 第一次左跨垄电机1、2速度因子
#define LEFT_CROSS_MOTOR3_FACTOR_1     1.1f    // 第一次左跨垄电机3速度因子
#define LEFT_CROSS_MOTOR4_FACTOR_1     1.1f    // 第一次左跨垄电机4速度因子

// 第二次左跨垄电机控制参数
#define LEFT_CROSS_MOTOR12_FACTOR_2    1.0f    // 第二次左跨垄电机1、2速度因子
#define LEFT_CROSS_MOTOR3_FACTOR_2     1.1f    // 第二次左跨垄电机3速度因子
#define LEFT_CROSS_MOTOR4_FACTOR_2     1.1f    // 第二次左跨垄电机4速度因子

// 第三次左跨垄电机控制参数
#define LEFT_CROSS_MOTOR12_FACTOR_3    1.0f    // 第三次左跨垄电机1、2速度因子
#define LEFT_CROSS_MOTOR3_FACTOR_3     1.2f    // 第三次左跨垄电机3速度因子
#define LEFT_CROSS_MOTOR4_FACTOR_3     1.1f    // 第三次左跨垄电机4速度因子

// 第一次右跨垄电机控制参数
#define RIGHT_CROSS_MOTOR1_FACTOR_1    1.0f    // 第一次右跨垄电机1速度因子
#define RIGHT_CROSS_MOTOR2_FACTOR_1    1.0f    // 第一次右跨垄电机2速度因子
#define RIGHT_CROSS_MOTOR3_FACTOR_1    1.1f    // 第一次右跨垄电机3速度因子
#define RIGHT_CROSS_MOTOR4_FACTOR_1    1.2f    // 第一次右跨垄电机4速度因子

// 第二次右跨垄电机控制参数
#define RIGHT_CROSS_MOTOR1_FACTOR_2    1.0f    // 第二次右跨垄电机1速度因子
#define RIGHT_CROSS_MOTOR2_FACTOR_2    1.0f    // 第二次右跨垄电机2速度因子
#define RIGHT_CROSS_MOTOR3_FACTOR_2    1.0f    // 第二次右跨垄电机3速度因子
#define RIGHT_CROSS_MOTOR4_FACTOR_2    1.1f    // 第二次右跨垄电机4速度因子

// 第三次右跨垄电机控制参数
#define RIGHT_CROSS_MOTOR1_FACTOR_3    1.0f    // 第三次右跨垄电机1速度因子
#define RIGHT_CROSS_MOTOR2_FACTOR_3    1.0f    // 第三次右跨垄电机2速度因子
#define RIGHT_CROSS_MOTOR3_FACTOR_3    1.0f    // 第三次右跨垄电机3速度因子
#define RIGHT_CROSS_MOTOR4_FACTOR_3    1.1f    // 第三次右跨垄电机4速度因子

// ========== 跨垄刹车阈值宏定义 ==========
// 各次跨垄的舵机刹车距离（单位：mm）
#define TARGET_DISTANCE_RIGHT_1      120     // 第一次右跨垄刹车距离，distance3到达此距离后触发刹车 
#define TARGET_DISTANCE_LEFT_1       160     // 第一次左跨垄刹车距离，distance2到达此距离后触发刹车
#define TARGET_DISTANCE_RIGHT_2      160     // 第二次右跨垄刹车距离，distance3到达此距离后触发刹车
#define TARGET_DISTANCE_LEFT_2       140     // 第二次左跨垄刹车距离，distance2到达此距离后触发刹车
#define TARGET_DISTANCE_RIGHT_3      250     // 第三次右跨垄刹车距离，distance3到达此距离后触发刹车
#define TARGET_DISTANCE_LEFT_3       130     // 第三次左跨垄刹车距离，distance2到达此距离后触发刹车

// 兼容性宏定义（保留用于线性减速结束距离）
#define TARGET_DISTANCE_LEFT         100     // 左跨垄目标距离（兼容性，建议使用具体宏）
#define TARGET_DISTANCE_RIGHT        100     // 右跨垄目标距离（兼容性，建议使用具体宏）

// ========== 线性减速阈值宏定义 ==========
#define LINEAR_DECEL_START_DISTANCE  700     // 线性减速开始距离（mm），distance2/distance3小于此值时开始线性减速
#define LINEAR_DECEL_END_DISTANCE    100     // 线性减速结束距离（mm），distance2/distance3到达此距离后减速完成（与TARGET_DISTANCE相同）
// 注意：线性减速范围从LINEAR_DECEL_START_DISTANCE到LINEAR_DECEL_END_DISTANCE

// 直行前进停止距离
#define STRAIGHT_FORWARD_STOP_DISTANCE 550+15  // 直行前进停止距离（mm），distance4小于此值时停止

// 延时定义（单位：ms，TIM7中断周期为1ms）
// 各阶段刹车延时
#define BRAKE_DELAY_1          100    // 第一次刹车后等待时间（ms）
#define BRAKE_DELAY_2          60    // 第二次刹车（右跨垄1后）等待时间（ms）
#define BRAKE_DELAY_3          60    // 第三次刹车（编码器后退1后）等待时间（ms）
#define BRAKE_DELAY_4          60    // 第四次刹车（左跨垄1后）等待时间（ms）
#define BRAKE_DELAY_5          60    // 第五次刹车（编码器后退2后）等待时间（ms）
#define BRAKE_DELAY_6          60    // 第六次刹车（右跨垄2后）等待时间（ms）
#define BRAKE_DELAY_7          60    // 第七次刹车（编码器后退3后）等待时间（ms）
#define BRAKE_DELAY_8          60    // 第八次刹车（左跨垄2后）等待时间（ms）
#define BRAKE_DELAY_9          60    // 第九次刹车（编码器后退4后）等待时间（ms）
#define BRAKE_DELAY_10         60    // 第十次刹车（左跨垄3后）等待时间（ms）

// 命令发送延时
#define CMD_SEND_DELAY_CYCLES  10     // 命令发送后等待周期数（约100ms）

// 特殊状态延时
#define STOP_WAIT_DELAY_TIME   50    // 停止等待时间（ms）- STATE_STOP_WAIT状态

// 距离阈值定义
#define DISTANCE_FRONT_STOP    20    // 前方停止距离（cm）

// 编码器后退距离定义（单位：编码器计数）
#define ENCODER_BACK_DISTANCE_1    1550-100 +200+200+100	   // 第一次编码器后退距离
#define ENCODER_BACK_DISTANCE_2    1600-100 +200-20+200+100	   // 第二次编码器后退距离
#define ENCODER_BACK_DISTANCE_3    1550-50  +200-20+100+30	  // 第三次编码器后退距离
#define ENCODER_BACK_DISTANCE_4    1650-120 +170	+100+30   // 第四次编码器后退距离
#define ENCODER_BACK_DISTANCE_5    2300    	+240+50+100		// 第五次编码器后退距离

// 状态定义
#define STATE_STRAIGHT         16    // 直行前进
#define STATE_STOP_BRAKE_1     40    // 第一次刹车
#define STATE_ACROSS_RIGHT_1   50    // 第一次右跨垄
#define STATE_ENCODER_BACK_1   51    // 第一次编码器后退
#define STATE_STOP_BRAKE_3     44    // 第三次刹车
#define STATE_ACROSS_LEFT_1    52    // 第一次左跨垄
#define STATE_ENCODER_BACK_2   53    // 第二次编码器后退
#define STATE_STOP_BRAKE_5     48    // 第五次刹车
#define STATE_ACROSS_RIGHT_2   54    // 第二次右跨垄
#define STATE_ENCODER_BACK_3   56    // 第三次编码器后退
#define STATE_STOP_BRAKE_7     57    // 第七次刹车
#define STATE_ACROSS_LEFT_2    58    // 第二次左跨垄
#define STATE_ENCODER_BACK_4   60    // 第四次编码器后退
#define STATE_STOP_BRAKE_9     61    // 第九次刹车
#define STATE_ACROSS_RIGHT_3   62    // 第三次右跨垄
#define STATE_STOP_WAIT        63    // 等待2秒
#define STATE_ACROSS_LEFT_3    64    // 第三次左跨垄
#define STATE_ENCODER_BACK_5   66    // 第五次编码器后退
#define STATE_STOP             4     // 停止

// 从控状态定义
#define SLAVE_MODE_VERTICAL    0     // 垂直状态
#define SLAVE_MODE_PARALLEL    1     // 平行状态（跨垄时）

// 步进电机动作延迟时间定义（参考主控_原本）
#define STEPPER_ENTER_DELAY_TIME  250  // 进入动作延迟时间（单位：ms）- 320-50-20=250ms
#define STEPPER_EXIT_DELAY_TIME    1000  // 退出动作延迟时间（单位：ms）- 900-50-20-30=800ms

// 步进电机动作参数定义
#define STEPPER_MOTOR_ID           1     // 步进电机编号
#define STEPPER_ENTER_DIRECTION    0     // 进入动作方向
#define STEPPER_EXIT_DIRECTION     1     // 退出动作方向
#define STEPPER_MOVE_SPEED         1000  // 步进电机移动速度
#define STEPPER_MOVE_ACCEL         0     // 步进电机加速度
#define STEPPER_MOVE_ANGLE         650.0f // 步进电机移动角度（度）

// 重置函数声明
void Reset_Distance_Grab_State(void);  // 重置距离触发状态（进入新的跨垄状态时调用）

#endif

