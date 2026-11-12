#include "control.h"
#include "stm32f4xx.h"                  // Device header
#include "oled.h"
#include "delay.h"
#include "timer.h"
#include "pid.h"
#include "key.h"
#include "steer.h" 
  
#include "usartdma.h"      // 包含SlaveSensorData_t定义
#include "usart2dma.h"
#include "usart3dma.h"
#include "usart4dma.h"
#include "usart5dma.h"


int flag_yaw_init=0;      // 陀螺仪初始化标志位
float yaw_init=0;         // 陀螺仪初始角度
float yaw=0;              // 相对角度（以初始角度为0度）
int16_t yaw_pid_out=0;
int16_t sonic_pid_out=0;
int steer_yaw=0;          // 舵机角度修正量（根据陀螺仪计算）
int base_speed=0; 
int m=0;
int flag=0;
int steer_direction=0;    // 舵机方向标志：0=前向, 1=右行, 2=左行, 3=倒退
int steer34_state=0;     // 舵机3和4状态标志：0=默认, 1=状态1(编码器后退), 2=状态2(右行), 3=状态3(左行), 4=状态4(第三次跨左垄)
__IO int MS10=0,MS20=0;
__IO int yaw_send_counter=0;  // 陀螺仪数据发送计数器（TIM7中断中递增，每50ms发送一次）

extern int key;
extern float yaw_2;  // 陀螺仪偏航角数据

// distance4控制相关变量
int distance4_control_enable=0;  // distance4控制使能标志
int16_t distance4_pid_out=0;  // distance4 PID输出值

// distance3控制相关变量
int distance3_control_enable=0;  // distance3控制使能标志
int16_t distance3_pid_out=0;  // distance3 PID输出值

// distance3和distance4的PID禁用标志（当从小于阈值变为大于阈值时禁用）
static int distance3_pid_disabled = 0;  // distance3 PID是否已禁用
static int distance4_pid_disabled = 0;  // distance4 PID是否已禁用
static int distance3_last_below_threshold = 0;  // distance3上一次是否小于阈值
static int distance4_last_below_threshold = 0;  // distance4上一次是否小于阈值

// 重置distance3和distance4的PID禁用标志（进入新的跨垄状态时调用）
void Reset_Distance_PID_Disabled_Flags(void)
{
    distance3_pid_disabled = 0;  // 重置distance3 PID禁用标志
    distance4_pid_disabled = 0;  // 重置distance4 PID禁用标志
    distance3_last_below_threshold = 0;  // 重置distance3上一次状态
    distance4_last_below_threshold = 0;  // 重置distance4上一次状态
}


