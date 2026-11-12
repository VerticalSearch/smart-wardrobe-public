#ifndef __CONTROL_H
#define __CONTROL_H

#include "sys.h"
#include <stdint.h>

/* Main board1 system tick. */
#define BOARD1_TICK_PERIOD_MS 1U

extern volatile uint32_t g_board1_tick_ms;
extern volatile uint8_t g_board1_camera_trigger_placeholder;
extern volatile uint8_t g_board1_camera_trigger_side_id;

void Control_Init(void);

#endif
