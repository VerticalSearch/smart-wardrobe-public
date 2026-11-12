#ifndef __STEPPER_MULTI_H
#define __STEPPER_MULTI_H

#include "sys.h"
#include <stdint.h>

#ifndef __cplusplus
#ifndef __bool_true_false_are_defined
typedef enum { false = 0, true = 1 } bool;
#define __bool_true_false_are_defined 1
#endif
#endif

typedef enum {
    STEPPER_CH1_UART2 = 0,
    STEPPER_CH2_UART3 = 1,
    STEPPER_CH3_UART4 = 2,
    STEPPER_CH4_UART5 = 3,
    STEPPER_CH5_UART6 = 4,
    STEPPER_CH_COUNT
} StepperChannel_t;

/* Initialize UART2~UART5 for stepper protocol output; USART6 is reserved for servo digit RX */
void stepper_multi_init_all(uint32_t baud);
/* Send enable/disable frame to a single motor */
void stepper_multi_enable(StepperChannel_t ch, uint8_t addr, bool en);
/* Send angle-move frame to a single motor */
void stepper_multi_move_angle(StepperChannel_t ch, uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, float angle);
/* UART2 shared bus helper: apply enable state to ID1 and ID2 */
void stepper_multi_enable_dual_opposite_uart2(uint8_t addr_id1, uint8_t addr_id2, bool en);
/* UART2 shared bus helper: ID1 uses dir_id1, ID2 uses opposite direction */
void stepper_multi_move_angle_dual_opposite_uart2(uint8_t addr_id1,
                                                   uint8_t addr_id2,
                                                   uint8_t dir_id1,
                                                   uint16_t speed,
                                                   uint8_t acc,
                                                   float angle);
/* Driver feedback diagnostics */
extern volatile uint32_t g_stepper_multi_rx_count;
extern volatile uint32_t g_stepper_multi_rx_overflow_count;
extern volatile uint32_t g_stepper_multi_status_ok_count;
extern volatile uint32_t g_stepper_multi_status_timeout_count;
extern volatile uint32_t g_stepper_multi_wait_timeout_count;
extern volatile uint8_t g_stepper_multi_last_channel;
extern volatile uint8_t g_stepper_multi_last_addr;
extern volatile uint8_t g_stepper_multi_last_status;

/* Clear buffered feedback bytes for one stepper UART */
void stepper_multi_clear_rx(StepperChannel_t ch);
/* Read Zhangdatou/ZDT motor status flag with command 0x3A */
uint8_t stepper_multi_read_status(StepperChannel_t ch, uint8_t addr, uint8_t *status, uint16_t timeout_ms);
/* Wait until one driver reports position reached in the 0x3A status flag */
uint8_t stepper_multi_wait_reached(StepperChannel_t ch, uint8_t addr, uint32_t timeout_ms);
/* UART2 X axis helper: wait for both ID1 and ID2 to reach position */
uint8_t stepper_multi_wait_reached_dual_opposite_uart2(uint8_t addr_id1, uint8_t addr_id2, uint32_t timeout_ms);

#endif