void TIM7_IRQHandler(void)       
{ 

	if(TIM_GetITStatus(TIM7,TIM_IT_Update)==SET)
	{	
		MS10++;
		MS20++;
		m++;
		yaw_send_counter++;  // 陀螺仪数据发送计数器递增
		
		// ========== 定期主动发送陀螺仪数据到主控（每10ms发送一次，100Hz更新率） ==========
		// 10ms更新频率与主控速度控制周期（10ms）同步，确保角度修正响应及时
		if(yaw_send_counter >= 10)  // 10ms = 10个1ms中断周期
		{
			yaw_send_counter = 0;
			Slave_SendSensorData();  // 主动发送陀螺仪数据
		}
		
		// ========== 陀螺仪初始化（上电后1秒记录初始角度） ==========
		if(m >= 1000 && flag_yaw_init == 0)
		{
			yaw_init = yaw_2;   // 记录当前角度作为初始值
			flag_yaw_init = 1;  // 标记已初始化
		}
		
		// ========== 计算相对角度（以初始角度为0度） ==========
		if(flag_yaw_init == 1)  // 初始化完成后才计算相对角度
		{
			// 简化版：直接计算差值，然后处理跨越±180度边界
			float delta = yaw_2 - yaw_init;
			
			// 处理跨越±180度边界的情况
			if(delta > 180.0f)
			{
				delta = delta - 360.0f;  // 从右侧跨过来，减去360度
			}
			else if(delta < -180.0f)
			{
				delta = delta + 360.0f;  // 从左侧跨过来，加上360度
			}
			
			yaw = -delta;  // 相对角度（取反，使初始方向为0度）
			
			// ========== 根据相对角度计算舵机修正量 ==========
			steer_yaw = Control_steer_yaw(0.0f, yaw);  // 期望角度0度，实际角度yaw
		}
	
		if(MS20>=10)
		{
			
			OLED_ShowNum(0,0,flag,1,12);
			
			OLED_ShowNum(0,12,KEY1 ,1,12);
			OLED_ShowNum(0,24,KEY2 ,1,12);

			OLED_ShowNum(24,0,openmv_data.x_coordinate,3,12);
			OLED_ShowNum(24,12,openmv_data.y_coordinate,3,12);	
			OLED_ShowNum(24,24,openmv_data.area,3,12);
					
		OLED_ShowNum (0,36,distance3   ,4,12);
		OLED_ShowNum (0,48,distance4   ,4,12);
		// 显示角度信息
		OLED_Show3FNum(48,0, yaw, 3, 3, 12);     // 显示相对角度（以初始角度为0度）
		OLED_Show3FNum(48,12, yaw_2, 3, 3, 12);  // 显示原始角度
		
		// 显示舵机修正量
		OLED_ShowNum(72,24, steer_yaw, 4, 12);     // 显示舵机角度修正量
		
		// 显示distance4控制信息
		OLED_ShowNum(72,48, distance4_control_enable, 1, 12);  // 显示distance4控制使能标志
		OLED_ShowNum(84,48, distance4_pid_out, 4, 12);  // 显示distance4 PID输出
		
		// 显示distance3控制信息
		OLED_ShowNum(72,36, distance3_control_enable, 1, 12);  // 显示distance3控制使能标志
		OLED_ShowNum(84,36, distance3_pid_out, 4, 12);  // 显示distance3 PID输出
		
//		// 显示主从通讯调试信息
//		extern uint8_t cmd_received_flag;
//		extern Command_t received_cmd;
//		extern u8 rec_data_u1[20];
//		OLED_ShowNum(0,0, cmd_received_flag, 1, 12);  // 显示是否收到命令标志
//		OLED_ShowNum(12,0, received_cmd.cmd_type, 2, 12);  // 显示接收到的命令类型 (CMD_GET_SENSOR=1, CMD_SET_DIRECTION=6)
//		OLED_ShowNum(36,0, steer_direction, 1, 12);  // 显示steer_direction（0=前向, 1=右行, 2=左行, 3=倒退）
//		OLED_ShowNum(0,12, rec_data_u1[0], 3, 12);  // 显示接收到的第一个字节（帧头，应该是0xFF/255）
//		OLED_ShowNum(36,12, rec_data_u1[1], 3, 12);  // 显示接收到的命令类型原始值
//		OLED_ShowNum(72,12, rec_data_u1[2], 3, 12);  // 显示接收到的param1（舵机方向值）
//		OLED_ShowNum(108,12, rec_data_u1[3], 3, 12);  // 显示接收到的param2

		OLED_Refresh_Gram();

			MS20=0;
		}

		// 根据steer_direction持续控制舵机姿态（每10ms更新一次）
		if(MS10>=10)
		{
			// 根据steer_direction调用相应的舵机控制函数
			switch(steer_direction)
			{
				case 0:  // 直行
					Steer_GoForward();
					break;
				case 1:  // 右行（右跨垄）
					Steer_TurnRight();
					break;
				case 2:  // 左行（左跨垄）
					Steer_TurnLeft();
					break;
				case 3:  // 倒退
					// 第五次后退时使用特殊的舵机控制函数
					extern uint8_t is_fifth_backward;
					if(is_fifth_backward == 1)
					{
						Steer_Control_5_8_Vertical();  // 第五次后退：使用特殊的舵机5-8控制函数
					}
					else
					{
						Steer_GoBackward();  // 其他次后退：使用正常的倒退控制函数
					}
					break;
				default:  // 默认直行
					Steer_GoForward();
					break;
			}
			MS10=0;
		}
		
//		Stepper_Update();
		
		// 每10ms触发一次超声波（4个同时）
//		if(z2>=10)						
//		{
//			Ultrasonic_Trig_Start(); // 使用原来的函数，4个同时触发
////			OpenMV_ProcessReceivedData();

//			z2=0;
//		}
		
		// 每5ms读取一次距离数据
//		if(z1>=10)						
//		{
//			Sonic_Get_Distance(); ///超声测距
//			z1=0;
//		}		
		
		// 从控只负责舵机控制，不控制电机（电机控制代码已删除）
		
		// 状态切换延迟0处理（TIM7每1ms中断一次）：收到指令 -> 状态1
		if(state_switch_delay0_counter > 0)
		{
			state_switch_delay0_counter--;  // 计数器减1
			if(state_switch_delay0_counter == 0)
			{
				state_switch_delay0_timeout = 1;  // 设置超时标志
			}
		}
		
		// 状态切换延迟1处理（TIM7每1ms中断一次）：状态1 -> 状态2
		if(state_switch_delay1_counter > 0)
		{
			state_switch_delay1_counter--;  // 计数器减1
			if(state_switch_delay1_counter == 0)
			{
				state_switch_delay1_timeout = 1;  // 设置超时标志
			}
		}
		
		// 状态切换延迟2处理（TIM7每1ms中断一次）：状态2 -> 状态3
		if(state_switch_delay2_counter > 0)
		{
			state_switch_delay2_counter--;  // 计数器减1
			if(state_switch_delay2_counter == 0)
			{
				state_switch_delay2_timeout = 1;  // 设置超时标志
			}
		}
		
		// 状态切换延迟3处理（TIM7每1ms中断一次）：状态3 -> 状态4
		if(state_switch_delay3_counter > 0)
		{
			state_switch_delay3_counter--;  // 计数器减1
			if(state_switch_delay3_counter == 0)
			{
				state_switch_delay3_timeout = 1;  // 设置超时标志
			}
		}
		
		// 状态切换延迟4处理（TIM7每1ms中断一次）：状态4 -> 状态5
		if(state_switch_delay4_counter > 0)
		{
			state_switch_delay4_counter--;  // 计数器减1
			if(state_switch_delay4_counter == 0)
			{
				state_switch_delay4_timeout = 1;  // 设置超时标志
			}
		}
		
		// 第三次跨左垄舵机3和4延时处理（TIM7每1ms中断一次）
		extern uint16_t left3_steer34_delay_counter;
		if(left3_steer34_delay_counter > 0)
		{
			left3_steer34_delay_counter--;  // 计数器减1（每1ms减1）
		}
			
			TIM_ClearITPendingBit(TIM7,TIM_IT_Update);  
		
	}
	
}

