/*
******************************************************************************************************************
*Filename      :usart2_dma.h
*Programmer(s) :chu
*Description   :Design for usart2_dma

							 USART2_RX---PD6
							 USART2_TX---PD5
******************************************************************************************************************
*/
#ifndef __USART2_DMA_H
#define __USART2_DMA_H	 
#include "sys.h"


//ʹ��USART2ʱ���������ÿ���ע��
#define U2_DATA_LEN  60
#define USART2_REC_LEN  	80  	//�����������ֽ��� 200

extern u8  USART2_RX_BUF[USART2_REC_LEN];		//���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з�
extern u16 USART2_RX_STA;  
extern u8 connect_flag;//���ӱ�־
extern u8 rec_data_u2[U2_DATA_LEN];   // ��������
extern u8 send_data_u2[U2_DATA_LEN];   
/*
******************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
*                                                   ����3�ú���
******************************************************************************************************************
*/
void USART2_Init(u32 bound);
//void USART2_Init(int Baud);
extern void USART2_DMATransfer(uint32_t *BufferSRC, uint32_t BufferSize);//UASRT DMA��������
extern void DMA1_S1_Reset(void);//DMA1_Stream1 ��������	
void openmv(u8 a);
void uart_init(u32 bound);
void Mea_dis(void);
void Read_LaserDis(unsigned char ID, unsigned int *Data);  
void Set_LaserDis(unsigned char ID, unsigned char Fun,unsigned char Par);

// 陀螺仪数据变量声明
extern float yaw_2;  // 陀螺仪偏航角数据
extern u8 angle_1, angle_2;  // 陀螺仪原始数据
extern short angle_z;  // 陀螺仪角度组合数据

#endif
