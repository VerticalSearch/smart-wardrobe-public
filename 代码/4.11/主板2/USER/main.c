#include "stm32f4xx.h"
#include "delay.h"
#include "timer.h"
#include "control.h"
#include "oled.h"
#include "usartdma.h"
#include "stepper_multi.h"
#include "board2_action_list.h"

static void board2_oled_show_status(void)
{
    OLED_Clear();

    OLED_ShowString(0, 0, (const u8 *)"RX:", 16);
    OLED_ShowNum(24, 0, g_board2_link_rx_count, 4, 16);
    OLED_ShowString(64, 0, (const u8 *)"CMD:", 16);
    OLED_ShowNum(96, 0, g_board2_link_cmd_count, 4, 16);

    OLED_ShowString(0, 16, (const u8 *)"T:", 16);
    OLED_ShowNum(16, 16, g_board2_link_last_cmd, 2, 16);
    OLED_ShowString(48, 16, (const u8 *)"CH:", 16);
    OLED_ShowNum(72, 16, g_board2_link_last_channel, 1, 16);
    OLED_ShowString(88, 16, (const u8 *)"A:", 16);
    OLED_ShowNum(104, 16, g_board2_link_last_angle, 4, 16);

    OLED_ShowString(0, 32, (const u8 *)"U6:", 16);
    OLED_ShowNum(24, 32, g_board2_servo_uart6_rx_count, 4, 16);
    OLED_ShowString(72, 32, (const u8 *)"SV:", 16);
    OLED_ShowNum(96, 32, g_board2_servo_cmd_count, 4, 16);

    OLED_ShowString(0, 48, (const u8 *)"ADC:", 16);
    OLED_ShowNum(40, 48, Board2_AdcIo_Read(1U), 1, 16);
    OLED_ShowNum(56, 48, Board2_AdcIo_Read(2U), 1, 16);
    OLED_ShowNum(72, 48, Board2_AdcIo_Read(3U), 1, 16);
    OLED_ShowNum(88, 48, Board2_AdcIo_Read(4U), 1, 16);

    OLED_Refresh_Gram();
}

int main(void)
{
    uint32_t last_oled_tick_ms = 0U;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init(168);

    OLED_Init();
    OLED_Clear();
    OLED_Refresh_Gram();

    Control_Init();
    Board2_ActionList_Init();

    USART1_Init(115200);
    stepper_multi_init_all(115200);

    TIM1_Servo_PWM_Init();
    USART6_DigitRx_Init(9600);
    TIM7_Cnt_Init(1000 - 1, 84 - 1);

#if (BOARD2_ACTION_LIST_AUTOSTART != 0U)
    Board2_ActionList_StartDefault();
#endif

    while (1)
    {
        Board2_ProcessReceivedCommand();
        Board2_ProcessServoUart6Command();
        Board2_RunPendingKeyEvent();
        Board2_ActionList_Task();
        Board2_LinkTaskStatusService();

        if ((uint32_t)(g_board2_tick_ms - last_oled_tick_ms) >= 200U)
        {
            last_oled_tick_ms = g_board2_tick_ms;
            board2_oled_show_status();
        }

        delay_ms(5);
    }
}
