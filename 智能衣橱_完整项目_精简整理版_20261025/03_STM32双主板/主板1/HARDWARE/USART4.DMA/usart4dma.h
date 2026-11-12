#ifndef __USART4_DMA_H
#define __USART4_DMA_H

#include "sys.h"
#include "stm32f4xx.h"
#include <stdint.h>

/* Hot-air mover stepper tuning parameters. */
#define BOARD1_HOT_AIR_STEPPER_ADDR       1U
#define BOARD1_HOT_AIR_STEPPER_SPEED_RPM  800U
#define BOARD1_HOT_AIR_STEPPER_ACCEL      80U
#define BOARD1_HOT_AIR_DIR_FORWARD        0U
#define BOARD1_HOT_AIR_DIR_REVERSE        1U
#define BOARD1_HOT_AIR_MOTION_CHANNEL     6U      /* Web/Raspberry Pi channel 6: board1 UART4 hot-air mover. */
#define BOARD1_HOT_AIR_TASK8_OUT_TARGET_DEG  100000LL /* Task 8 outward cumulative target. */
#define BOARD1_HOT_AIR_TASK8_HOME_TARGET_DEG 0LL      /* Task 8 home cumulative target. */
#define BOARD1_HOT_AIR_TASK8_WAIT_MS      30000U  /* Estimated wait for each 100000 degree movement, ms. */

extern volatile uint32_t g_board1_hot_air_stepper_tx_count;
extern volatile uint32_t g_board1_hot_air_stepper_rx_count;
extern volatile uint8_t g_board1_hot_air_stepper_last_rx;
extern volatile uint8_t g_board1_hot_air_stepper_last_cmd;
extern volatile uint8_t g_board1_hot_air_stepper_last_dir;
extern volatile uint64_t g_board1_hot_air_stepper_last_angle;
extern volatile int64_t g_board1_hot_air_stepper_target_deg;
extern volatile int64_t g_board1_hot_air_stepper_position_deg;

void uart4_init(uint32_t bound);
void Board1_HotAirStepper_Init(uint32_t baudrate);
void Board1_HotAirStepper_Enable(uint8_t enable);
void Board1_HotAirStepper_Stop(void);
void Board1_HotAirStepper_MoveAngle(uint8_t dir, uint64_t angle_deg);
void Board1_HotAirStepper_GotoAngle(int64_t target_deg);
void UART4_IRQHandler(void);

#endif
