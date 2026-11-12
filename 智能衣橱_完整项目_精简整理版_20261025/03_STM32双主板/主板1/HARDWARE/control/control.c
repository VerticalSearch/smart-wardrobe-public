#include "control.h"
#include "stm32f4xx.h"

volatile uint32_t g_board1_tick_ms = 0U;
volatile uint8_t g_board1_camera_trigger_placeholder = 0U;
volatile uint8_t g_board1_camera_trigger_side_id = 0U;

void Control_Init(void)
{
    g_board1_tick_ms = 0U;
    g_board1_camera_trigger_placeholder = 0U;
    g_board1_camera_trigger_side_id = 0U;
}

void TIM7_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM7, TIM_IT_Update) == SET)
    {
        g_board1_tick_ms = g_board1_tick_ms + BOARD1_TICK_PERIOD_MS;
        TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
    }
}
