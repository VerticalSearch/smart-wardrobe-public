#ifndef __PID_H
#define __PID_H		

typedef struct
{
	float P;
	float pout;
	float I;
	float iout;
	float D;
	float dout;
	float integral;		   //������
	float err;       	   //ƫ��ֵ
	float last_err;  	   //�ϴ�ƫ��ֵ
	float Expected;  	   //����ֵ
	float PMAX;			   //P����޷�ֵ
	float IMAX;			   //I����޷�ֵ
	float DMAX;			   //D����޷�ֵ
	float PIDOUT;    	   //���ֵ
} PIDStru;





extern PIDStru PID_sonic_structure;

extern PIDStru PID_Control_structure;

// 跑垄专用PID结构体
extern PIDStru PID_Straight_yaw;    // 直行yaw控制
extern PIDStru PID_Across_yaw;      // 跨垄yaw控制
extern PIDStru PID_Across_yaw_left; // 左转跨垄yaw控制
	
void PID_Init(PIDStru *PID,float P,float I,float D);

int Control_yaw(float Expected, float angle);
int PID_Control_yaw(PIDStru *PID, float Expected, float observe_value);

// 跑垄专用控制函数
int Control_Straight_yaw(float Expected, float angle);
int Control_Across_yaw(float Expected, float angle);
int Control_Across_yaw_left(float Expected, float angle);

int Control_sonic(float exp_sonic,float observe_sonic);
int PID_Control_sonic(PIDStru *PID,float Expected,float observe_value);

float Limit_Pwm_float(float pwm,float pwm_min,float pwm_max);


#endif