// 保持垂直姿态（正常行驶时）- 已废弃，使用Steer_GoForward()代替
// void Steer_MaintainVertical(void)
// {
//     // 纵向模式（垂直状态） - 使用基础PWM值 + 陀螺仪修正
//     Steer_PWM_Control(5, 2950 - steer_yaw);  // 舵机5 - 添加陀螺仪修正
//     Steer_PWM_Control(6, 2850 - steer_yaw);  // 舵机6 - 添加陀螺仪修正
//     Steer_PWM_Control(7, 3080);              // 舵机7 - 不修正
//     Steer_PWM_Control(8, 2900);              // 舵机8 - 不修正
// }

// 跨垄姿态调整（平行状态）- 已废弃，使用Steer_TurnRight()或Steer_TurnLeft()代替
// void Steer_AdjustForCrossing(void)
// {
//     // 横向模式（平行状态） - 使用基础PWM值 + 陀螺仪修正
//     Steer_PWM_Control(5, 1600 - steer_yaw);  // 舵机5 - 添加陀螺仪修正
//     Steer_PWM_Control(6, 4220 );  // 舵机6 - 添加陀螺仪修正
//     Steer_PWM_Control(7, 4480 - steer_yaw);  // 舵机7 - 添加陀螺仪修正
//     Steer_PWM_Control(8, 1550 );  // 舵机8 - 添加陀螺仪修正
// }

