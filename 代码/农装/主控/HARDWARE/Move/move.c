#include "move.h"
#include "sys.h"
#include "control.h"
#include "motor.h"
#include "usartdma.h"  // 添加主从通讯头文件
#ifndef CMD_SET_DIRECTION
#define CMD_SET_DIRECTION  0x06  // 设置运动方向：0=前向, 1=右行, 2=左行, 3=倒退
#endif
extern __IO int i,j,z1,z2,z3,z4;
extern int flag_mpu;
extern int base_speed;
//extern  SonicDis Sonic_Dis;
extern int speedA,speedB,speedC,speedD;
extern uint16_t distance4;  // 添加distance4外部变量声明
uint16_t target_base=0;

void move_forward(void)
{

	z1=0;
	target_base  =8000;	
	
	// 安全检查：如果distance4小于20cm，不执行前进
	if(distance4 >= 380)
	{
		flag_mpu=1;  // 前进模式
		// 等待前进完成或遇到障碍
		while(distance4 >= 380){};  // 持续前进直到距离小于20cm
	}
	
	flag_mpu=11;  // 刹车
	delay_ms(500);
	// 刹车后向从控发送舵机模式切换指令（垂直模式，前向）
	Master_SendCommand(CMD_SET_MODE, 0, 0);     // 0=垂直模式
	// 已删除：不再向从控发送舵机3和4控制指令
	  // 等待舵机转向完成
	
	z4=0;
	while(z4<3000){};// 等待	 

	
	 
}


void move_left(void)
{
	// 安全检查：如果distance4小于20cm，不执行左转
	if(distance4 >= 20)
	{
		flag_mpu=3;  // 左转模式
		base_speed = 3000;
		// 等待左转完成或遇到障碍
		while(distance4 >= 20){};  // 持续左转直到距离小于20cm
	}
	
	flag_mpu=11;  // 刹车
	z4=0;
	while(z4<1000){};	
}

void move_backward(void)
{
	// 后退不需要检查distance4，后退是安全的
	flag_mpu=2;  // 后退模式
	base_speed = 3000;
//	while(Sonic_Dis.A_3>=85){};	
	flag_mpu=11;  // 刹车
	z4=0;
	while(z4<1000){};	
}
void move_right(void)
{
//	// 安全检查：如果distance4小于20cm，不执行右转
//	if(distance4 >= 380)
//	{
//		flag_mpu=4;  // 右转模式
//		base_speed = 10000;
//		while(GDD==0){};  // 等待光电传感器
//		// 等待右转完成或遇到障碍
//		while(distance4 >= 380){};  // 持续右转直到距离小于20cm
//	}
	target_base =6000;
	flag_mpu =2;
//	flag_mpu=11;  // 刹车
	z4=0;
	while(z4<25000){};
		flag_mpu=11;
		z4=0;
		while(z4<10000000){};	
}