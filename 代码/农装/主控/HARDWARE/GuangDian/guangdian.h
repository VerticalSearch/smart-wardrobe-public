#ifndef __GUANGDIAN_H
#define __GUANGDIAN_H			  	 

// 光电管初始化函数
void GD_Init(void);

// 光电管读取函数声明
u8 GD_Read_GD1(void);  // 读取GD1状态 (PE8)
u8 GD_Read_GD2(void);  // 读取GD2状态 (PE9)
u8 GD_Read_GD3(void);  // 读取GD3状态 (PE10)
u8 GD_Read_GD4(void);  // 读取GD4状态 (PE11)
u8 GD_Read_GD5(void);  // 读取GD5状态 (PE15)
u8 GD_Read_GD6(void);  // 读取GD6状态 (PE14)
u8 GD_Read_GD7(void);  // 读取GD7状态 (PE13)
u8 GD_Read_GD8(void);  // 读取GD8状态 (PE12)
void GD_Read_All(u8 *gd_status);  // 读取所有光电管状态



#endif  

