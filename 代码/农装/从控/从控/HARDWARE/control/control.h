#ifndef __CONTROL_H
#define __CONTROL_H

#include "sys.h"

// 四个方向的舵机控制函数（带陀螺仪修正和distance4控制）
void Steer_GoForward(void);  		 // 直行（前向）
void Steer_TurnRight(void);  		 // 右行（内部集成distance4控制）
void Steer_TurnLeft(void);   		 // 左行（内部集成distance4控制）
void Steer_GoBackward(void); 		 // 倒退
void Steer_Control_5_8_Vertical(void);  // 控制舵机5、6、7、8（使用垂直状态的基础PWM值 + 陀螺仪修正）

// 外部变量声明
extern int steer_direction;          // 舵机方向标志：0=前向, 1=右行, 2=左行, 3=倒退
extern int steer34_state;           // 舵机3和4状态标志：0=默认, 1=状态1(编码器后退), 2=状态2(右行), 3=状态3(左行), 4=状态4(第三次跨左垄)
extern int distance4_control_enable; // distance4控制使能标志
extern int16_t distance4_pid_out;  	 // distance4 PID输出值
extern int distance3_control_enable; // distance3控制使能标志
extern int16_t distance3_pid_out;  	 // distance3 PID输出值

// distance4控制阈值定义
#define DISTANCE4_Threshold1    200  // 阈值1：小于此值时启用控制（单位：mm）
#define DISTANCE4_Threshold2    110   // 阈值2：目标距离值（单位：mm）

// distance3控制阈值定义
#define DISTANCE3_Threshold1    200  // 阈值1：小于此值时启用控制（单位：mm）
#define DISTANCE3_Threshold2    110   // 阈值2：目标距离值（单位：mm）

/**************************************************
舵机一：升降
				/\		最高2100
			   /  \			
			   \  /		
				\/		半降3050
			  ------	下降3300
			  |	   |	
舵机二：开合
			闭合		小开		大开
			2900		3900	   5100
**************************************************/

// 状态1角度设置
#define STEER1_State1_Angle  5100    // 状态切换1：舵机1角度			不动
#define STEER2_State1_Angle  3000    // 状态切换1：舵机2角度		不动
                                     
// 状态2角度设置                        
#define STEER1_State2_Angle  2850    // 状态切换2：舵机1角度		闭合夹爪
#define STEER2_State2_Angle  3000    // 状态切换2：舵机2角度			不动
                                     
// 状态3角度设置                        
#define STEER1_State3_Angle  2850    // 状态切换3：舵机1角度			不动
#define STEER2_State3_Angle  1600    // 状态切换3：舵机2角度		提升
                                     
// 状态4角度设置                        
#define STEER1_State4_Angle  3900    // 状态切换4：舵机1角度		小开
#define STEER2_State4_Angle  1600    // 状态切换4：舵机2角度			不动		
                                     
// 状态5角度设置                        
#define STEER1_State5_Angle  5100    // 状态切换5：舵机1角度		大开
#define STEER2_State5_Angle  2900    // 状态切换5：舵机2角度		半降


/**************************************************
舵机3                           舵机4 
4000  -----      |      -----  6200
          |      |      |
		  |      |      |
         2300          5000

**************************************************/


// 舵机3和4状态角度设置
// 状态1角度设置		同降		  直行/编码器后退 
#define STEER3_State1_Angle  2200    // 状态切换1：舵机3角度		左
#define STEER4_State1_Angle  5050    // 状态切换1：舵机4角度		右
											
// 状态2角度设置       左升右降       左跨垄          		
#define STEER3_State2_Angle  4000    // 状态切换2：舵机3角度		左
#define STEER4_State2_Angle  5050    // 状态切换2：舵机4角度		右
											
// 状态3角度设置       左降右升       右跨垄                 		
#define STEER3_State3_Angle  2200    // 状态切换3：舵机3角度		左
#define STEER4_State3_Angle  2850    // 状态切换3：舵机4角度		右
											
// 状态4角度设置       同升      	  第三次跨左垄                		
#define STEER3_State4_Angle  4000    // 状态切换4：舵机3角度		左
#define STEER4_State4_Angle  2850    // 状态切换4：舵机4角度		右




// 状态切换延迟时间定义
#define STATE_SWITCH_Delay0_Ms  30   // 收到指令后到开始执行状态1的初始延迟时间（单位：毫秒）
#define STATE_SWITCH_Delay1_Ms  15   // 状态1执行完后到状态2执行的延迟时间（单位：毫秒）
#define STATE_SWITCH_Delay2_Ms  120  // 状态2执行完后到状态3执行的延迟时间（单位：毫秒）
#define STATE_SWITCH_Delay3_Ms  260+100  // 状态3执行完后到状态4执行的延迟时间（单位：毫秒）
#define STATE_SWITCH_Delay4_Ms  430+300  // 状态4执行完后到状态5执行的延迟时间（单位：毫秒）

// 舵机基础PWM值定义（直行/倒退模式）
#define STEER5_Vertical_Base  2780  // 舵机5直行/倒退模式基础PWM值
#define STEER6_Vertical_Base  2980  // 舵机6直行/倒退模式基础PWM值
#define STEER7_Vertical_Base  2600  // 舵机7直行/倒退模式基础PWM值
#define STEER8_Vertical_Base  2920  // 舵机8直行/倒退模式基础PWM值

// 舵机基础PWM值定义（左右行模式）
#define STEER5_Parallel_Base  1440  // 舵机5左右行模式基础PWM值
#define STEER6_Parallel_Base  4380  // 舵机6左右行模式基础PWM值
#define STEER7_Parallel_Base  4020  // 舵机7左右行模式基础PWM值
#define STEER8_Parallel_Base  1570  // 舵机8左右行模式基础PWM值

#endif

