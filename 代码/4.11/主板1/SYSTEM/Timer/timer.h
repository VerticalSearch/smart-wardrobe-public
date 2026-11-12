#ifndef __TIMER_H
#define __TIMER_H			

void TIM7_Cnt_Init(u16 arr,u32 psc);

/* 主板1继电器/开关量输出通道：默认低电平关闭，高电平触发外设工作 */
#define BOARD1_RELAY_CH_FAN_MAIN          1U  /* PWMD：原热风/风机主输出 */
#define BOARD1_RELAY_CH_HOT_AIR           2U  /* DIRD：原加热/热风输出 */
#define BOARD1_RELAY_CH_PWMC_AUX          3U  /* PWMC：备用输出 */
#define BOARD1_RELAY_CH_UV_LAMP           4U  /* DIRC：紫外灯，拉高亮灯 */
#define BOARD1_RELAY_CH_VENT_FAN          5U  /* PWMB：换气风扇，拉高转动 */
#define BOARD1_RELAY_CH_FILL_LIGHT        6U  /* DIRB：拍照区补光灯 */

/* 兼容旧名称：旧代码里的“负离子/备用”实际已经改接为紫外灯/换气风扇 */
#define BOARD1_RELAY_CH_NEGATIVE_ION      BOARD1_RELAY_CH_UV_LAMP
#define BOARD1_RELAY_CH_AUX               BOARD1_RELAY_CH_VENT_FAN

void Board1_RelayIo_Init(void);
void Board1_Relay_Set(u8 channel, u8 enable);
void Board1_FillLight_Set(u8 enable);
void Board1_UvLamp_Set(u8 enable);
void Board1_VentFan_Set(u8 enable);



#endif
