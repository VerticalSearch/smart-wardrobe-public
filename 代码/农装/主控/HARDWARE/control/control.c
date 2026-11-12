#include "control.h"
#include "stm32f4xx.h"                  // Device header
#include "oled.h"
#include "delay.h"
#include "timer.h"
#include "motor.h"
#include "stepper.h"                    
#include "pid.h"
#include "key.h"
#include "encoder.h"   
#include "usartdma.h"  
#include "usart2dma.h"
#include "usart3dma.h"
#include "usart4dma.h"  

#ifndef CMD_SET_DIRECTION

#define CMD_SET_DIRECTION  0x06  // 设置运动方向：0=前向, 1=右行, 2=左行, 3=倒退
#endif

int speedA=0,speedB=0,speedC=0,speedD=0;
int16_t angle;
float yaw_init=0;
int16_t yaw_pid_out=0;
int16_t sonic_pid_out=0;
int base_speed=0; 
int flag=0;
__IO int MS10=0,MS20=0;

extern uint16_t distance2; 						// 激光雷达2距离数据
extern uint16_t distance3; 						// 激光雷达3距离数据
extern uint16_t distance4; 						// 激光雷达4距离数据

// 跑垄控制相关变量
int car_state = 0;              				// 当前状态
int mound_count = 0;            				// 已跨过的垄数
int encoder_distance_target = 0; 				// 编码器目标距离
int encoder_distance_reached = 0;				// 编码器距离到达标志
int brake_delay_counter = 0;     				// 刹车延时计数器
int last_slave_mode = -1;        				// 上次从控状态，避免重复发送
int cmd_send_counter = 0;       				// 命令发送后延时计数器
// 编码器后退累计距离变量
long total_encoder_back_distance_2 = 0;  // 编码器B后退距离累计
long total_encoder_back_distance_4 = 0;  // 编码器D后退距离累计

// 编码器上一次的值（用于计算增量）
int enc_1_last = 0, enc_2_last = 0, enc_3_last = 0, enc_4_last = 0;
int encoder_back_started = 0;  // 编码器后退是否已开始
int encoder_back_current_speed = 0;  // 编码器后退当前PWM值（用于加减速）
// 编码器后退初始值（用于精确计算距离）
int enc_2_start = 0, enc_4_start = 0;  // 开始后退时的编码器值
int encoder_back_distance = 0;  // 当前已后退的距离（从初始值开始计算）

// 跨垄夹取动作状态标志（防止重复触发）
int cross_left_grab_triggered = 0;   // 左跨垄夹取动作是否已触发
int cross_right_grab_triggered = 0;  // 右跨垄夹取动作是否已触发

// 步进电机动作延迟计数器
int stepper_enter_delay_counter = 0;  // 进入动作延迟计数器（单位：ms）
int stepper_exit_delay_counter = 0;   // 退出动作延迟计数器（单位：ms）
int distance_left_protect_counter = 0;   // 左跨垄距离触发后的保护时间计数器（单位：ms）
int distance_right_protect_counter = 0;  // 右跨垄距离触发后的保护时间计数器（单位：ms）

// 作业动作组触发标志（基于距离检测）
int right_cross_action_triggered = 0;  // 右跨垄作业动作组触发标志（防止重复触发）
int left_cross_action_triggered = 0;   // 左跨垄作业动作组触发标志（防止重复触发）






void TIM7_IRQHandler(void)       
{ 

	if(TIM_GetITStatus(TIM7,TIM_IT_Update)==SET)
	{	
		MS10++;
		MS20++;
		
		if(MS20>=20)
		{
			OLED_ShowNum (0,0 ,distance2 ,4,12);  // 激光雷达2
			OLED_ShowNum (0,12,distance3 ,4,12);  // 激光雷达3
			OLED_ShowNum (0,24,distance4 ,4,12);  // 激光雷达4
			
			OLED_ShowIntNum(36, 0, (int)enc_speed_1, 4, 12);  // 编码器A速度
			OLED_ShowIntNum(36,12, (int)enc_speed_2, 4, 12);  // 编码器B速度
			OLED_ShowIntNum(36,24, (int)enc_speed_3, 4, 12);  // 编码器C速度
			OLED_ShowIntNum(36,36, (int)enc_speed_4, 4, 12);  // 编码器D速度
			
			// 显示陀螺仪角度值（度*10转换为度，保留1位小数）
			OLED_Show3FNum(0, 48, sensor_data.yaw / 10.0f, 3, 1, 12);  // 陀螺仪角度（度）
			
			OLED_Refresh_Gram();

			MS20=0;
		}

		
		if(MS10>=10)  
		{
			// 更新编码器累计位置（用于后退距离计算）
			Encoder_Speed_Calculate(); 
			
			MS10=0; 
		}
		
		// 更新进入动作延迟计数器
		if(stepper_enter_delay_counter > 0) {
			stepper_enter_delay_counter--;  // 每1ms减1
			// 延迟时间到达，执行步进电机进入动作
			if(stepper_enter_delay_counter == 0) {
				stepper_move_angle(STEPPER_MOTOR_ID, STEPPER_ENTER_DIRECTION, STEPPER_MOVE_SPEED, STEPPER_MOVE_ACCEL, STEPPER_MOVE_ANGLE);  // 进入动作
			}
		}
		
		// 更新退出动作延迟计数器
		if(stepper_exit_delay_counter > 0) {
			stepper_exit_delay_counter--;  // 每1ms减1
			// 延迟时间到达，执行步进电机退出动作
			if(stepper_exit_delay_counter == 0) {
				stepper_move_angle(STEPPER_MOTOR_ID, STEPPER_EXIT_DIRECTION, STEPPER_MOVE_SPEED, STEPPER_MOVE_ACCEL, STEPPER_MOVE_ANGLE);  // 退出动作
			}
		}
		
		// 更新左跨垄距离触发后的保护时间计数器
		if(distance_left_protect_counter > 0) {
			distance_left_protect_counter--;  // 每1ms减1
		}
		
		// 更新右跨垄距离触发后的保护时间计数器
		if(distance_right_protect_counter > 0) {
			distance_right_protect_counter--;  // 每1ms减1
		}
		
				
		if(flag==1)
		{
			
		}	
		if(flag==2)
		{
			
		}		
		if(flag==3)
		{		
			
		}		
		if(flag==4)
		{
			
		}		
		if(flag==5)
		{
			
		}	


		
		// 
		if(car_state == STATE_STRAIGHT || car_state == STATE_STOP_BRAKE_1 || 
		   car_state == STATE_ACROSS_RIGHT_1 || 
		   car_state == STATE_ENCODER_BACK_1 || car_state == STATE_STOP_BRAKE_3 || 
		   car_state == STATE_ACROSS_LEFT_1 || 
		   car_state == STATE_ENCODER_BACK_2 || car_state == STATE_STOP_BRAKE_5 || 
		   car_state == STATE_ACROSS_RIGHT_2 || 
		   car_state == STATE_ENCODER_BACK_3 || car_state == STATE_STOP_BRAKE_7 || 
		   car_state == STATE_ACROSS_LEFT_2 || 
		   car_state == STATE_ENCODER_BACK_4 || car_state == STATE_STOP_BRAKE_9 || 
		   car_state == STATE_ACROSS_RIGHT_3 || 
		   car_state == STATE_STOP_WAIT || car_state == STATE_ACROSS_LEFT_3 || 
		   car_state == STATE_ENCODER_BACK_5 || 
		   car_state == STATE_STOP)
		{
			Run_Mound_Task();
		}
			
			TIM_ClearITPendingBit(TIM7,TIM_IT_Update);  
		
	}
	
}

