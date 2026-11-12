#include "stm32f4xx.h"                  // Device header
#include "oled.h"
#include "delay.h"
#include "control.h"
#include "timer.h"
#include "pid.h"
#include "key.h"
#include "steer.h" 
#include "usartdma.h"
#include "usart2dma.h"
#include "usart3dma.h"
#include "usart4dma.h"
#include "usart5dma.h"

/********************************		
			
			GD1   GD2					
		_________________			Y↑ 前进方向 (Forward)	
     BHH|Steer6	  Steer5|HHA		 |	
		|	  			|			 |	
	GD6	|	 土豆收集框	|GD5		 +----→ X	
  sonic1|	 Stepper    |sonic3			
	PHT1|_______________|PHT2			
	'	|	   | |		|	'			
	'	|	   | |		|	'	       
	'	|	   |O|		|	'	       
	'	|	   |P|Steer1|	'	       
	'	|	   |E|		|	'	       
	'	|	   |N|		|	'	       
	'	|Steer2|M|		|	'	       sonic1-4 超声波传感器
	'	|	   |V|		|	'	       Laser1-2 垄侧激光雷达
	'	|	   | |		|	'		   PHT1-2 光电对射传感器	
	'	|Laser1| |Laser2|	'	       Steer5-8 舵轮舵机 
	'	|_______________|	'          Steer1-2 夹爪舵机
	PHT1| 电		电	  线|PHT2          HH A-D 编码器电机
  sonic2| 路		池	  路|sonic4		   OPENMV 摄像头
	GD8 | 板				|GD7
		|Steer8	  Steer7|
     DHH|_______________|HHC
			GD3   GD4
	
********************************/


extern float yaw_init;
extern int16_t yaw_pid_out;
extern int steer_yaw;  
extern int flag;
int key=0;
int angle1,
	angle2;
	
int main(void)
{
  	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init(168);
	TIM7_Cnt_Init(1000-1,84-1);
	OLED_Init();
	KEY_Init();
	
	PID_Init(&PID_steer_yaw_structure, 20.0f, 0.0f, 5.0f);  
	
	PID_Init(&PID_distance4_structure, 3.5f, 0.0f, -0.01f);  
	PID_Init(&PID_distance3_structure, 3.5f, 0.0f, -0.01f);
	
	// 初始化三个左跨垄状态的独立PID
	
	
	
	// 初始化三个左跨垄状态的独立yaw PID
	PID_Init(&PID_steer_yaw_left1_structure, 20.0f, 0.0f, 5.0f);  // 第一次左跨垄yaw PID
	PID_Init(&PID_distance4_left1_structure, 2.5f, 0.0f, -0.05f);  // 第一次左跨垄distance4 PID
	PID_Init(&PID_distance3_left1_structure, 2.5f, 0.0f, -0.05f);  // 第一次左跨垄distance3 PID
	
	PID_Init(&PID_steer_yaw_left2_structure, 20.0f, 0.0f, 5.0f);  // 第二次左跨垄yaw PID
	PID_Init(&PID_distance4_left2_structure, 2.5f, 0.0f, -0.05f);  // 第二次左跨垄distance4 PID
	PID_Init(&PID_distance3_left2_structure, 2.5f, 0.0f, -0.05f);  // 第二次左跨垄distance3 PID
	
	PID_Init(&PID_steer_yaw_left3_structure, 20.0f, 0.0f, 5.0f);  // 第三次左跨垄yaw PID 
	PID_Init(&PID_distance4_left3_structure, 2.5f, 0.0f, -0.05f);  // 第三次左跨垄distance4 PID
	PID_Init(&PID_distance3_left3_structure, 2.5f, 0.0f, -0.05f);  // 第三次左跨垄distance3 PID


	
	TIM1_PWM_Init(10000-1, 42-1);
	
	TIM3_PWM_Init(20000-1, 42-1);  
	
	Steer1_4_Init_Position();
	Steer5_8_Init_Position();
	delay_ms(200);
	
	USART1_Init(115200);
	USART2_Init(9600);
	uart3_init(921600);  
	uart4_init(921600);  
	USART5_Init(115200);
    OpenMV_ResetData();
  
	OLED_Clear();
		
	while(1)
	{
		
		Slave_ProcessReceivedCommand(); 
		Slave_CheckStateSwitchDelay();  // 检查状态切换延迟（非阻塞）
		OpenMV_ProcessReceivedData();  
		
		key=KEY_Scan(0);
			
		if(key==1)
		{
			
		}
 
		if(key==2)
		{
	
		}

	}
	
}

