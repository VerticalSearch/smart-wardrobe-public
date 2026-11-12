#include "stm32f4xx.h"                  // Device header
#include "pid.h"

PIDStru PID_Control_structure;

PIDStru PID_steer_yaw_structure;  // 舵机陀螺仪控制PID结构体

PIDStru PID_distance4_structure;  // distance4控制PID结构体
PIDStru PID_distance3_structure;  // distance3控制PID结构体

// 三个左跨垄状态的独立PID结构体
PIDStru PID_distance4_left1_structure;  // 第一次左跨垄distance4控制PID结构体
PIDStru PID_distance3_left1_structure;  // 第一次左跨垄distance3控制PID结构体
PIDStru PID_distance4_left2_structure;  // 第二次左跨垄distance4控制PID结构体
PIDStru PID_distance3_left2_structure;  // 第二次左跨垄distance3控制PID结构体
PIDStru PID_distance4_left3_structure;  // 第三次左跨垄distance4控制PID结构体
PIDStru PID_distance3_left3_structure;  // 第三次左跨垄distance3控制PID结构体

// 三个左跨垄状态的独立yaw PID结构体
PIDStru PID_steer_yaw_left1_structure;  // 第一次左跨垄yaw控制PID结构体
PIDStru PID_steer_yaw_left2_structure;  // 第二次左跨垄yaw控制PID结构体
PIDStru PID_steer_yaw_left3_structure;  // 第三次左跨垄yaw控制PID结构体

float Limit_Pwm_float(float pwm,float pwm_min,float pwm_max)
{
	if(pwm > pwm_max)			 
    pwm =  pwm_max;
	if(pwm < pwm_min)	
    pwm =  pwm_min;
	return pwm;   
}

int PID_Control_yaw(PIDStru *PID, float Expected, float observe_value)
{
    PID->Expected = Expected;
    PID->err = observe_value - PID->Expected;
    PID->pout = PID->P * PID->err;
    PID->integral = 0.8f * PID->integral;  // һ�׵�ͨ�˲�
    PID->integral += PID->err;
    PID->integral = Limit_Pwm_float(PID->integral, -100, 100); // �����޷�
    PID->iout = PID->I * PID->integral;
    PID->iout = Limit_Pwm_float(PID->iout, -400, 400);        // ��������޷�

    PID->dout = PID->D * (PID->err - PID->last_err);
    PID->last_err = PID->err;

    PID->PIDOUT = PID->pout + PID->iout + PID->dout;
    PID->PIDOUT = Limit_Pwm_float(PID->PIDOUT, -4000, 4000);  // ������޷�

    return (int)PID->PIDOUT;
}


int Control_yaw(float Expected, float angle) {
    return PID_Control_yaw(&PID_Control_structure, Expected, angle);
}

/**************************************************************************
 * 功能：舵机陀螺仪PID控制函数
 * 参数：PID     - PID结构体指针
 *       Expected - 期望角度值（通常为0度）
 *       observe_value - 实际角度值（yaw相对角度）
 * 返回：PID输出值（舵机角度修正量）
 * 说明：用于根据陀螺仪偏航角控制舵机，保持直线行驶
 **************************************************************************/
int PID_Control_steer_yaw(PIDStru *PID, float Expected, float observe_value)
{
	PID->Expected = Expected;        // 期望角度（0度）
	PID->err = observe_value - PID->Expected;  // 误差 = 实际角度 - 期望角度
	PID->pout = PID->P * PID->err;   // 比例项
	
	PID->integral = 0.8f * PID->integral;  // 一阶低通滤波
	PID->integral += PID->err;
	PID->integral = Limit_Pwm_float(PID->integral, -60, 60);  // 积分限幅
	PID->iout = PID->I * PID->integral;
	PID->iout = Limit_Pwm_float(PID->iout, -120, 120);  // 积分输出限幅
	
	PID->dout = PID->D * (PID->err - PID->last_err);  // 微分项
	PID->last_err = PID->err;
	
	PID->PIDOUT = PID->pout + PID->iout + PID->dout;
	PID->PIDOUT = Limit_Pwm_float(PID->PIDOUT, -400, 400);  // 输出限幅（舵机角度范围）
	return (int)PID->PIDOUT;
}

/**************************************************************************
 * 功能：舵机陀螺仪控制函数（用户接口）
 * 参数：Expected - 期望角度值（通常为0）
 *       observe_yaw - 实际角度值（yaw相对角度）
 * 返回：舵机角度修正量
 **************************************************************************/
int Control_steer_yaw(float Expected, float observe_yaw)
{
	return PID_Control_steer_yaw(&PID_steer_yaw_structure, Expected, observe_yaw);
}

// 三个左跨垄状态的独立yaw PID控制函数
int Control_steer_yaw_left1(float Expected, float observe_yaw)
{
	return PID_Control_steer_yaw(&PID_steer_yaw_left1_structure, Expected, observe_yaw);
}

int Control_steer_yaw_left2(float Expected, float observe_yaw)
{
	return PID_Control_steer_yaw(&PID_steer_yaw_left2_structure, Expected, observe_yaw);
}

int Control_steer_yaw_left3(float Expected, float observe_yaw)
{
	return PID_Control_steer_yaw(&PID_steer_yaw_left3_structure, Expected, observe_yaw);
}

