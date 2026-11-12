#include "stm32f4xx.h"                  // Device header
#include "pid.h"

PIDStru PID_sonic_structure;//����pid�����ṹ��

PIDStru PID_Control_structure;

// 跑垄专用PID结构体
PIDStru PID_Straight_yaw;    // 直行yaw控制
PIDStru PID_Across_yaw;      // 跨垄yaw控制
PIDStru PID_Across_yaw_left; // 左转跨垄yaw控制

float Limit_Pwm_float(float pwm,float pwm_min,float pwm_max)
{
	if(pwm > pwm_max)			 
    pwm =  pwm_max;
	if(pwm < pwm_min)	
    pwm =  pwm_min;
	return pwm;   
}

int PID_Control_sonic(PIDStru *PID,float Expected,float observe_value)//����pid����
{
	PID->Expected = Expected;
	PID->err      = observe_value - PID->Expected;
	PID->pout     = PID->P * PID->err;
	PID->integral = 0.8f*PID->integral;//һ�׵�ͨ�˲�
	PID->integral+= PID->err;
	PID->integral = Limit_Pwm_float(PID->integral,-2,2);//�������޷�(��ֹ�˱�����)
	PID->iout     = PID->I * PID->integral;
	PID->iout     = Limit_Pwm_float(PID->iout,-5,5);   //�޷�(��ֹ�˱�����)
	PID->dout	    = PID->D * (PID->err - PID->last_err);
	PID->last_err = PID->err;
	PID->PIDOUT   = PID->pout + PID->iout + PID->dout;
	PID->PIDOUT   = Limit_Pwm_float(PID->PIDOUT,-7200,7200);
	return (int)PID->PIDOUT;
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

// 跑垄专用控制函数
int Control_Straight_yaw(float Expected, float angle) {
    return PID_Control_yaw(&PID_Straight_yaw, Expected, angle);
}

int Control_Across_yaw(float Expected, float angle) {
    return PID_Control_yaw(&PID_Across_yaw, Expected, angle);
}

int Control_Across_yaw_left(float Expected, float angle) {
    return PID_Control_yaw(&PID_Across_yaw_left, Expected, angle);
}

int Control_sonic(float exp_sonic,float observe_sonic)
{
return 	PID_Control_sonic(&PID_sonic_structure,exp_sonic,observe_sonic);
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