// 跑垄控制主函数
void Run_Mound_Task(void)
{
    // distance2: 左侧 
    // distance3: 右侧 
    // distance4: 前方
    
    switch(car_state) {
        case STATE_STRAIGHT:  // 直行前进（参考状态16）
            // 首次进入时设置前向方向
            if(last_slave_mode == -1) {
                Master_SendCommand(CMD_SET_DIRECTION, 0, 0);  // 设置方向为前向
                last_slave_mode = SLAVE_MODE_VERTICAL;
            }
            
            Run_Straight_Forward();
            
            // 直行直到distance4 < 停止距离
            if(distance4 < STRAIGHT_FORWARD_STOP_DISTANCE) {
                car_state = STATE_STOP_BRAKE_1;  // 刹车
            }
            break;
            
        case STATE_STOP_BRAKE_1:  // 第一次刹车
            MotorA_Ctrl(-1);
            MotorB_Ctrl(-1);
            MotorC_Ctrl(-1);
            MotorD_Ctrl(-1);
            
            // 计时延时300ms
            brake_delay_counter++;
            if(brake_delay_counter >= BRAKE_DELAY_1) {  // 第一次刹车延迟
                brake_delay_counter = 0;
                
                // 切换舵机为右行状态（只发送一次）
                if(last_slave_mode != SLAVE_MODE_PARALLEL) {
                    Master_SendCommand(CMD_SET_DIRECTION, 1, 0);  // 设置方向为右行
                    last_slave_mode = SLAVE_MODE_PARALLEL;
                    cmd_send_counter = CMD_SEND_DELAY_CYCLES;  // 等待命令发送完成
                }
                
                // 等待命令发送完成
                if(cmd_send_counter > 0) {
                    cmd_send_counter--;
                } else {
                    // 进入第一次右跨垄
                    car_state = STATE_ACROSS_RIGHT_1;
                }
            }
            break;
            
        // ========== 第一阶段：第一次右跨垄 ==========
        case STATE_ACROSS_RIGHT_1:  // 第一次右跨垄
        {
            // 首次进入时重置距离触发状态
            static int right1_init = 0;
            static int right1_brake_started = 0;  // 刹车和舵机旋转是否已开始
            
            if(right1_init == 0) {
                Reset_Distance_Grab_State();
                right1_init = 1;
                right1_brake_started = 0;
            }
            
            // 优先级最高：检查是否达到目标距离，触发刹车和舵机旋转
            if(distance3 <= TARGET_DISTANCE_RIGHT_1) {
                if(right1_brake_started == 0) {
                    // 立即停止电机（刹车）
                    MotorA_Ctrl(0);
                    MotorB_Ctrl(0);
                    MotorC_Ctrl(0);
                    MotorD_Ctrl(0);
                    // 发送舵机旋转命令到垂直状态（完成刹车）
                    if(last_slave_mode != SLAVE_MODE_VERTICAL) {
                        Master_SendCommand(CMD_SET_DIRECTION, 3, 0);  // 设置方向为倒退（编码器后退）
                        last_slave_mode = SLAVE_MODE_VERTICAL;
                        cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                    }
                    right1_brake_started = 1;
                    brake_delay_counter = 0;
                }
                
                // 等待舵机旋转完成（优先级最高，不执行跨垄控制）
                if(right1_brake_started == 1) {
                    if(cmd_send_counter > 0) {
                        cmd_send_counter--;
                    } else {
                        brake_delay_counter++;
                        if(brake_delay_counter >= BRAKE_DELAY_2) {  // 等待舵机旋转完成
                            brake_delay_counter = 0;
                            right1_init = 0;
                            right1_brake_started = 0;
                            extern int enc_1, enc_2, enc_3, enc_4;
                            enc_1 = 0;
                            enc_2 = 0;
                            enc_3 = 0;
                            enc_4 = 0;
                            encoder_distance_reached = 0;
                            encoder_distance_target = ENCODER_BACK_DISTANCE_1;
                            car_state = STATE_ENCODER_BACK_1;  // 第一次编码器后退
                        }
                    }
                }
            } else if(right1_brake_started == 0) {
                // 未达到目标距离且未开始舵机旋转，继续运行跨垄控制
                Run_Across_Right();
            }
            // 如果right1_brake_started == 1但距离未达到，说明舵机正在旋转，不执行跨垄控制
        }
            break;
            
        case STATE_ENCODER_BACK_1:  // 第一次编码器后退
            Run_Encoder_Back();
            if(encoder_distance_reached) {
                car_state = STATE_STOP_BRAKE_3;
            }
            break;
            
        // ========== 第二阶段：第一次左跨垄 ==========
        case STATE_STOP_BRAKE_3:  // 第三次刹车
            MotorA_Ctrl(-1);
            MotorB_Ctrl(-1);
            MotorC_Ctrl(-1);
            MotorD_Ctrl(-1);
            brake_delay_counter++;
            if(brake_delay_counter >= BRAKE_DELAY_3) {  // 第三次刹车延迟
                brake_delay_counter = 0;
                if(last_slave_mode != SLAVE_MODE_PARALLEL) {
                    Master_SendCommand(CMD_SET_DIRECTION, 2, 0);  // 设置方向为左行，param2=0表示第一次跨左垄
                    last_slave_mode = SLAVE_MODE_PARALLEL;
                    cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                }
                if(cmd_send_counter > 0) {
                    cmd_send_counter--;
                } else {
                    // 进入第一次左跨垄
                    car_state = STATE_ACROSS_LEFT_1;  // 第一次左跨垄
                }
            }
            break;
            
        case STATE_ACROSS_LEFT_1:  // 第一次左跨垄
        {
            // 首次进入时重置距离触发状态
            static int left1_init = 0;
            static int left1_brake_started = 0;  // 刹车和舵机旋转是否已开始
            
            if(left1_init == 0) {
                Reset_Distance_Grab_State();
                left1_init = 1;
                left1_brake_started = 0;
            }
            
            // 优先级最高：检查是否达到目标距离，触发刹车和舵机旋转
            if(distance2 <= TARGET_DISTANCE_LEFT_1) {
                if(left1_brake_started == 0) {
                    // 立即停止电机（刹车）
                    MotorA_Ctrl(0);
                    MotorB_Ctrl(0);
                    MotorC_Ctrl(0);
                    MotorD_Ctrl(0);
                    // 发送舵机旋转命令到垂直状态（完成刹车）
                    if(last_slave_mode != SLAVE_MODE_VERTICAL) {
                        Master_SendCommand(CMD_SET_DIRECTION, 3, 0);  // 设置方向为倒退（编码器后退）
                        last_slave_mode = SLAVE_MODE_VERTICAL;
                        cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                    }
                    left1_brake_started = 1;
                    brake_delay_counter = 0;
                }
                
                // 等待舵机旋转完成（优先级最高，不执行跨垄控制）
                if(left1_brake_started == 1) {
                    if(cmd_send_counter > 0) {
                        cmd_send_counter--;
                    } else {
                        brake_delay_counter++;
                        if(brake_delay_counter >= BRAKE_DELAY_4) {  // 等待舵机旋转完成
                            brake_delay_counter = 0;
                            left1_init = 0;
                            left1_brake_started = 0;
                            extern int enc_1, enc_2, enc_3, enc_4;
                            enc_1 = 0;
                            enc_2 = 0;
                            enc_3 = 0;
                            enc_4 = 0;
                            encoder_distance_reached = 0;
                            encoder_distance_target = ENCODER_BACK_DISTANCE_2;
                            car_state = STATE_ENCODER_BACK_2;  // 第二次编码器后退
                        }
                    }
                }
            } else if(left1_brake_started == 0) {
                // 未达到目标距离且未开始舵机旋转，继续运行跨垄控制
                Run_Across_Left();
            }
            // 如果left1_brake_started == 1但距离未达到，说明舵机正在旋转，不执行跨垄控制
        }
            break;
            
        case STATE_ENCODER_BACK_2:  // 第二次编码器后退
            Run_Encoder_Back();
            if(encoder_distance_reached) {
                car_state = STATE_STOP_BRAKE_5;
            }
            break;
            
        // ========== 第三阶段：第二次右跨垄 ==========
        case STATE_STOP_BRAKE_5:  // 第五次刹车
            MotorA_Ctrl(-1);
            MotorB_Ctrl(-1);
            MotorC_Ctrl(-1);
            MotorD_Ctrl(-1);
            brake_delay_counter++;
            if(brake_delay_counter >= BRAKE_DELAY_5) {  // 第五次刹车延迟
                brake_delay_counter = 0;
                if(last_slave_mode != SLAVE_MODE_PARALLEL) {
                    Master_SendCommand(CMD_SET_DIRECTION, 1, 0);  // 设置方向为右行
                    last_slave_mode = SLAVE_MODE_PARALLEL;
                    cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                }
                if(cmd_send_counter > 0) {
                    cmd_send_counter--;
                } else {
                    // 进入第二次右跨垄
                    car_state = STATE_ACROSS_RIGHT_2;  // 第二次右跨垄
                }
            }
            break;
            
        case STATE_ACROSS_RIGHT_2:  // 第二次右跨垄
        {
            // 首次进入时重置距离触发状态
            static int right2_init = 0;
            static int right2_brake_started = 0;  // 刹车和舵机旋转是否已开始
            
            if(right2_init == 0) {
                Reset_Distance_Grab_State();
                right2_init = 1;
                right2_brake_started = 0;
            }
            
            // 优先级最高：检查是否达到目标距离，触发刹车和舵机旋转
            if(distance3 <= TARGET_DISTANCE_RIGHT_2) {
                if(right2_brake_started == 0) {
                    // 立即停止电机（刹车）
                    MotorA_Ctrl(0);
                    MotorB_Ctrl(0);
                    MotorC_Ctrl(0);
                    MotorD_Ctrl(0);
                    // 发送舵机旋转命令到垂直状态（完成刹车）
                    if(last_slave_mode != SLAVE_MODE_VERTICAL) {
                        Master_SendCommand(CMD_SET_DIRECTION, 3, 0);  // 设置方向为倒退（编码器后退）
                        last_slave_mode = SLAVE_MODE_VERTICAL;
                        cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                    }
                    right2_brake_started = 1;
                    brake_delay_counter = 0;
                }
                
                // 等待舵机旋转完成（优先级最高，不执行跨垄控制）
                if(right2_brake_started == 1) {
                    if(cmd_send_counter > 0) {
                        cmd_send_counter--;
                    } else {
                        brake_delay_counter++;
                        if(brake_delay_counter >= BRAKE_DELAY_6) {  // 等待舵机旋转完成
                            brake_delay_counter = 0;
                            right2_init = 0;
                            right2_brake_started = 0;
                            extern int enc_1, enc_2, enc_3, enc_4;
                            enc_1 = 0;
                            enc_2 = 0;
                            enc_3 = 0;
                            enc_4 = 0;
                            encoder_distance_reached = 0;
                            encoder_distance_target = ENCODER_BACK_DISTANCE_3;
                            car_state = STATE_ENCODER_BACK_3;  // 第三次编码器后退
                        }
                    }
                }
            } else if(right2_brake_started == 0) {
                // 未达到目标距离且未开始舵机旋转，继续运行跨垄控制
                Run_Across_Right();
            }
            // 如果right2_brake_started == 1但距离未达到，说明舵机正在旋转，不执行跨垄控制
        }
            break;
            
        case STATE_ENCODER_BACK_3:  // 第三次编码器后退
            Run_Encoder_Back();
            if(encoder_distance_reached) {
                car_state = STATE_STOP_BRAKE_7;
            }
            break;
            
        // ========== 第四阶段：第二次左跨垄（最后一次） ==========
        case STATE_STOP_BRAKE_7:  // 第七次刹车
            MotorA_Ctrl(-1);
            MotorB_Ctrl(-1);
            MotorC_Ctrl(-1);
            MotorD_Ctrl(-1);
            brake_delay_counter++;
            if(brake_delay_counter >= BRAKE_DELAY_7) {  // 第七次刹车延迟
                brake_delay_counter = 0;
                if(last_slave_mode != SLAVE_MODE_PARALLEL) {
                    Master_SendCommand(CMD_SET_DIRECTION, 2, 2);  // 设置方向为左行，param2=2表示第二次跨左垄
                    last_slave_mode = SLAVE_MODE_PARALLEL;
                    cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                }
                if(cmd_send_counter > 0) {
                    cmd_send_counter--;
                } else {
                    // 进入第二次左跨垄
                    car_state = STATE_ACROSS_LEFT_2;  // 第二次左跨垄
                }
            }
            break;
            
        case STATE_ACROSS_LEFT_2:  // 第二次左跨垄
        {
            // 首次进入时重置距离触发状态
            static int left2_init = 0;
            static int left2_brake_started = 0;  // 刹车和舵机旋转是否已开始
            
            if(left2_init == 0) {
                Reset_Distance_Grab_State();
                left2_init = 1;
                left2_brake_started = 0;
            }
            
            // 优先级最高：检查是否达到目标距离，触发刹车和舵机旋转
            if(distance2 <= TARGET_DISTANCE_LEFT_2) {
                if(left2_brake_started == 0) {
                    // 立即停止电机（刹车）
                    MotorA_Ctrl(0);
                    MotorB_Ctrl(0);
                    MotorC_Ctrl(0);
                    MotorD_Ctrl(0);
                    // 发送舵机旋转命令到垂直状态（完成刹车）
                    if(last_slave_mode != SLAVE_MODE_VERTICAL) {
                        Master_SendCommand(CMD_SET_DIRECTION, 3, 0);  // 设置方向为倒退
                        last_slave_mode = SLAVE_MODE_VERTICAL;
                        cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                    }
                    left2_brake_started = 1;
                    brake_delay_counter = 0;
                }
                
                // 等待舵机旋转完成（优先级最高，不执行跨垄控制）
                if(left2_brake_started == 1) {
                    if(cmd_send_counter > 0) {
                        cmd_send_counter--;
                    } else {
                        brake_delay_counter++;
                        if(brake_delay_counter >= BRAKE_DELAY_8) {  // 等待舵机旋转完成
                            brake_delay_counter = 0;
                            left2_init = 0;
                            left2_brake_started = 0;
                            extern int enc_1, enc_2, enc_3, enc_4;
                            enc_1 = 0;
                            enc_2 = 0;
                            enc_3 = 0;
                            enc_4 = 0;
                            encoder_distance_reached = 0;
                            encoder_distance_target = ENCODER_BACK_DISTANCE_4;
                            car_state = STATE_ENCODER_BACK_4;  // 第四次编码器后退
                        }
                    }
                }
            } else if(left2_brake_started == 0) {
                // 未达到目标距离且未开始舵机旋转，继续运行跨垄控制
                Run_Across_Left();
            }
            // 如果left2_brake_started == 1但距离未达到，说明舵机正在旋转，不执行跨垄控制
        }
            break;
            
        // ========== 第五阶段：第四次编码器后退 ==========
        case STATE_ENCODER_BACK_4:  // 第四次编码器后退
            Run_Encoder_Back();
            if(encoder_distance_reached) {
                car_state = STATE_STOP_BRAKE_9;
            }
            break;
            
        case STATE_STOP_BRAKE_9:  // 第九次刹车
            MotorA_Ctrl(-1);
            MotorB_Ctrl(-1);
            MotorC_Ctrl(-1);
            MotorD_Ctrl(-1);
            brake_delay_counter++;
            if(brake_delay_counter >= BRAKE_DELAY_9) {  // 第九次刹车延迟
                brake_delay_counter = 0;
                if(last_slave_mode != SLAVE_MODE_PARALLEL) {
                    Master_SendCommand(CMD_SET_DIRECTION, 1, 0);  // 设置方向为右行
                    last_slave_mode = SLAVE_MODE_PARALLEL;
                    cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                }
                if(cmd_send_counter > 0) {
                    cmd_send_counter--;
                } else {
                    // 进入第三次右跨垄
                    car_state = STATE_ACROSS_RIGHT_3;  // 第三次右跨垄
                }
            }
            break;
            
        case STATE_ACROSS_RIGHT_3:  // 第三次右跨垄
        {
            // 首次进入时重置距离触发状态
            static int right3_init = 0;
            static int right3_brake_started = 0;  // 刹车和舵机旋转是否已开始
            
            if(right3_init == 0) {
                Reset_Distance_Grab_State();
                right3_init = 1;
                right3_brake_started = 0;
            }
            
            // 优先级最高：检查是否达到目标距离，触发刹车和舵机旋转
            if(distance3 <= TARGET_DISTANCE_RIGHT_3) {
                if(right3_brake_started == 0) {
                    // 立即停止电机（刹车）
                    MotorA_Ctrl(0);
                    MotorB_Ctrl(0);
                    MotorC_Ctrl(0);
                    MotorD_Ctrl(0);
                    // 最后一次右跨垄：触发步进电机退出动作（移动回来）
                    // 直接设置退出计数器，确保退出动作能够执行
                    // 如果作业动作组已经设置了，重新设置可以确保退出动作执行（即使之前已经执行过）
                    stepper_exit_delay_counter = STEPPER_EXIT_DELAY_TIME;
                    // 直接发送左行命令（从右行状态转到左行状态，不经过垂直状态）
                    // 注意：右行和左行都是SLAVE_MODE_PARALLEL，所以需要直接发送命令
                    Master_SendCommand(CMD_SET_DIRECTION, 2, 1);  // 设置方向为左行，param2=1表示第三次跨左垄
                    last_slave_mode = SLAVE_MODE_PARALLEL;
                        cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                    right3_brake_started = 1;
                    brake_delay_counter = 0;
                }
                
                // 等待舵机旋转完成（优先级最高，不执行跨垄控制）
                if(right3_brake_started == 1) {
                    if(cmd_send_counter > 0) {
                        cmd_send_counter--;
                    } else {
                        brake_delay_counter++;
                        if(brake_delay_counter >= STOP_WAIT_DELAY_TIME) {  // 等待舵机旋转完成
                            brake_delay_counter = 0;
                            right3_init = 0;
                            right3_brake_started = 0;
                            car_state = STATE_ACROSS_LEFT_3;  // 直接进入第三次左跨垄
                        }
                    }
                }
            } else if(right3_brake_started == 0) {
                // 未达到目标距离且未开始舵机旋转，继续运行跨垄控制
                Run_Across_Right();
            }
            // 如果right3_brake_started == 1但距离未达到，说明舵机正在旋转，不执行跨垄控制
        }
            break;
            
        case STATE_STOP_WAIT:  // 等待并设置左行方向（已废弃，保留用于兼容）
        {
            // 确保速度为0
            MotorA_Ctrl(0);
            MotorB_Ctrl(0);
            MotorC_Ctrl(0);
            MotorD_Ctrl(0);
            
            // 首次进入时发送左行命令（从垂直状态转到左行状态）
            static int stop_wait_init = 0;
            if(stop_wait_init == 0) {
                Master_SendCommand(CMD_SET_DIRECTION, 2, 1);  // 设置方向为左行，param2=1表示第三次跨左垄
                last_slave_mode = SLAVE_MODE_PARALLEL;
                cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                brake_delay_counter = 0;
                stop_wait_init = 1;
            }
            
            // 等待命令发送完成
            if(cmd_send_counter > 0) {
                cmd_send_counter--;
            } else {
                // 命令发送完成后，等待STOP_WAIT_DELAY_TIME再进入跨左垄状态
                brake_delay_counter++;
                if(brake_delay_counter >= STOP_WAIT_DELAY_TIME) {  // 等待时间
                    brake_delay_counter = 0;
                    stop_wait_init = 0;  // 重置标志，准备下次使用
                    // 进入第三次左跨垄（第六条垄道，不进行夹取动作）
                    car_state = STATE_ACROSS_LEFT_3;  // 第三次左跨垄
                }
            }
        }
            break;
            
        case STATE_ACROSS_LEFT_3:  // 第三次左跨垄（最后一次左跨垄，第六条垄道，不进行夹取动作）
        {
            // 首次进入时重置距离触发状态
            static int left3_init = 0;
            static int left3_brake_started = 0;  // 刹车和舵机旋转是否已开始
            
            if(left3_init == 0) {
                // 如果步进电机退出计数器正在倒计时（从第三次右跨垄设置的），不要重置它
                // 先保存退出计数器的值
                int saved_exit_counter = stepper_exit_delay_counter;
                // 调用重置函数（会重置退出计数器）
                Reset_Distance_Grab_State();
                // 恢复退出计数器（如果它还在倒计时）
                if(saved_exit_counter > 0) {
                    stepper_exit_delay_counter = saved_exit_counter;
                }
                left3_init = 1;
                left3_brake_started = 0;
            }
            
            // 第六条垄道不进行夹取动作，直接跳过
            // 不检测distance2 < CROSS_SPEED_CONTROL_THRESHOLD，不触发步进运动和舵机1、2动作
            
            // 优先级最高：检查是否达到目标距离，触发刹车和舵机旋转
            if(distance2 <= TARGET_DISTANCE_LEFT_3) {
                if(left3_brake_started == 0) {
                    // 立即停止电机（刹车）
                    MotorA_Ctrl(0);
                    MotorB_Ctrl(0);
                    MotorC_Ctrl(0);
                    MotorD_Ctrl(0);
                    // 发送舵机旋转命令到垂直状态（完成刹车）
                    if(last_slave_mode != SLAVE_MODE_VERTICAL) {
                        Master_SendCommand(CMD_SET_DIRECTION, 3, 5);  // 设置方向为倒退，param2=5表示第五次后退（特殊标识）
                        last_slave_mode = SLAVE_MODE_VERTICAL;
                        cmd_send_counter = CMD_SEND_DELAY_CYCLES;
                    }
                    left3_brake_started = 1;
                    brake_delay_counter = 0;
                }
                
                // 等待舵机旋转完成（优先级最高，不执行跨垄控制）
                if(left3_brake_started == 1) {
                    if(cmd_send_counter > 0) {
                        cmd_send_counter--;
                    } else {
                        brake_delay_counter++;
                        if(brake_delay_counter >= BRAKE_DELAY_10) {  // 等待舵机旋转完成
                            brake_delay_counter = 0;
                            left3_init = 0;
                            left3_brake_started = 0;
                            extern int enc_1, enc_2, enc_3, enc_4;
                            enc_1 = 0;
                            enc_2 = 0;
                            enc_3 = 0;
                            enc_4 = 0;
                            encoder_distance_reached = 0;
                            encoder_distance_target = ENCODER_BACK_DISTANCE_5;
                            car_state = STATE_ENCODER_BACK_5;  // 第五次编码器后退
                        }
                    }
                }
            } else if(left3_brake_started == 0) {
                // 未达到目标距离且未开始舵机旋转，继续运行跨垄控制
                Run_Across_Left();
            }
            // 如果left3_brake_started == 1但距离未达到，说明舵机正在旋转，不执行跨垄控制
        }
            break;
            
        case STATE_ENCODER_BACK_5:  // 第五次编码器后退
            Run_Encoder_Back();
            if(encoder_distance_reached) {
                car_state = STATE_STOP;  // 最终停止
            }
            break;
      
            
        case STATE_STOP:  // 最终停止
            MotorA_Ctrl(0);
            MotorB_Ctrl(0);
            MotorC_Ctrl(0);
            MotorD_Ctrl(0);
            break;
    }
    
    // 安全检查：前方距离过近，停止
    if(distance4 < DISTANCE_FRONT_STOP) {
        car_state = STATE_STOP;
    }
}