/**************************************************************************
 * 功能：distance4的PID控制函数
 * 参数：PID     - PID结构体指针
 *       Expected - 期望距离值（threshold2）
 *       observe_value - 实际距离值（distance4）
 * 返回：PID输出值（舵机角度修正量）
 * 说明：用于根据distance4控制舵机，使车身接近目标距离
 **************************************************************************/
int PID_Control_distance4(PIDStru *PID, float Expected, float observe_value)
{
	PID->Expected = Expected;        // 期望距离
	PID->err = observe_value - PID->Expected;  // 误差 = 实际距离 - 期望距离
	PID->pout = PID->P * PID->err;   // 比例项
	
	PID->integral = 0.8f * PID->integral;  // 一阶低通滤波
	PID->integral += PID->err;
	PID->integral = Limit_Pwm_float(PID->integral, -200, 200);  // 积分限幅
	PID->iout = PID->I * PID->integral;
	PID->iout = Limit_Pwm_float(PID->iout, -200, 200);  // 积分输出限幅
	
	PID->dout = PID->D * (PID->err - PID->last_err);  // 微分项
	PID->last_err = PID->err;
	
	PID->PIDOUT = PID->pout + PID->iout + PID->dout;
	PID->PIDOUT = Limit_Pwm_float(PID->PIDOUT, -400, 400);  // 输出限幅
	return (int)PID->PIDOUT;
}

/**************************************************************************
 * 功能：distance4控制函数（用户接口）
 * 参数：Expected - 期望距离值（threshold2）
 *       observe_distance4 - 实际距离值（distance4）
 * 返回：舵机角度修正量
 **************************************************************************/
int Control_distance4(float Expected, float observe_distance4)
{
	return PID_Control_distance4(&PID_distance4_structure, Expected, observe_distance4);
}

/**************************************************************************
 * 功能：distance3的PID控制函数
 * 参数：PID     - PID结构体指针
 *       Expected - 期望距离值（threshold2）
 *       observe_value - 实际距离值（distance3）
 * 返回：PID输出值（舵机角度修正量）
 * 说明：用于根据distance3控制舵机，使车身接近目标距离
 **************************************************************************/
int PID_Control_distance3(PIDStru *PID, float Expected, float observe_value)
{
	PID->Expected = Expected;        // 期望距离
	PID->err = observe_value - PID->Expected;  // 误差 = 实际距离 - 期望距离
	PID->pout = PID->P * PID->err;   // 比例项
	
	PID->integral = 0.8f * PID->integral;  // 一阶低通滤波
	PID->integral += PID->err;
	PID->integral = Limit_Pwm_float(PID->integral, -200, 200);  // 积分限幅
	PID->iout = PID->I * PID->integral;
	PID->iout = Limit_Pwm_float(PID->iout, -200, 200);  // 积分输出限幅
	
	PID->dout = PID->D * (PID->err - PID->last_err);  // 微分项
	PID->last_err = PID->err;
	
	PID->PIDOUT = PID->pout + PID->iout + PID->dout;
	PID->PIDOUT = Limit_Pwm_float(PID->PIDOUT, -400, 400);  // 输出限幅
	return (int)PID->PIDOUT;
}

/**************************************************************************
 * 功能：distance3控制函数（用户接口）
 * 参数：Expected - 期望距离值（threshold2）
 *       observe_distance3 - 实际距离值（distance3）
 * 返回：舵机角度修正量
 **************************************************************************/
int Control_distance3(float Expected, float observe_distance3)
{
	return PID_Control_distance3(&PID_distance3_structure, Expected, observe_distance3);
}

// 三个左跨垄状态的独立PID控制函数
int Control_distance4_left1(float Expected, float observe_distance4)
{
	return PID_Control_distance4(&PID_distance4_left1_structure, Expected, observe_distance4);
}

int Control_distance3_left1(float Expected, float observe_distance3)
{
	return PID_Control_distance3(&PID_distance3_left1_structure, Expected, observe_distance3);
}

int Control_distance4_left2(float Expected, float observe_distance4)
{
	return PID_Control_distance4(&PID_distance4_left2_structure, Expected, observe_distance4);
}

int Control_distance3_left2(float Expected, float observe_distance3)
{
	return PID_Control_distance3(&PID_distance3_left2_structure, Expected, observe_distance3);
}

int Control_distance4_left3(float Expected, float observe_distance4)
{
	return PID_Control_distance4(&PID_distance4_left3_structure, Expected, observe_distance4);
}

int Control_distance3_left3(float Expected, float observe_distance3)
{
	return PID_Control_distance3(&PID_distance3_left3_structure, Expected, observe_distance3);
}























void PID_Init(PIDStru *PID,float P,float I,float D)
{
	PID->P 			= P;
	PID->I 			= I;
	PID->D 			= D;
	PID->integral  = 0.0f;
	PID->pout 		= 0.0f;
	PID->iout 		= 0.0f;
	PID->dout 		= 0.0f;
	PID->IMAX   	= 1000.0f;
	PID->PMAX   	= 10000.0f;
	PID->DMAX   	= 10000.0f;
	PID->Expected 	= 0.0f;
	PID->err		= 0.0f;
	PID->last_err	= 0.0f;
}