// 直行（前向）- 使用垂直状态的PWM
void Steer_GoForward(void)
{
    // 直行模式 - 使用垂直状态的基础PWM值 + 陀螺仪修正
    Steer5_8_PWM_Control(5, STEER5_Vertical_Base - steer_yaw);  // 舵机5 - 垂直状态 + 陀螺仪修正
    Steer5_8_PWM_Control(6, STEER6_Vertical_Base - steer_yaw);  // 舵机6 - 垂直状态 + 陀螺仪修正
    Steer5_8_PWM_Control(7, STEER7_Vertical_Base );  // 舵机7 - 垂直状态 + 陀螺仪修正
    Steer5_8_PWM_Control(8, STEER8_Vertical_Base );  // 舵机8 - 垂直状态 + 陀螺仪修正
    
    // 控制舵机3和4到状态1（直行/编码器后退）
    Steer1_4_PWM_Control(3, STEER3_State1_Angle);  // 舵机3：状态1
    Steer1_4_PWM_Control(4, STEER4_State1_Angle);  // 舵机4：状态1
}

// 右行 - 使用平行状态的PWM
void Steer_TurnRight(void)
{
    extern uint16_t distance4;  // 从usart4dma.h引入
    extern uint16_t distance3;  // 从usart3dma.h引入
    
    // 右行模式基础PWM值
    int base_pwm5 = STEER5_Parallel_Base;
    int base_pwm6 = STEER6_Parallel_Base;
    int base_pwm7 = STEER7_Parallel_Base;
    int base_pwm8 = STEER8_Parallel_Base;
    
    // 计算distance3和distance4的平均值，作为目标值（让它们趋向于相等）
    uint16_t distance_target = (distance3 + distance4) / 2;
    
    // 检测distance4从小于阈值变为大于阈值的情况
    int distance4_current_below_threshold = (distance4 < DISTANCE4_Threshold1) ? 1 : 0;
    
    // 如果从小于阈值变为大于阈值，禁用PID控制（在这个跨垄状态中不再使用）
    if(distance4_last_below_threshold == 1 && distance4_current_below_threshold == 0)
    {
        distance4_pid_disabled = 1;  // 标记为已禁用，在这个跨垄状态中不再使用
    }
    
    // 更新上一次的状态
    distance4_last_below_threshold = distance4_current_below_threshold;
    
    // 检查distance4是否小于阈值1且未被禁用，如果是则应用distance4控制
    if(distance4 < DISTANCE4_Threshold1 && distance4_pid_disabled == 0)
    {
        // 使用PID控制，让distance4趋向于平均值（与distance3相等）
        distance4_pid_out = Control_distance4(distance_target, distance4);
        distance4_control_enable = 1;
    }
    else
    {
        // 不在阈值内或已被禁用，禁用distance4控制（PID输出为0）
        distance4_control_enable = 0;
        distance4_pid_out = 0;
    }
    
    // 检测distance3从小于阈值变为大于阈值的情况
    int distance3_current_below_threshold = (distance3 < DISTANCE3_Threshold1) ? 1 : 0;
    
    // 如果从小于阈值变为大于阈值，禁用PID控制（在这个跨垄状态中不再使用）
    if(distance3_last_below_threshold == 1 && distance3_current_below_threshold == 0)
    {
        distance3_pid_disabled = 1;  // 标记为已禁用，在这个跨垄状态中不再使用
    }
    
    // 更新上一次的状态
    distance3_last_below_threshold = distance3_current_below_threshold;
    
    // 检查distance3是否小于阈值1且未被禁用，如果是则应用distance3控制
    if(distance3 < DISTANCE3_Threshold1 && distance3_pid_disabled == 0)
    {
        // 使用PID控制，让distance3趋向于平均值（与distance4相等）
        distance3_pid_out = Control_distance3(distance_target, distance3);
        distance3_control_enable = 1;
    }
    else
    {
        // 不在阈值内或已被禁用，禁用distance3控制（PID输出为0）
        distance3_control_enable = 0;
        distance3_pid_out = 0;
    }
    
    // 应用舵机控制：基础PWM值 + 陀螺仪修正（始终应用）+ distance4控制 + distance3控制
    // 注意：无论distance3/distance4是否在阈值内，都会应用陀螺仪修正（steer_yaw）以保持直线行驶
    Steer5_8_PWM_Control(5, base_pwm5 - steer_yaw - distance4_pid_out + distance3_pid_out);  // 舵机5
    Steer5_8_PWM_Control(6, base_pwm6 - distance4_pid_out + distance3_pid_out);  // 舵机6
    Steer5_8_PWM_Control(7, base_pwm7 - steer_yaw - distance4_pid_out + distance3_pid_out);  // 舵机7
    Steer5_8_PWM_Control(8, base_pwm8 - distance4_pid_out + distance3_pid_out);  // 舵机8
    
    // 控制舵机3和4
    extern uint8_t keep_steer34_state1;
    if(keep_steer34_state1 == 1)
    {
        // 如果收到动作指令，在整个跨垄过程中持续控制舵机3和4到状态1（同降）
        Steer1_4_PWM_Control(3, STEER3_State1_Angle);  // 舵机3：状态1
        Steer1_4_PWM_Control(4, STEER4_State1_Angle);  // 舵机4：状态1
    }
    else
    {
        // 正常控制：舵机3和4到状态3（右跨垄：左降右升）
        Steer1_4_PWM_Control(3, STEER3_State3_Angle);  // 舵机3：状态3
        Steer1_4_PWM_Control(4, STEER4_State3_Angle);  // 舵机4：状态3
    }
}

