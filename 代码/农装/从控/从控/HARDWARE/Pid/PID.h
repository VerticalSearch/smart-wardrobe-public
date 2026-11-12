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





extern PIDStru PID_Control_structure;

extern PIDStru PID_steer_yaw_structure;  // 舵机陀螺仪控制PID结构体

extern PIDStru PID_distance4_structure;  // distance4控制PID结构体
extern PIDStru PID_distance3_structure;  // distance3控制PID结构体

// 三个左跨垄状态的独立PID结构体
extern PIDStru PID_distance4_left1_structure;  // 第一次左跨垄distance4控制PID结构体
extern PIDStru PID_distance3_left1_structure;  // 第一次左跨垄distance3控制PID结构体
extern PIDStru PID_distance4_left2_structure;  // 第二次左跨垄distance4控制PID结构体
extern PIDStru PID_distance3_left2_structure;  // 第二次左跨垄distance3控制PID结构体
extern PIDStru PID_distance4_left3_structure;  // 第三次左跨垄distance4控制PID结构体
extern PIDStru PID_distance3_left3_structure;  // 第三次左跨垄distance3控制PID结构体

// 三个左跨垄状态的独立yaw PID结构体
extern PIDStru PID_steer_yaw_left1_structure;  // 第一次左跨垄yaw控制PID结构体
extern PIDStru PID_steer_yaw_left2_structure;  // 第二次左跨垄yaw控制PID结构体
extern PIDStru PID_steer_yaw_left3_structure;  // 第三次左跨垄yaw控制PID结构体
	
void PID_Init(PIDStru *PID,float P,float I,float D);

int Control_yaw(float Expected, float angle);
int PID_Control_yaw(PIDStru *PID, float Expected, float observe_value);

// 舵机陀螺仪控制函数
int Control_steer_yaw(float Expected, float observe_yaw);
int PID_Control_steer_yaw(PIDStru *PID, float Expected, float observe_value);

// 三个左跨垄状态的独立yaw PID控制函数
int Control_steer_yaw_left1(float Expected, float observe_yaw);
int Control_steer_yaw_left2(float Expected, float observe_yaw);
int Control_steer_yaw_left3(float Expected, float observe_yaw);

// distance4控制函数
int Control_distance4(float Expected, float observe_distance4);
int PID_Control_distance4(PIDStru *PID, float Expected, float observe_value);

// distance3控制函数
int Control_distance3(float Expected, float observe_distance3);
int PID_Control_distance3(PIDStru *PID, float Expected, float observe_value);

// 三个左跨垄状态的独立PID控制函数
int Control_distance4_left1(float Expected, float observe_distance4);
int Control_distance3_left1(float Expected, float observe_distance3);
int Control_distance4_left2(float Expected, float observe_distance4);
int Control_distance3_left2(float Expected, float observe_distance3);
int Control_distance4_left3(float Expected, float observe_distance4);
int Control_distance3_left3(float Expected, float observe_distance3);

float Limit_Pwm_float(float pwm,float pwm_min,float pwm_max);


#endif