// 重置距离触发状态（进入新的跨垄状态时调用）
void Reset_Distance_Grab_State(void)
{
    stepper_enter_delay_counter = 0;  // 重置进入动作延迟计数器
    stepper_exit_delay_counter = 0;   // 重置退出动作延迟计数器
    distance_left_protect_counter = 0;   // 重置左跨垄保护时间计数器
    distance_right_protect_counter = 0;  // 重置右跨垄保护时间计数器
    cross_left_grab_triggered = 0;   // 重置左跨垄触发标志
    cross_right_grab_triggered = 0;  // 重置右跨垄触发标志
    right_cross_action_triggered = 0;  // 重置右跨垄作业动作组触发标志
    left_cross_action_triggered = 0;   // 重置左跨垄作业动作组触发标志
}

// 直行前进控制（参考5.7状态16）
void Run_Straight_Forward(void)
{
    // 直接设置PWM值控制电机前进
    MotorA_Ctrl(BaseSpeed_Forward);   // 电机A
    MotorB_Ctrl(BaseSpeed_Forward);   // 电机B
    MotorC_Ctrl(BaseSpeed_Forward);   // 电机C
    MotorD_Ctrl(BaseSpeed_Forward);   // 电机D
}


// 左转向跨垄控制
void Run_Across_Left(void)
{
    // 根据当前状态选择对应的刹车距离阈值
    int target_distance;
    if(car_state == STATE_ACROSS_LEFT_1) {
        target_distance = TARGET_DISTANCE_LEFT_1;
    } else if(car_state == STATE_ACROSS_LEFT_2) {
        target_distance = TARGET_DISTANCE_LEFT_2;
    } else if(car_state == STATE_ACROSS_LEFT_3) {
        target_distance = TARGET_DISTANCE_LEFT_3;
    } else {
        target_distance = TARGET_DISTANCE_LEFT_1;  // 默认使用第一次左跨垄的距离
    }
    
    // 检查是否到达刹车阈值，如果到达则立即停止电机（由主状态机处理刹车和舵机转动）
    if(distance2 <= target_distance) {
        MotorA_Ctrl(0);
        MotorB_Ctrl(0);
        MotorC_Ctrl(0);
        MotorD_Ctrl(0);
        return;  // 直接返回，不执行后续的PWM设置
    }
    
    int target_pwm;
    int pwm_1, pwm_2, pwm_3, pwm_4;
    
    // 检测distance2小于600时触发作业动作组（左跨垄，但第三条左垄除外）
    if(distance2 < ACTION_TRIGGER_DISTANCE_LEFT && !left_cross_action_triggered) {
        // 第三条左垄不触发作业动作组
        if(car_state != STATE_ACROSS_LEFT_3) {
            // distance2小于600，触发步进和从控舵机1234的动作组
            stepper_enter_delay_counter = STEPPER_ENTER_DELAY_TIME;
            stepper_exit_delay_counter = STEPPER_EXIT_DELAY_TIME;
            Master_SendCommand(CMD_STATE_SWITCH_1, 0, 0);  // 发送舵机1234动作组指令
            left_cross_action_triggered = 1;  // 标记已触发，防止重复触发
        } else {
            // 第三条左垄：标记已触发但不执行动作组，防止重复检测
            left_cross_action_triggered = 1;
        }
    }
    
    // 根据当前状态选择对应的速度参数 - 电机1、2
    int speed_max_m12, speed_normal_m12, speed_exit_min_m12;
    // 根据当前状态选择对应的速度参数 - 电机3、4
    int speed_max_m34, speed_normal_m34, speed_exit_min_m34;
    
    if(car_state == STATE_ACROSS_LEFT_1) {
        speed_max_m12 = CROSS_SPEED_MAX_LEFT_1_M12;
        speed_normal_m12 = CROSS_SPEED_NORMAL_LEFT_1_M12;
        speed_exit_min_m12 = CROSS_SPEED_EXIT_MIN_LEFT_1_M12;
        speed_max_m34 = CROSS_SPEED_MAX_LEFT_1_M34;
        speed_normal_m34 = CROSS_SPEED_NORMAL_LEFT_1_M34;
        speed_exit_min_m34 = CROSS_SPEED_EXIT_MIN_LEFT_1_M34;
    } else if(car_state == STATE_ACROSS_LEFT_2) {
        speed_max_m12 = CROSS_SPEED_MAX_LEFT_2_M12;
        speed_normal_m12 = CROSS_SPEED_NORMAL_LEFT_2_M12;
        speed_exit_min_m12 = CROSS_SPEED_EXIT_MIN_LEFT_2_M12;
        speed_max_m34 = CROSS_SPEED_MAX_LEFT_2_M34;
        speed_normal_m34 = CROSS_SPEED_NORMAL_LEFT_2_M34;
        speed_exit_min_m34 = CROSS_SPEED_EXIT_MIN_LEFT_2_M34;
    } else if(car_state == STATE_ACROSS_LEFT_3) {
        speed_max_m12 = CROSS_SPEED_MAX_LEFT_3_M12;
        speed_normal_m12 = CROSS_SPEED_NORMAL_LEFT_3_M12;
        speed_exit_min_m12 = CROSS_SPEED_EXIT_MIN_LEFT_3_M12;
        speed_max_m34 = CROSS_SPEED_MAX_LEFT_3_M34;
        speed_normal_m34 = CROSS_SPEED_NORMAL_LEFT_3_M34;
        speed_exit_min_m34 = CROSS_SPEED_EXIT_MIN_LEFT_3_M34;
    } else {
        speed_max_m12 = CROSS_SPEED_MAX_LEFT_1_M12;  // 默认使用第一次左跨垄的速度
        speed_normal_m12 = CROSS_SPEED_NORMAL_LEFT_1_M12;
        speed_exit_min_m12 = CROSS_SPEED_EXIT_MIN_LEFT_1_M12;
        speed_max_m34 = CROSS_SPEED_MAX_LEFT_1_M34;
        speed_normal_m34 = CROSS_SPEED_NORMAL_LEFT_1_M34;
        speed_exit_min_m34 = CROSS_SPEED_EXIT_MIN_LEFT_1_M34;
    }
    
    // PWM控制逻辑 - 电机1、2（跨左垄：直接使用最大速度，到达700mm时开始线性衰减）
    // 1. 当distance2 < LINEAR_DECEL_START_DISTANCE时：使用出垄控制（线性减速）
    int target_pwm_m12;
    if(distance2 < LINEAR_DECEL_START_DISTANCE) {
        // 出垄控制：根据distance2线性衰减PWM（LINEAR_DECEL_START_DISTANCE时speed_max_m12，LINEAR_DECEL_END_DISTANCE时speed_exit_min_m12）
        float distance_range = (float)LINEAR_DECEL_START_DISTANCE - LINEAR_DECEL_END_DISTANCE;
        float current_range = distance2 - LINEAR_DECEL_END_DISTANCE;
        if(distance_range > 0 && current_range > 0) {
            float speed_ratio = current_range / distance_range;
            if(speed_ratio > 1.0f) speed_ratio = 1.0f;
            if(speed_ratio < 0.0f) speed_ratio = 0.0f;
            target_pwm_m12 = speed_exit_min_m12 + (speed_max_m12 - speed_exit_min_m12) * speed_ratio;
        } else {
            target_pwm_m12 = speed_exit_min_m12;
        }
    }
    // 2. 当distance2 >= 700时：使用最大速度
    else {
        target_pwm_m12 = speed_max_m12;
    }
    
    // PWM控制逻辑 - 电机3、4（跨左垄：直接使用最大速度，到达700mm时开始线性衰减）
    // 1. 当distance2 < LINEAR_DECEL_START_DISTANCE时：使用出垄控制（线性减速）
    int target_pwm_m34;
    if(distance2 < LINEAR_DECEL_START_DISTANCE) {
        // 出垄控制：根据distance2线性衰减PWM（LINEAR_DECEL_START_DISTANCE时speed_max_m34，LINEAR_DECEL_END_DISTANCE时speed_exit_min_m34）
        float distance_range = (float)LINEAR_DECEL_START_DISTANCE - LINEAR_DECEL_END_DISTANCE;
        float current_range = distance2 - LINEAR_DECEL_END_DISTANCE;
        if(distance_range > 0 && current_range > 0) {
            float speed_ratio = current_range / distance_range;
            if(speed_ratio > 1.0f) speed_ratio = 1.0f;
            if(speed_ratio < 0.0f) speed_ratio = 0.0f;
            target_pwm_m34 = speed_exit_min_m34 + (speed_max_m34 - speed_exit_min_m34) * speed_ratio;
        } else {
            target_pwm_m34 = speed_exit_min_m34;
        }
    }
    // 2. 当distance2 >= 700时：使用最大速度
    else {
        target_pwm_m34 = speed_max_m34;
    }
    
    // 根据当前状态选择对应的电机控制因子
    float motor12_factor, motor3_factor, motor4_factor;
    if(car_state == STATE_ACROSS_LEFT_1) {
        motor12_factor = LEFT_CROSS_MOTOR12_FACTOR_1;
        motor3_factor = LEFT_CROSS_MOTOR3_FACTOR_1;
        motor4_factor = LEFT_CROSS_MOTOR4_FACTOR_1;
    } else if(car_state == STATE_ACROSS_LEFT_2) {
        motor12_factor = LEFT_CROSS_MOTOR12_FACTOR_2;
        motor3_factor = LEFT_CROSS_MOTOR3_FACTOR_2;
        motor4_factor = LEFT_CROSS_MOTOR4_FACTOR_2;
    } else if(car_state == STATE_ACROSS_LEFT_3) {
        motor12_factor = LEFT_CROSS_MOTOR12_FACTOR_3;
        motor3_factor = LEFT_CROSS_MOTOR3_FACTOR_3;
        motor4_factor = LEFT_CROSS_MOTOR4_FACTOR_3;
    } else {
        motor12_factor = LEFT_CROSS_MOTOR12_FACTOR_1;  // 默认使用第一次左跨垄的因子
        motor3_factor = LEFT_CROSS_MOTOR3_FACTOR_1;
        motor4_factor = LEFT_CROSS_MOTOR4_FACTOR_1;
    }
    
    // 左行基础PWM分配（使用各自的目标PWM值）
    pwm_1 = target_pwm_m12;
    pwm_2 = -target_pwm_m12;
    pwm_3 = -target_pwm_m34;
    pwm_4 = target_pwm_m34;
    
    // 应用电机1、2和电机3、4的速度因子
    pwm_1 = (int)(pwm_1 * motor12_factor);
    pwm_2 = (int)(pwm_2 * motor12_factor);
    pwm_3 = (int)(pwm_3 * motor3_factor);
    pwm_4 = (int)(pwm_4 * motor4_factor);

    // 设置电机PWM值
    MotorA_Ctrl(pwm_1);
    MotorB_Ctrl(pwm_2);
    MotorC_Ctrl(pwm_3);
    MotorD_Ctrl(pwm_4);
}