// 左行 - 使用平行状态的PWM
void Steer_TurnLeft(void)
{
    extern uint16_t distance4;  // 从usart4dma.h引入
    extern uint16_t distance3;  // 从usart3dma.h引入
    extern float yaw;  // 从control.c引入相对角度
    
    // 根据steer34_state选择不同的yaw PID计算steer_yaw
    int steer_yaw_left;
    if(steer34_state == 2)  // 第一次左跨垄
    {
        steer_yaw_left = Control_steer_yaw_left1(0.0f, yaw);
    }
    else if(steer34_state == 5)  // 第二次左跨垄
    {
        steer_yaw_left = Control_steer_yaw_left2(0.0f, yaw);
    }
    else if(steer34_state == 4)  // 第三次左跨垄
    {
        steer_yaw_left = Control_steer_yaw_left3(0.0f, yaw);
    }
    else
    {
        steer_yaw_left = steer_yaw;  // 默认使用通用yaw PID
    }
    
    // 左行模式基础PWM值
    int base_pwm5 = STEER5_Parallel_Base;
    int base_pwm6 = STEER6_Parallel_Base;
    int base_pwm7 = STEER7_Parallel_Base;
    int base_pwm8 = STEER8_Parallel_Base;
    
    // 计算distance3和distance4的平均值，作为目标值（让它们趋向于相等）
    uint16_t distance_target = (distance3 + distance4) / 2;
    
    // 根据steer34_state选择不同的PID结构体
    // 检测distance4从小于阈值变为大于阈值的情况
    int distance4_current_below_threshold = (distance4 < DISTANCE4_Threshold1) ? 1 : 0;
    
    // 如果从小于阈值变为大于阈值，禁用PID控制（在这个跨垄状态中不再使用）
    if(distance4_last_below_threshold == 1 && distance4_current_below_threshold == 0)
    {
        distance4_pid_disabled = 1;  // 标记为已禁用，在这个跨垄状态中不再使用
    }
    
    // 更新上一次的状态
    distance4_last_below_threshold = distance4_current_below_threshold;
    
    // 检查distance4是否小于阈值1且未被禁用，如果是则应用distance4控制
    if(distance4 < DISTANCE4_Threshold1 && distance4_pid_disabled == 0)
    {
        // 根据steer34_state使用不同的PID
        if(steer34_state == 2)  // 第一次左跨垄
        {
            distance4_pid_out = Control_distance4_left1(distance_target, distance4);
        }
        else if(steer34_state == 5)  // 第二次左跨垄
        {
            distance4_pid_out = Control_distance4_left2(distance_target, distance4);
        }
        else if(steer34_state == 4)  // 第三次左跨垄
        {
            distance4_pid_out = Control_distance4_left3(distance_target, distance4);
        }
        else
        {
            distance4_pid_out = Control_distance4(distance_target, distance4);  // 默认使用通用PID
        }
        distance4_control_enable = 1;
    }
    else
    {
        // 不在阈值内或已被禁用，禁用distance4控制（PID输出为0）
        distance4_control_enable = 0;
        distance4_pid_out = 0;
    }
    
    // 检测distance3从小于阈值变为大于阈值的情况
    int distance3_current_below_threshold = (distance3 < DISTANCE3_Threshold1) ? 1 : 0;
    
    // 如果从小于阈值变为大于阈值，禁用PID控制（在这个跨垄状态中不再使用）
    if(distance3_last_below_threshold == 1 && distance3_current_below_threshold == 0)
    {
        distance3_pid_disabled = 1;  // 标记为已禁用，在这个跨垄状态中不再使用
    }
    
    // 更新上一次的状态
    distance3_last_below_threshold = distance3_current_below_threshold;
    
    // 检查distance3是否小于阈值1且未被禁用，如果是则应用distance3控制
    if(distance3 < DISTANCE3_Threshold1 && distance3_pid_disabled == 0)
    {
        // 根据steer34_state使用不同的PID
        if(steer34_state == 2)  // 第一次左跨垄
        {
            distance3_pid_out = Control_distance3_left1(distance_target, distance3);
        }
        else if(steer34_state == 5)  // 第二次左跨垄
        {
            distance3_pid_out = Control_distance3_left2(distance_target, distance3);
        }
        else if(steer34_state == 4)  // 第三次左跨垄
        {
            distance3_pid_out = Control_distance3_left3(distance_target, distance3);
        }
        else
        {
            distance3_pid_out = Control_distance3(distance_target, distance3);  // 默认使用通用PID
        }
        distance3_control_enable = 1;
    }
    else
    {
        // 不在阈值内或已被禁用，禁用distance3控制（PID输出为0）
        distance3_control_enable = 0;
        distance3_pid_out = 0;
    }
    
    // 应用舵机控制：基础PWM值 + 陀螺仪修正（始终应用）+ distance4控制 + distance3控制
    // 注意：无论distance3/distance4是否在阈值内，都会应用陀螺仪修正（steer_yaw_left）以保持直线行驶
    // 使用对应左跨垄状态的yaw PID输出
    Steer5_8_PWM_Control(5, base_pwm5 + distance4_pid_out - distance3_pid_out);  // 舵机5
    Steer5_8_PWM_Control(6, base_pwm6 -steer_yaw_left + distance4_pid_out - distance3_pid_out);  // 舵机6
    Steer5_8_PWM_Control(7, base_pwm7 + distance4_pid_out - distance3_pid_out);  // 舵机7
    Steer5_8_PWM_Control(8, base_pwm8 -steer_yaw_left + distance4_pid_out - distance3_pid_out);  // 舵机8
    
    // 控制舵机3和4
    extern uint8_t keep_steer34_state1;
    extern uint16_t left3_steer34_delay_counter;
    // 第三次跨左垄时，延时1000ms后使用状态4的角度（同升）
    if(steer34_state == 4)  // 第三次跨左垄（同升）
    {
        // 延时1000ms后才控制到状态4
        if(left3_steer34_delay_counter == 0)
        {
            Steer1_4_PWM_Control(3, STEER3_State4_Angle);  // 舵机3：状态4
            Steer1_4_PWM_Control(4, STEER4_State4_Angle);  // 舵机4：状态4
        }
        // 延时期间不控制舵机3和4（保持之前的状态）
    }
    else if(keep_steer34_state1 == 1)
    {
        // 如果收到动作指令，在整个跨垄过程中持续控制舵机3和4到状态1（同降）
        Steer1_4_PWM_Control(3, STEER3_State1_Angle);  // 舵机3：状态1
        Steer1_4_PWM_Control(4, STEER4_State1_Angle);  // 舵机4：状态1
    }
    else
    {
        // 正常控制：根据steer34_state控制舵机3和4
        if(steer34_state == 2 || steer34_state == 5)  // 第一次或第二次左跨垄（左升右降）
        {
            Steer1_4_PWM_Control(3, STEER3_State2_Angle);  // 舵机3：状态2
            Steer1_4_PWM_Control(4, STEER4_State2_Angle);  // 舵机4：状态2
        }
        else  // 默认：普通左跨垄（左升右降）
        {
            Steer1_4_PWM_Control(3, STEER3_State2_Angle);  // 舵机3：状态2
            Steer1_4_PWM_Control(4, STEER4_State2_Angle);  // 舵机4：状态2
        }
    }
}

