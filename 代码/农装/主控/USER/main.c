#include "stm32f4xx.h"                  // Device header
#include "oled.h"
#include "delay.h"
#include "control.h"
#include "timer.h"
#include "motor.h"
#include "pid.h"
#include "key.h"
#include "usart2dma.h"  // ���������Ǵ���ͷ�ļ�
#include "usart3dma.h"
#include "usart4dma.h"
#include "usartdma.h"  // ��������ͨѶͷ�ļ�
#include "encoder.h"   // ���ӱ�����ͷ�ļ�
#include "Stepper.h"
#include "guangdian.h"  // 光电对管头文件

/********************************		
			
			GD1   GD2					
		_________________			Y�� ǰ������ (Forward)	
     BHH|Steer6	  Steer5|HHA		 |	
		|	  			|			 |	
	GD6	|	 �����ռ���	|GD5		 +----�� X	
  sonic1|	 Stepper    |sonic3			
	PHT1|_______________|PHT2			
	'	|	   | |		|	'			
	'	|	   | |		|	'	       
	'	|	   |O|		|	'	       
	'	|	   |P|Steer1|	'	       GD1 GD2 ǰ��ɲ������
	'	|	   |E|		|	'	       GD5 GD6 GD7 GD8 ��¢����
	'	|	   |N|		|	'	       sonic1-4 ������������
	'	|Steer2|M|		|	'	       Laser1-2 ¢�༤���״�
	'	|	   |V|		|	'	       PHT1-2 �����䴫����9�	
	'	|	   | |		|	'		   Steer5-8 ���ֶ�� 
	'	|Laser1| |Laser2|	'	       Steer1-2 ��צ���
	'	|_______________|	'          HH A-D ���������
	PHT1| ��		��	  ��	|PHT2          OPENMV ����ͷ
  sonic2| ·		��	  ·	|sonic4		   
	GD8 | ��				|GD7
		|Steer8	  Steer7|
     DHH|_______________|HHC
			
			主控   		从控
	串口1 	主从   		主从
	串口2	激光（前侧）  	陀螺仪
	串口3	激光（左侧）	激光（内侧）
	串口4	激光（右侧）   激光
	串口5	步进   		摄像头
	
********************************/

extern int speedA,speedB,speedC,speedD;
extern int16_t angle;
extern int flag_mpu;
extern float yaw_init;
extern int16_t yaw_pid_out;
extern int m;
extern int flag;
int key=0;
int remote=0;

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init(168);
	TIM7_Cnt_Init(1000-1,84-1);
	OLED_Init();
	GD_Init();  // 光电对管初始化
	Motor_pwm_Init(8400-1,0);
	motor_Gpio_Init();
	
//	PID_Init(&PID_Straight_yaw, 0.0f, 0.0f, 0.0f);        // 直行yaw控制
//	PID_Init(&PID_Across_yaw, 0.0f, 0.0f, 0.0f);          // 跨垄yaw控制
//	PID_Init(&PID_Across_yaw_left, 140.0f, 0.0f, 0.0f);  // 左转跨垄yaw控制
	
	KEY_Init();
		
	uart2_init(230400);  // 左侧激光雷达 (USART2) mzy激光雷达
	uart3_init(230400);  // 右侧激光雷达 (USART3) mzy激光雷达
	uart4_init(921600);  // 前侧模块2 (USART4)
	
	USART1_Init(115200);  
	
	TIM2_Encoder_Init(ENCODER_TIM_PERIOD, 0);  
	TIM3_Encoder_Init(ENCODER_TIM_PERIOD, 0);  
	TIM4_Encoder_Init(ENCODER_TIM_PERIOD, 0);  
	TIM8_Encoder_Init(ENCODER_TIM_PERIOD, 0); 
	
	stepper_init(115200);
	
	OLED_Clear();
		OLED_Refresh_Gram();

	stepper_enable(1, true);     
	delay_ms(500);

//	stepper_move_angle(1, 1, 1000, 220, 550.0f);	
//	delay_ms(500);
	
	
	while(1)
	{
	
		Master_ProcessReceivedData();  
		
		Read_Encoder(); 
				
		key=KEY_Scan(0);
			
		if(key==1)
		{
			// 按下按键1：发送右行指令，执行第二次右跨垄程序，然后刹车
			Master_SendCommand(CMD_SET_DIRECTION, 1, 0);  // 设置方向为右行
						delay_ms(1000);

			last_slave_mode = SLAVE_MODE_PARALLEL;  // 设置为平行模式（跨垄模式）
			// 设置状态为第二次右跨垄，系统会自动执行跨垄并刹车
			car_state = STATE_ACROSS_RIGHT_2;
		}
		if(key==2)
		{
			// 按下按键2：直接开始第二次跨左垄程序
			Master_SendCommand(CMD_SET_DIRECTION, 2, 0);  // 设置方向为左行
			delay_ms(1000);
			last_slave_mode = SLAVE_MODE_PARALLEL;  // 设置为平行模式（跨垄模式）
			// 设置状态为第二次左跨垄，系统会自动执行跨垄并刹车
			car_state = STATE_ACROSS_LEFT_2;
		}
		if(key==3)
		{
			// 触发步进电机和舵机1、2的动作组，时序和主程序中一样
			extern int stepper_enter_delay_counter;  // 步进电机进入动作延迟计数器
			extern int stepper_exit_delay_counter;    // 步进电机退出动作延迟计数器
			
			// 设置步进电机延时计数器（和主程序中触发动作组时一样）
			stepper_enter_delay_counter = STEPPER_ENTER_DELAY_TIME;  // 250ms
			stepper_exit_delay_counter = STEPPER_EXIT_DELAY_TIME;    // 800ms
			
			// 发送舵机1、2动作组指令（从控会处理舵机1、2的时序控制）
			// 从控收到指令后会按顺序执行：延迟30ms->状态1->延迟15ms->状态2->延迟120ms->状态3->延迟260ms->状态4->延迟430ms->状态5
			Master_SendCommand(CMD_STATE_SWITCH_1, 0, 0);
		}
		if(key==4)
		{
//		stepper_move_angle(1, 1, 150, 100, 360.0f); 
		}			
		if(key==5||remote==1)
		{
		// 开始跑垄任务
		car_state = STATE_STRAIGHT;  			// 初始化为直行状态
		mound_count = 0;              			// 重置垄计数
		encoder_distance_target = 1780; 		// 设置编码器目标距离
		encoder_distance_reached = 0;     		// 重置编码器到达标志
		last_slave_mode = -1;              		// 重置从控状态标志
			remote=0;
		}
		
	
		
		
	
	}
	
	
	}