// 右转向跨垄控制
void Run_Across_Right(void)
{
    // 根据当前状态选择对应的刹车距离阈值
    int target_distance;
    if(car_state == STATE_ACROSS_RIGHT_1) {
        target_distance = TARGET_DISTANCE_RIGHT_1;
    } else if(car_state == STATE_ACROSS_RIGHT_2) {
        target_distance = TARGET_DISTANCE_RIGHT_2;
    } else if(car_state == STATE_ACROSS_RIGHT_3) {
        target_distance = TARGET_DISTANCE_RIGHT_3;
    } else {
        target_distance = TARGET_DISTANCE_RIGHT_1;  // 默认使用第一次右跨垄的距离
    }
    
    // 检查是否到达刹车阈值，如果到达则立即停止电机（由主状态机处理刹车和舵机转动）
    if(distance3 <= target_distance) {
        MotorA_Ctrl(0);
        MotorB_Ctrl(0);
        MotorC_Ctrl(0);
        MotorD_Ctrl(0);
        return;  // 直接返回，不执行后续的PWM设置
    }
    
    int target_pwm = 0;  // 初始化target_pwm，防止未初始化使用
    int pwm_1, pwm_2, pwm_3, pwm_4;
    
    // 检测distance3小于600时触发作业动作组（右跨垄）
    if(distance3 < ACTION_TRIGGER_DISTANCE_RIGHT && !right_cross_action_triggered) {
        // distance3小于600，触发步进和从控舵机1234的动作组
        stepper_enter_delay_counter = STEPPER_ENTER_DELAY_TIME;
        stepper_exit_delay_counter = STEPPER_EXIT_DELAY_TIME;
        // 第三次右跨垄时，param2=3表示只执行状态1-4，不执行状态5
        if(car_state == STATE_ACROSS_RIGHT_3) {
            Master_SendCommand(CMD_STATE_SWITCH_1, 0, 3);  // 发送舵机1234动作组指令（第三次右跨垄，只执行状态1-4）
        } else {
            Master_SendCommand(CMD_STATE_SWITCH_1, 0, 0);  // 发送舵机1234动作组指令
        }
        right_cross_action_triggered = 1;  // 标记已触发，防止重复触发
    }
    
    // 根据当前状态选择对应的速度参数
    int speed_max, speed_normal, speed_exit_min;
    if(car_state == STATE_ACROSS_RIGHT_1) {
        speed_max = CROSS_SPEED_MAX_RIGHT_1;
        speed_normal = CROSS_SPEED_NORMAL_RIGHT_1;
        speed_exit_min = CROSS_SPEED_EXIT_MIN_RIGHT_1;
    } else if(car_state == STATE_ACROSS_RIGHT_2) {
        speed_max = CROSS_SPEED_MAX_RIGHT_2;
        speed_normal = CROSS_SPEED_NORMAL_RIGHT_2;
        speed_exit_min = CROSS_SPEED_EXIT_MIN_RIGHT_2;
    } else if(car_state == STATE_ACROSS_RIGHT_3) {
        speed_max = CROSS_SPEED_MAX_RIGHT_3;
        speed_normal = CROSS_SPEED_NORMAL_RIGHT_3;
        speed_exit_min = CROSS_SPEED_EXIT_MIN_RIGHT_3;
    } else {
        speed_max = CROSS_SPEED_MAX_RIGHT_1;  // 默认使用第一次右跨垄的速度
        speed_normal = CROSS_SPEED_NORMAL_RIGHT_1;
        speed_exit_min = CROSS_SPEED_EXIT_MIN_RIGHT_1;
    }
    
    // PWM控制逻辑（跨右垄：直接使用最大速度，到达700mm时开始线性衰减）
    // 1. 当distance3 < LINEAR_DECEL_START_DISTANCE时：使用出垄控制（线性减速）
    if(distance3 < LINEAR_DECEL_START_DISTANCE) {
        // 出垄控制：根据distance3线性衰减PWM（LINEAR_DECEL_START_DISTANCE时speed_max，LINEAR_DECEL_END_DISTANCE时speed_exit_min）
        float distance_range = (float)LINEAR_DECEL_START_DISTANCE - LINEAR_DECEL_END_DISTANCE;
        float current_range = distance3 - LINEAR_DECEL_END_DISTANCE;
        if(distance_range > 0 && current_range > 0) {
            float speed_ratio = current_range / distance_range;
            if(speed_ratio > 1.0f) speed_ratio = 1.0f;
            if(speed_ratio < 0.0f) speed_ratio = 0.0f;
            target_pwm = speed_exit_min + (speed_max - speed_exit_min) * speed_ratio;
        } else {
            target_pwm = speed_exit_min;
        }
    }
    // 2. 当distance3 >= 700时：使用最大速度
    else {
        target_pwm = speed_max;
    }
    
    // 右行基础PWM分配
    pwm_1 = -target_pwm;
    pwm_2 = target_pwm;
    pwm_3 = target_pwm;
    pwm_4 = -target_pwm;
    
    // 根据当前状态选择对应的电机控制因子
    float motor1_factor, motor2_factor, motor3_factor, motor4_factor;
    if(car_state == STATE_ACROSS_RIGHT_1) {
        motor1_factor = RIGHT_CROSS_MOTOR1_FACTOR_1;
        motor2_factor = RIGHT_CROSS_MOTOR2_FACTOR_1;
        motor3_factor = RIGHT_CROSS_MOTOR3_FACTOR_1;
        motor4_factor = RIGHT_CROSS_MOTOR4_FACTOR_1;
    } else if(car_state == STATE_ACROSS_RIGHT_2) {
        motor1_factor = RIGHT_CROSS_MOTOR1_FACTOR_2;
        motor2_factor = RIGHT_CROSS_MOTOR2_FACTOR_2;
        motor3_factor = RIGHT_CROSS_MOTOR3_FACTOR_2;
        motor4_factor = RIGHT_CROSS_MOTOR4_FACTOR_2;
    } else if(car_state == STATE_ACROSS_RIGHT_3) {
        motor1_factor = RIGHT_CROSS_MOTOR1_FACTOR_3;
        motor2_factor = RIGHT_CROSS_MOTOR2_FACTOR_3;
        motor3_factor = RIGHT_CROSS_MOTOR3_FACTOR_3;
        motor4_factor = RIGHT_CROSS_MOTOR4_FACTOR_3;
    } else {
        motor1_factor = RIGHT_CROSS_MOTOR1_FACTOR_1;  // 默认使用第一次右跨垄的因子
        motor2_factor = RIGHT_CROSS_MOTOR2_FACTOR_1;
        motor3_factor = RIGHT_CROSS_MOTOR3_FACTOR_1;
        motor4_factor = RIGHT_CROSS_MOTOR4_FACTOR_1;
    }
    
    // 应用电机速度因子（纠正车身偏移）
    pwm_1 = (int)(pwm_1 * motor1_factor);
    pwm_2 = (int)(pwm_2 * motor2_factor);
    pwm_3 = (int)(pwm_3 * motor3_factor);
    pwm_4 = (int)(pwm_4 * motor4_factor);
    
    // 设置电机PWM值
    MotorA_Ctrl(pwm_1);
    MotorB_Ctrl(pwm_2);
    MotorC_Ctrl(pwm_3);
    MotorD_Ctrl(pwm_4);
}

