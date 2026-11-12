/*
******************************************************************************
*Filename      :laser_common.h
*Programmer(s) :历年备赛
*Description   :激光雷达通用定义
******************************************************************************
*/

#ifndef __LASER_COMMON_H
#define __LASER_COMMON_H	 
#include "sys.h"
#include "stm32f4xx.h"
#include <stdint.h>

// STP-23激光雷达协议定义
#define ANGLE_PER_FRAME 12
#define HEADER 0x54
#define POINT_PER_PACK 12
#define VERLEN  0x2C 	//数据位，一帧数据接收到的点数，目前固定为12个点，数据位固定为1

// 激光雷达数据结构定义
typedef struct __attribute__((packed)) Point_Data
{
	uint16_t distance;  //距离
	uint8_t intensity;  //信号强度
}LidarPointStructDef;

typedef struct __attribute__((packed)) Pack_Data
{
	uint8_t header;
	uint8_t ver_len;
	uint16_t temperature;
	uint16_t start_angle;
	LidarPointStructDef point[POINT_PER_PACK];
	uint16_t end_angle;
	uint16_t timestamp;
	uint8_t crc8;
}LiDARFrameTypeDef;

// CRC校验表声明
extern const uint8_t CrcTable[256];

// 传统激光雷达协议定义（保留原有定义）
#define HEADER_OLD 0xAA							/* 起始帧 */
#define device_address 0x00     /* 设备地址 */
#define chunk_offset 0x00       /* 偏移地址参数 */
#define PACK_GET_DISTANCE 0x02 	/* 获取距离数据命令 */
#define PACK_RESET_SYSTEM 0x0D 	/* 复位命令 */
#define PACK_STOP 0x0F 				  /* 停止数据输出命令 */
#define PACK_ACK 0x10           /* 应答数据命令 */
#define PACK_VERSION 0x14       /* 获取设备信息命令 */

typedef struct {
	int16_t distance;  						/* 距离数据，激光雷达目标距离单位 mm */
	uint16_t noise;		 						/* 噪声数据，激光雷达当前环境下的外部噪声干扰，数值越大说明噪声越大 */
	uint32_t peak;								/* 峰值强度信息，激光雷达目标反射的光强度 */
	uint8_t confidence;						/* 置信度，由激光雷达内部算法和接收强度信息融合后的目标置信度 */
	uint32_t intg;     						/* 积分数据，激光雷达当前环境下的积分数据 */
	int16_t reftof;   						/* 温度补偿值，激光雷达芯片内部温度变化补偿值，只是一个温度变化，无法对应实际温度 */
}LidarPointTypedef;

struct AckResultData{
	uint8_t ack_cmd_id;						/* 回复的命令 id */
	uint8_t result; 							/* 1表示成功,0表示失败 */
};

struct LiManuConfig
{
	uint32_t version; 						/* 软件版本号 */
	uint32_t hardware_version; 		/* 硬件版本号 */
	uint32_t manufacture_date; 		/* 生产日期 */
	uint32_t manufacture_time; 		/* 生产时间 */
	uint32_t id1; 								/* 设备 id1 */
	uint32_t id2; 								/* 设备 id2 */
	uint32_t id3; 								/* 设备 id3 */
	uint8_t sn[8]; 								/* sn */
	uint16_t pitch_angle[4]; 			/* 角度信息 */
	uint16_t blind_area[2]; 			/* 盲区信息 */
	uint32_t frequence; 					/* 数据频率 */
};

#endif