// 倒退 - 使用垂直状态的PWM（反向）
void Steer_GoBackward(void)
{
    // 倒退模式 - 使用垂直状态的基础PWM值（反向）+ 陀螺仪修正
    Steer5_8_PWM_Control(5, STEER5_Vertical_Base );  // 舵机5 - 垂直状态反向 + 陀螺仪修正
    Steer5_8_PWM_Control(6, STEER6_Vertical_Base );  // 舵机6 - 垂直状态反向 + 陀螺仪修正
    Steer5_8_PWM_Control(7, STEER7_Vertical_Base - steer_yaw);  // 舵机7 - 垂直状态反向 + 陀螺仪修正
    Steer5_8_PWM_Control(8, STEER8_Vertical_Base - steer_yaw);  // 舵机8 - 垂直状态反向 + 陀螺仪修正
    
    // 控制舵机3和4到状态1（编码器后退）
    Steer1_4_PWM_Control(3, STEER3_State1_Angle);  // 舵机3：状态1
    Steer1_4_PWM_Control(4, STEER4_State1_Angle);  // 舵机4：状态1
}

// 控制舵机5、6、7、8（使用垂直状态的基础PWM值 + 陀螺仪修正）
void Steer_Control_5_8_Vertical(void)
{
    // 使用垂直状态的基础PWM值 + 陀螺仪修正
    Steer5_8_PWM_Control(5, STEER5_Vertical_Base );  // 舵机5 - 垂直状态基础值
    Steer5_8_PWM_Control(6, STEER6_Vertical_Base );  // 舵机6 - 垂直状态基础值
    Steer5_8_PWM_Control(7, STEER7_Vertical_Base - steer_yaw+40);  // 舵机7 - 垂直状态基础值 + 陀螺仪修正
    Steer5_8_PWM_Control(8, STEER8_Vertical_Base - steer_yaw+40);  // 舵机8 - 垂直状态基础值 + 陀螺仪修正

    Steer1_4_PWM_Control(3, STEER3_State1_Angle);  // 舵机3：状态1
    Steer1_4_PWM_Control(4, STEER4_State1_Angle);  // 舵机4：状态1

}