// 编码器后退控制（跨垄后调整位置）- 精确倒车算法
void Run_Encoder_Back(void)
{
    extern int enc_1, enc_2, enc_3, enc_4;  // 编码器累计计数
    int enc_2_current, enc_4_current;  // 当前编码器值
    int enc_2_delta_from_start, enc_4_delta_from_start;  // 从开始后退时的增量值
    int enc_2_delta_step, enc_4_delta_step;  // 每步的增量值（用于累加统计）
    int enc_ave;  // 平均后退距离
    int target_pwm;  // 当前目标PWM值
    float distance_ratio;  // 当前距离占总距离的比例
    float accel_distance;  // 加速阶段距离
    float decel_distance;  // 减速阶段距离
    float const_distance;  // 匀速阶段距离
    float remaining_distance;  // 剩余距离
    
    // 读取当前编码器值（累计值）
    enc_2_current = enc_2;
    enc_4_current = enc_4;
    
    // 初始化：记录开始后退时的编码器值
    if(encoder_back_started == 0) {
        enc_2_start = enc_2_current;  // 记录开始时的编码器2值
        enc_4_start = enc_4_current;  // 记录开始时的编码器4值
        encoder_back_distance = 0;  // 重置已后退距离
        encoder_back_started = 1;
        encoder_back_current_speed = ENCODER_BACK_Speed_Min;  // 重置为初始PWM值
        enc_2_last = enc_2_current;
        enc_4_last = enc_4_current;
    }
    
    // 计算每步增量值（当前值 - 上一次值）
    enc_2_delta_step = enc_2_current - enc_2_last;
    enc_4_delta_step = enc_4_current - enc_4_last;
    
    // 计算增量的绝对值
    int enc_2_delta_abs = (enc_2_delta_step < 0) ? (-enc_2_delta_step) : enc_2_delta_step;
    int enc_4_delta_abs = (enc_4_delta_step < 0) ? (-enc_4_delta_step) : enc_4_delta_step;
    
    // 累加增量的绝对值（只使用2和4号编码器）
    total_encoder_back_distance_2 += enc_2_delta_abs;
    total_encoder_back_distance_4 += enc_4_delta_abs;
    encoder_back_distance += enc_2_delta_abs;
    encoder_back_distance += enc_4_delta_abs;
    
    // 使用累加的增量绝对值计算平均后退距离
    enc_ave = encoder_back_distance / 2;  // 2和4号编码器的平均值
    
    // 更新上一次的值（只使用2和4号编码器）
    enc_2_last = enc_2_current;
    enc_4_last = enc_4_current;
    
    // ========== 精确加减速算法：基于距离的梯形加减速 ==========
    // 计算各阶段距离
    accel_distance = encoder_distance_target * ENCODER_BACK_Accel_Ratio;  // 加速阶段距离
    decel_distance = encoder_distance_target * ENCODER_BACK_Decel_Ratio;  // 减速阶段距离
    const_distance = encoder_distance_target - accel_distance - decel_distance;  // 匀速阶段距离
    
    // 计算剩余距离
    remaining_distance = encoder_distance_target - enc_ave;
    
    // 根据当前距离计算目标PWM值
    if(enc_ave <= accel_distance) {
        // 加速阶段：从ENCODER_BACK_Speed_Min线性加速到ENCODER_BACK_Speed_Max
        distance_ratio = (float)enc_ave / accel_distance;  // 0.0 ~ 1.0
        target_pwm = ENCODER_BACK_Speed_Min + (ENCODER_BACK_Speed_Max - ENCODER_BACK_Speed_Min) * distance_ratio;
    } else if(enc_ave <= (accel_distance + const_distance)) {
        // 匀速阶段：保持最大PWM值
        target_pwm = ENCODER_BACK_Speed_Max;
    } else {
        // 减速阶段：从ENCODER_BACK_Speed_Max线性减速到ENCODER_BACK_Speed_Min
        float decel_progress = (float)(enc_ave - accel_distance - const_distance) / decel_distance;  // 0.0 ~ 1.0
        target_pwm = ENCODER_BACK_Speed_Max - (ENCODER_BACK_Speed_Max - ENCODER_BACK_Speed_Min) * decel_progress;
        // 确保PWM不低于最小值
        if(target_pwm < ENCODER_BACK_Speed_Min) {
            target_pwm = ENCODER_BACK_Speed_Min;
        }
    }
    
    // 精确停止：当剩余距离很小时，进一步减速
    if(remaining_distance < 50) {  // 剩余距离小于50个编码器计数
        target_pwm = ENCODER_BACK_Speed_Min * (remaining_distance / 50.0f);  // 线性减速到0
        if(target_pwm < 20) {
            target_pwm = 20;  // 最小PWM限制
        }
    }
    
    // 更新当前速度（用于平滑过渡）
    encoder_back_current_speed = target_pwm;
    
    // 设置四个电机的PWM值（后退，使用负值）
    MotorA_Ctrl(-target_pwm);  // 电机A - 后退
    MotorB_Ctrl(-target_pwm);  // 电机B - 后退
    MotorC_Ctrl(-target_pwm);  // 电机C - 后退
    MotorD_Ctrl(-target_pwm);  // 电机D - 后退

    // 精确停止：检查是否到达目标距离（考虑速度，提前停止）
    if(enc_ave >= encoder_distance_target) {
        // 到达目标距离，立即停止
        MotorA_Ctrl(0);
        MotorB_Ctrl(0);
        MotorC_Ctrl(0);
        MotorD_Ctrl(0);
        encoder_distance_reached = 1;
        encoder_back_started = 0;  // 重置标志，准备下一次后退
        encoder_back_current_speed = 0;  // 重置PWM值
        encoder_back_distance = 0;  // 重置距离
    }
}

// PWM限幅函数（简化版，只做基本限幅）
int Limit_Pwm_moter(int pwm, int min_range, int max_range)
{
    if(pwm > 7000) pwm = 7000;
    if(pwm < -7000) pwm = -7000;
    return pwm;
}


