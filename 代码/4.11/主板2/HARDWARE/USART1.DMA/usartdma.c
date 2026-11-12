#include "usartdma.h"
#include "stepper_multi.h"
#include "control.h"
#include "timer.h"
#include "board2_action_list.h"
#include "board2_action_tuning.h"
#include "delay.h"

#define BOARD2_STEPPER_DEFAULT_ADDR      1U
#define BOARD2_STEPPER_UART2_ADDR_ID1    1U
#define BOARD2_STEPPER_UART2_ADDR_ID2    2U
#define BOARD2_STEPPER_FALLBACK_SPEED    1200U
#define BOARD2_STEPPER_FALLBACK_ACCEL    20U
#define BOARD2_LEGACY_DIRECT_ACTION_ENABLE 0U

/* ========================== 网页任务10：刷毛清理动作参数（可调） ========================== */
#define BOARD2_ACTION10_SERVO4_WORK_ANGLE_DEG      80U     /* 第一步：舵机4先转到80度，让刷毛机构进入工作姿态 */
#define BOARD2_ACTION10_SERVO4_HOME_ANGLE_DEG      BOARD2_ACTION_BRUSH_SERVO4_HOME_DEG /* 最后一步：舵机4回到刷毛结束姿态 */
#define BOARD2_ACTION10_BRUSH_LOW_TARGET_DEG       (-18000)/* 第二步：刷毛电机累计位置转到-18000度 */
#define BOARD2_ACTION10_BRUSH_HOME_TARGET_DEG      0       /* 第三步：刷毛电机累计位置回到0度 */
#define BOARD2_ACTION10_SERVO_WAIT_MS              500U    /* 舵机转动后等待时间，单位ms */
#define BOARD2_ACTION10_BRUSH_MOVE_WAIT_MS         9000U   /* 刷毛电机单段运动估算等待时间，单位ms */

/* Module-local UART1 DMA state */
static uint8_t s_uart1_rx_dma_buffer[UART1_DATA_LEN];
static uint8_t s_uart1_tx_dma_buffer[UART1_DATA_LEN];

#define BOARD2_LINK_RX_QUEUE_LEN 16U

static volatile Board2CommandFrame_t s_rx_queue[BOARD2_LINK_RX_QUEUE_LEN];
static volatile uint8_t s_rx_q_head = 0U;
static volatile uint8_t s_rx_q_tail = 0U;
static volatile uint8_t s_rx_q_count = 0U;
static uint8_t s_board2_remote_channel_param = 0U;

#define USART6_DIGIT_RX_QUEUE_SIZE 32U

static uint8_t s_usart6_digit_rx_queue[USART6_DIGIT_RX_QUEUE_SIZE];
static volatile uint8_t s_usart6_digit_q_head = 0U;
static volatile uint8_t s_usart6_digit_q_tail = 0U;
static volatile uint8_t s_usart6_digit_q_count = 0U;
volatile uint32_t g_board2_servo_uart6_rx_count = 0U;
volatile uint8_t g_board2_servo_uart6_last_byte = 0U;
volatile uint32_t g_board2_servo_cmd_count = 0U;
volatile int g_board2_servo_ch1_angle_deg = TIM1_SERVO1_INIT_ANGLE_DEG;
volatile int g_board2_servo_ch2_angle_deg = TIM1_SERVO2_INIT_ANGLE_DEG;
volatile int g_board2_servo_ch3_angle_deg = TIM1_SERVO3_INIT_ANGLE_DEG;
volatile int g_board2_servo_ch4_angle_deg = TIM1_SERVO4_INIT_ANGLE_DEG;
volatile uint32_t g_board2_link_rx_count = 0U;
volatile uint32_t g_board2_link_cmd_count = 0U;
volatile uint8_t g_board2_link_last_cmd = 0U;
volatile uint8_t g_board2_link_last_channel = 0U;
volatile uint16_t g_board2_link_last_angle = 0U;
volatile int16_t g_board2_link_last_target = 0;

static void board2_run_brush_clean_motion(void);
static void board2_link_send_action_status(uint8_t action_id,
                                           uint8_t status_code,
                                           uint16_t detail);
static void board2_link_flush_pending_action_status(void);
static void board2_link_begin_action_status(uint8_t action_id, uint8_t target_c);
static void board2_link_finish_action_status(uint8_t status_code);
static void board2_start_action(uint8_t action_id, uint8_t target_c);
static volatile uint8_t s_board2_link_active_action_id = 0U;
static volatile uint8_t s_board2_link_active_target_c = 0U;
static volatile uint8_t s_board2_link_active_action_running = 0U;
static volatile uint8_t s_board2_link_pending_status_valid = 0U;
static volatile uint8_t s_board2_link_pending_status_action_id = 0U;
static volatile uint8_t s_board2_link_pending_status_code = 0U;
static volatile uint16_t s_board2_link_pending_status_detail = 0U;

static uint8_t board2_decode_channel_from_param(uint8_t channel_param, StepperChannel_t *channel)
{
    if (channel == 0)
    {
        return 0U;
    }

    switch (channel_param)
    {
    case 1: *channel = STEPPER_CH1_UART2; return 1U;
    case 2: *channel = STEPPER_CH2_UART3; return 1U;
    case 3: *channel = STEPPER_CH3_UART4; return 1U;
    case 4: *channel = STEPPER_CH4_UART5; return 1U;
    case 5: *channel = STEPPER_CH4_UART5; return 1U;
    default: return 0U;
    }
}

static uint8_t board2_current_stepper_addr(StepperChannel_t channel)
{
    if (channel == STEPPER_CH4_UART5)
    {
        if (s_board2_remote_channel_param == BOARD2_LINK_CHANNEL_UART5_MOTOR1)
        {
            return BOARD2_AXIS_UART5_MOTOR1_ADDR;
        }

        return BOARD2_AXIS_TELESCOPE_ADDR;
    }

    if (channel == STEPPER_CH2_UART3)
    {
        return BOARD2_AXIS_Y_ADDR;
    }

    if (channel == STEPPER_CH3_UART4)
    {
        return BOARD2_AXIS_Z_ADDR;
    }

    return BOARD2_STEPPER_DEFAULT_ADDR;
}

static void board2_stepper_enable_by_channel(StepperChannel_t channel, bool enable)
{
    if (channel == STEPPER_CH1_UART2)
    {
        /* UART2 uses two motors (ID1/ID2) */
        stepper_multi_enable_dual_opposite_uart2(BOARD2_STEPPER_UART2_ADDR_ID1,
                                                 BOARD2_STEPPER_UART2_ADDR_ID2,
                                                 enable);
    }
    else
    {
        stepper_multi_enable(channel, board2_current_stepper_addr(channel), enable);
    }
}

static void board2_stop_all_remote_steppers(void)
{
    s_board2_remote_channel_param = 1U;
    board2_stepper_enable_by_channel(STEPPER_CH1_UART2, false);
    s_board2_remote_channel_param = 2U;
    board2_stepper_enable_by_channel(STEPPER_CH2_UART3, false);
    s_board2_remote_channel_param = 3U;
    board2_stepper_enable_by_channel(STEPPER_CH3_UART4, false);
    s_board2_remote_channel_param = BOARD2_LINK_CHANNEL_TELESCOPE;
    board2_stepper_enable_by_channel(STEPPER_CH4_UART5, false);
    s_board2_remote_channel_param = BOARD2_LINK_CHANNEL_UART5_MOTOR1;
    board2_stepper_enable_by_channel(STEPPER_CH4_UART5, false);
}

static uint16_t board2_fixed_speed_by_channel(StepperChannel_t channel)
{
    switch (channel)
    {
    case STEPPER_CH1_UART2: return BOARD2_AXIS_X_SPEED_RPM;
    case STEPPER_CH2_UART3: return BOARD2_AXIS_Y_SPEED_RPM;
    case STEPPER_CH3_UART4: return BOARD2_AXIS_Z_SPEED_RPM;
    case STEPPER_CH4_UART5:
        return (s_board2_remote_channel_param == BOARD2_LINK_CHANNEL_UART5_MOTOR1) ?
               BOARD2_AXIS_UART5_MOTOR1_SPEED_RPM :
               BOARD2_AXIS_TELESCOPE_SPEED_RPM;
    default: return BOARD2_STEPPER_FALLBACK_SPEED;
    }
}

static uint8_t board2_fixed_accel_by_channel(StepperChannel_t channel)
{
    switch (channel)
    {
    case STEPPER_CH1_UART2: return BOARD2_ACCEL_U8(BOARD2_AXIS_X_ACCEL);
    case STEPPER_CH2_UART3: return BOARD2_ACCEL_U8(BOARD2_AXIS_Y_ACCEL);
    case STEPPER_CH3_UART4: return BOARD2_ACCEL_U8(BOARD2_AXIS_Z_ACCEL);
    case STEPPER_CH4_UART5:
        return (s_board2_remote_channel_param == BOARD2_LINK_CHANNEL_UART5_MOTOR1) ?
               BOARD2_ACCEL_U8(BOARD2_AXIS_UART5_MOTOR1_ACCEL) :
               BOARD2_ACCEL_U8(BOARD2_AXIS_TELESCOPE_ACCEL);
    default: return BOARD2_ACCEL_U8(BOARD2_STEPPER_FALLBACK_ACCEL);
    }
}

static volatile float *board2_accum_by_channel(StepperChannel_t channel)
{
    switch (channel)
    {
    case STEPPER_CH1_UART2: return &g_board2_x_accum_deg;
    case STEPPER_CH2_UART3: return &g_board2_y_accum_deg;
    case STEPPER_CH3_UART4: return &g_board2_z_accum_deg;
    case STEPPER_CH4_UART5:
        return (s_board2_remote_channel_param == BOARD2_LINK_CHANNEL_UART5_MOTOR1) ?
               &g_board2_uart5_motor1_accum_deg :
               &g_board2_telescope_accum_deg;
    default: return 0;
    }
}

static uint8_t board2_positive_dir_by_channel(StepperChannel_t channel)
{
    switch (channel)
    {
    case STEPPER_CH1_UART2: return BOARD2_AXIS_X_KEY_DIR;
    case STEPPER_CH2_UART3: return BOARD2_AXIS_Y_KEY_DIR;
    case STEPPER_CH3_UART4: return BOARD2_AXIS_Z_KEY_DIR;
    case STEPPER_CH4_UART5:
        return (s_board2_remote_channel_param == BOARD2_LINK_CHANNEL_UART5_MOTOR1) ?
               BOARD2_AXIS_UART5_MOTOR1_KEY_DIR :
               BOARD2_AXIS_TELESCOPE_KEY_DIR;
    default: return BOARD_LINK_DIR_FORWARD;
    }
}

static uint8_t board2_negative_dir_by_channel(StepperChannel_t channel)
{
    switch (channel)
    {
    case STEPPER_CH1_UART2: return BOARD2_AXIS_X_HOME_DIR;
    case STEPPER_CH2_UART3: return BOARD2_AXIS_Y_HOME_DIR;
    case STEPPER_CH3_UART4: return BOARD2_AXIS_Z_HOME_DIR;
    case STEPPER_CH4_UART5:
        return (s_board2_remote_channel_param == BOARD2_LINK_CHANNEL_UART5_MOTOR1) ?
               BOARD2_AXIS_UART5_MOTOR1_HOME_DIR :
               BOARD2_AXIS_TELESCOPE_HOME_DIR;
    default: return BOARD_LINK_DIR_REVERSE;
    }
}

static uint8_t board2_axis_dir_from_link_control(StepperChannel_t channel, uint8_t control)
{
    return (control == BOARD_LINK_DIR_REVERSE) ?
           board2_negative_dir_by_channel(channel) :
           board2_positive_dir_by_channel(channel);
}

static float board2_signed_angle_by_channel(StepperChannel_t channel, uint8_t dir, float angle_deg)
{
    switch (channel)
    {
    case STEPPER_CH1_UART2:
        return (dir == BOARD2_AXIS_X_POS_DIR) ? angle_deg : -angle_deg;
    case STEPPER_CH2_UART3:
        return (dir == BOARD2_AXIS_Y_POS_DIR) ? angle_deg : -angle_deg;
    case STEPPER_CH3_UART4:
        return (dir == BOARD2_AXIS_Z_POS_DIR) ? angle_deg : -angle_deg;
    case STEPPER_CH4_UART5:
        if (s_board2_remote_channel_param == BOARD2_LINK_CHANNEL_UART5_MOTOR1)
        {
            return (dir == BOARD2_AXIS_UART5_MOTOR1_POS_DIR) ? angle_deg : -angle_deg;
        }
        return (dir == BOARD2_AXIS_TELESCOPE_POS_DIR) ? angle_deg : -angle_deg;
    default:
        return angle_deg;
    }
}

static void board2_track_remote_motion(StepperChannel_t channel, uint8_t dir, float angle_deg)
{
    volatile float *accum = board2_accum_by_channel(channel);
    float signed_angle;

    if (angle_deg <= 0.0f)
    {
        return;
    }

    if (accum == 0)
    {
        return;
    }

    signed_angle = board2_signed_angle_by_channel(channel, dir, angle_deg);
    *accum += signed_angle;

    if (channel == STEPPER_CH1_UART2)
    {
        g_board2_x_send_count++;
    }
    else if (channel == STEPPER_CH2_UART3)
    {
        g_board2_y_send_count++;
    }
    else if (channel == STEPPER_CH3_UART4)
    {
        g_board2_z_send_count++;
    }
    else if (channel == STEPPER_CH4_UART5)
    {
        if (s_board2_remote_channel_param == BOARD2_LINK_CHANNEL_UART5_MOTOR1)
        {
            g_board2_uart5_motor1_send_count++;
        }
        else
        {
            g_board2_telescope_send_count++;
        }
    }
}

static uint8_t board2_stepper_move_angle_by_channel(StepperChannel_t channel,
                                                    uint8_t dir,
                                                    uint16_t speed_rpm,
                                                    uint8_t accel,
                                                    float angle_deg)
{
    if (angle_deg <= 0.0f)
    {
        return 1U;
    }

    if (channel == STEPPER_CH1_UART2)
    {
        /* UART2: ID2 direction is opposite to ID1 */
        stepper_multi_move_angle_dual_opposite_uart2(BOARD2_STEPPER_UART2_ADDR_ID1,
                                                      BOARD2_STEPPER_UART2_ADDR_ID2,
                                                      dir,
                                                      speed_rpm,
                                                      accel,
                                                      angle_deg);
    }
    else
    {
        stepper_multi_move_angle(channel,
                                 board2_current_stepper_addr(channel),
                                 dir,
                                 speed_rpm,
                                 accel,
                                 angle_deg);
    }

    if (channel == STEPPER_CH1_UART2)
    {
        if (stepper_multi_wait_reached_dual_opposite_uart2(BOARD2_STEPPER_UART2_ADDR_ID1,
                                                           BOARD2_STEPPER_UART2_ADDR_ID2,
                                                           BOARD2_AXIS_CLOSED_LOOP_TIMEOUT_MS) == 0U)
        {
            return 0U;
        }
    }
    else if ((channel != STEPPER_CH4_UART5) &&
             (stepper_multi_wait_reached(channel,
                                         board2_current_stepper_addr(channel),
                                         BOARD2_AXIS_CLOSED_LOOP_TIMEOUT_MS) == 0U))
    {
        return 0U;
    }

    board2_track_remote_motion(channel, dir, angle_deg);
    return 1U;
}

static void board2_stepper_goto_angle_by_channel(StepperChannel_t channel,
                                                 int16_t target_deg,
                                                 uint16_t speed_rpm,
                                                 uint8_t accel)
{
    volatile float *current = board2_accum_by_channel(channel);
    float current_deg;
    float target;
    float delta;
    float move_angle;
    uint8_t dir;

    if (current == 0)
    {
        return;
    }

    current_deg = *current;
    target = (float)target_deg;
    delta = target - current_deg;
    if (delta > -0.5f && delta < 0.5f)
    {
        *current = target;
        return;
    }

    if (delta > 0.0f)
    {
        dir = board2_positive_dir_by_channel(channel);
        move_angle = delta;
    }
    else
    {
        dir = board2_negative_dir_by_channel(channel);
        move_angle = -delta;
    }

    if (board2_stepper_move_angle_by_channel(channel,
                                             dir,
                                             speed_rpm,
                                             accel,
                                             move_angle) != 0U)
    {
        *current = target;
    }
}

static void usart6_digit_rx_reset_queue(void)
{
    __disable_irq();
    s_usart6_digit_q_head = 0U;
    s_usart6_digit_q_tail = 0U;
    s_usart6_digit_q_count = 0U;
    __enable_irq();
}

static uint8_t usart6_digit_rx_try_get_byte(uint8_t *out_byte)
{
    if (out_byte == 0)
    {
        return 0U;
    }

    __disable_irq();
    if (s_usart6_digit_q_count == 0U)
    {
        __enable_irq();
        return 0U;
    }

    *out_byte = s_usart6_digit_rx_queue[s_usart6_digit_q_head];
    s_usart6_digit_q_head = (uint8_t)((s_usart6_digit_q_head + 1U) % USART6_DIGIT_RX_QUEUE_SIZE);
    s_usart6_digit_q_count--;
    __enable_irq();

    return 1U;
}

static void board2_servo_move_step(u8 channel, volatile int *angle_deg, int delta_deg)
{
    int target;

    if (angle_deg == 0)
    {
        return;
    }

    target = *angle_deg + delta_deg;
    if (target < (int)TIM1_SERVO_MIN_ANGLE_DEG)
    {
        target = TIM1_SERVO_MIN_ANGLE_DEG;
    }
    else if (target > (int)TIM1_SERVO_MAX_ANGLE_DEG)
    {
        target = TIM1_SERVO_MAX_ANGLE_DEG;
    }

    /* 目标角度没有变化时不重复写PWM，减少舵机保持时抖动 */
    if (*angle_deg == target)
    {
        return;
    }

    *angle_deg = target;
    TIM1_Servo_SetAngle(channel, target);
}

static void board2_servo_set_absolute(u8 channel, uint16_t angle_deg)
{
    int target = (int)angle_deg;

    if (channel == 0U)
    {
        board2_start_action((uint8_t)angle_deg, 0U);
        g_board2_servo_cmd_count++;
        return;
    }

    if (target < (int)TIM1_SERVO_MIN_ANGLE_DEG)
    {
        target = TIM1_SERVO_MIN_ANGLE_DEG;
    }
    else if (target > (int)TIM1_SERVO_MAX_ANGLE_DEG)
    {
        target = TIM1_SERVO_MAX_ANGLE_DEG;
    }

    if (channel == TIM1_SERVO_CH1)
    {
        /* 舵机1角度没变就不刷新PWM，避免重复设置导致抖动 */
        if (g_board2_servo_ch1_angle_deg == target)
        {
            return;
        }

        g_board2_servo_ch1_angle_deg = target;
        TIM1_Servo_SetAngle(TIM1_SERVO_CH1, target);
        g_board2_servo_cmd_count++;
    }
    else if (channel == TIM1_SERVO_CH2)
    {
        /* 舵机2同样只在角度变化时刷新 */
        if (g_board2_servo_ch2_angle_deg == target)
        {
            return;
        }

        g_board2_servo_ch2_angle_deg = target;
        TIM1_Servo_SetAngle(TIM1_SERVO_CH2, target);
        g_board2_servo_cmd_count++;
    }
    else if (channel == TIM1_SERVO_CH3)
    {
        /* 舵机3：PWME输出 */
        if (g_board2_servo_ch3_angle_deg == target)
        {
            return;
        }

        g_board2_servo_ch3_angle_deg = target;
        TIM1_Servo_SetAngle(TIM1_SERVO_CH3, target);
        g_board2_servo_cmd_count++;
    }
    else if (channel == TIM1_SERVO_CH4)
    {
        /* 舵机4：PWMF输出 */
        if (g_board2_servo_ch4_angle_deg == target)
        {
            return;
        }

        g_board2_servo_ch4_angle_deg = target;
        TIM1_Servo_SetAngle(TIM1_SERVO_CH4, target);
        g_board2_servo_cmd_count++;
    }
}

static void board2_run_brush_clean_motion(void)
{
    uint8_t previous_channel_param = s_board2_remote_channel_param;

    board2_servo_set_absolute(TIM1_SERVO_CH4, BOARD2_ACTION10_SERVO4_WORK_ANGLE_DEG);
    delay_ms(BOARD2_ACTION10_SERVO_WAIT_MS);

    /* 刷毛电机与伸缩杆共用UART5，这里强制选择地址1的刷毛电机，避免误控制地址2伸缩杆 */
    s_board2_remote_channel_param = BOARD2_LINK_CHANNEL_UART5_MOTOR1;
    board2_stepper_goto_angle_by_channel(STEPPER_CH4_UART5,
                                         BOARD2_ACTION10_BRUSH_LOW_TARGET_DEG,
                                         BOARD2_AXIS_UART5_MOTOR1_SPEED_RPM,
                                         BOARD2_ACCEL_U8(BOARD2_AXIS_UART5_MOTOR1_ACCEL));
    delay_ms(BOARD2_ACTION10_BRUSH_MOVE_WAIT_MS);

    board2_stepper_goto_angle_by_channel(STEPPER_CH4_UART5,
                                         BOARD2_ACTION10_BRUSH_HOME_TARGET_DEG,
                                         BOARD2_AXIS_UART5_MOTOR1_SPEED_RPM,
                                         BOARD2_ACCEL_U8(BOARD2_AXIS_UART5_MOTOR1_ACCEL));
    delay_ms(BOARD2_ACTION10_BRUSH_MOVE_WAIT_MS);

    board2_servo_set_absolute(TIM1_SERVO_CH4, BOARD2_ACTION10_SERVO4_HOME_ANGLE_DEG);
    delay_ms(BOARD2_ACTION10_SERVO_WAIT_MS);

    s_board2_remote_channel_param = previous_channel_param;
}

void Board2_RunBrushCleanMotion(void)
{
    board2_run_brush_clean_motion();
}

void USART1_Init(int baud)
{
    NVIC_InitTypeDef nvic;
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    DMA_InitTypeDef dma;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);

    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &gpio);

    nvic.NVIC_IRQChannel = DMA2_Stream7_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2;
    nvic.NVIC_IRQChannelSubPriority = 2;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    DMA_DeInit(DMA2_Stream7);
    dma.DMA_Channel = DMA_Channel_4;
    dma.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    dma.DMA_Memory0BaseAddr = (uint32_t)s_uart1_tx_dma_buffer;
    dma.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    dma.DMA_BufferSize = UART1_DATA_LEN;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dma.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    dma.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dma.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream7, &dma);
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA2_Stream7, DISABLE);

    DMA_DeInit(DMA2_Stream5);
    dma.DMA_Channel = DMA_Channel_4;
    dma.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    dma.DMA_Memory0BaseAddr = (uint32_t)s_uart1_rx_dma_buffer;
    dma.DMA_DIR = DMA_DIR_PeripheralToMemory;
    dma.DMA_BufferSize = UART1_DATA_LEN;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Circular;
    dma.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_Init(DMA2_Stream5, &dma);
    DMA_Cmd(DMA2_Stream5, ENABLE);

    usart.USART_BaudRate = baud;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);

    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 2;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TC, DISABLE);
    USART_Cmd(USART1, ENABLE);
}

void USART6_DigitRx_Init(int baud)
{
    NVIC_InitTypeDef nvic;
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOC, &gpio);

    USART_DeInit(USART6);
    usart.USART_BaudRate = baud;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART6, &usart);

    usart6_digit_rx_reset_queue();
    g_board2_servo_uart6_rx_count = 0U;
    g_board2_servo_uart6_last_byte = 0U;
    g_board2_servo_cmd_count = 0U;
    g_board2_servo_ch1_angle_deg = TIM1_SERVO1_INIT_ANGLE_DEG;
    g_board2_servo_ch2_angle_deg = TIM1_SERVO2_INIT_ANGLE_DEG;
    g_board2_servo_ch3_angle_deg = TIM1_SERVO3_INIT_ANGLE_DEG;
    g_board2_servo_ch4_angle_deg = TIM1_SERVO4_INIT_ANGLE_DEG;

    USART_ClearFlag(USART6, USART_FLAG_ORE);
    USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART6, ENABLE);

    nvic.NVIC_IRQChannel = USART6_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2;
    nvic.NVIC_IRQChannelSubPriority = 3;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

void USART1_DMATransfer(uint32_t *buffer_src, uint32_t buffer_size)
{
    if (DMA2_Stream7->CR & DMA_SxCR_EN)
    {
        return;
    }

    DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7);
    DMA2_Stream7->NDTR = buffer_size;
    DMA2_Stream7->M0AR = (uint32_t)buffer_src;
    DMA2_Stream7->CR |= DMA_SxCR_EN;
}

void DMA2_S5_Reset(void)
{
    DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5 | DMA_FLAG_HTIF5 | DMA_FLAG_TEIF5 | DMA_FLAG_DMEIF5 | DMA_FLAG_FEIF5);
    DMA_Cmd(DMA2_Stream5, DISABLE);
    DMA2_Stream5->NDTR = UART1_DATA_LEN;
    DMA2_Stream5->M0AR = (uint32_t)s_uart1_rx_dma_buffer;
    DMA_Cmd(DMA2_Stream5, ENABLE);
}

uint8_t CalculateChecksum(uint8_t *data, uint8_t len)
{
    uint8_t checksum = 0;
    uint8_t i;

    for (i = 0; i < len; i++)
    {
        checksum ^= data[i];
    }
    return checksum;
}

static void board2_link_send_command(uint8_t cmd_type,
                                     uint8_t channel,
                                     uint8_t control,
                                     uint16_t angle_deg)
{
    s_uart1_tx_dma_buffer[0] = CMD_HEADER;
    s_uart1_tx_dma_buffer[1] = cmd_type;
    s_uart1_tx_dma_buffer[2] = channel;
    s_uart1_tx_dma_buffer[3] = control;
    s_uart1_tx_dma_buffer[4] = (uint8_t)((angle_deg >> 8) & 0xFFU);
    s_uart1_tx_dma_buffer[5] = (uint8_t)(angle_deg & 0xFFU);
    s_uart1_tx_dma_buffer[6] = CalculateChecksum(s_uart1_tx_dma_buffer,
                                                  (uint8_t)(BOARD_LINK_CMD_FRAME_LEN - 1U));

    USART1_DMATransfer((uint32_t *)s_uart1_tx_dma_buffer, BOARD_LINK_CMD_FRAME_LEN);
}

static void board2_link_send_action_status(uint8_t action_id,
                                           uint8_t status_code,
                                           uint16_t detail)
{
    if ((DMA2_Stream7->CR & DMA_SxCR_EN) != 0U)
    {
        s_board2_link_pending_status_action_id = action_id;
        s_board2_link_pending_status_code = status_code;
        s_board2_link_pending_status_detail = detail;
        s_board2_link_pending_status_valid = 1U;
        return;
    }

    board2_link_send_command(CMD_ACTION_STATUS, action_id, status_code, detail);
}

static void board2_link_flush_pending_action_status(void)
{
    uint8_t action_id;
    uint8_t status_code;
    uint16_t detail;

    if (s_board2_link_pending_status_valid == 0U)
    {
        return;
    }

    if ((DMA2_Stream7->CR & DMA_SxCR_EN) != 0U)
    {
        return;
    }

    action_id = s_board2_link_pending_status_action_id;
    status_code = s_board2_link_pending_status_code;
    detail = s_board2_link_pending_status_detail;
    s_board2_link_pending_status_valid = 0U;
    board2_link_send_command(CMD_ACTION_STATUS, action_id, status_code, detail);
}

static void board2_link_begin_action_status(uint8_t action_id, uint8_t target_c)
{
    s_board2_link_active_action_id = action_id;
    s_board2_link_active_target_c = target_c;
    s_board2_link_active_action_running = 1U;
    board2_link_send_action_status(action_id, BOARD_ACTION_STATUS_RUNNING, target_c);
}

static void board2_link_finish_action_status(uint8_t status_code)
{
    uint8_t action_id = s_board2_link_active_action_id;

    if (action_id == 0U)
    {
        return;
    }

    board2_link_send_action_status(action_id, status_code, s_board2_link_active_target_c);
    s_board2_link_active_action_id = 0U;
    s_board2_link_active_target_c = 0U;
    s_board2_link_active_action_running = 0U;
}

void Board2_LinkTaskStatusService(void)
{
    board2_link_flush_pending_action_status();

    if ((s_board2_link_active_action_running != 0U) &&
        (Board2_ActionList_IsRunning() == 0U))
    {
        uint8_t result = Board2_ActionList_GetResult();
        uint8_t status_code = BOARD_ACTION_STATUS_ERROR;

        if (result == BOARD2_ACTION_RESULT_DONE)
        {
            status_code = BOARD_ACTION_STATUS_DONE;
        }
        else if (result == BOARD2_ACTION_RESULT_STOPPED)
        {
            status_code = BOARD_ACTION_STATUS_STOPPED;
        }
        board2_link_finish_action_status(status_code);
    }

    board2_link_flush_pending_action_status();
}

void Board2_LinkRequestK230Capture(uint8_t side_id)
{
    board2_link_send_command(CMD_K230_CAPTURE, side_id, 0U, 0U);
}

void Board2_LinkSetPhotoFillLight(uint8_t enable)
{
    board2_link_send_command(CMD_PHOTO_FILL_LIGHT,
                             0U,
                             (enable != 0U) ? 1U : 0U,
                             0U);
}

static void board2_start_action(uint8_t action_id, uint8_t target_c)
{
    if ((Board2_ActionList_IsRunning() != 0U) && (action_id != 0U))
    {
        board2_link_send_action_status(action_id,
                                       BOARD_ACTION_STATUS_BUSY,
                                       target_c);
        return;
    }

    if (action_id == 0U)
    {
        board2_stop_all_remote_steppers();
        Board2_ActionList_Stop();
        board2_link_finish_action_status(BOARD_ACTION_STATUS_STOPPED);
        return;
    }

    if (action_id == 5U)
    {
        Board2_MoveTelescopeStep();
        return;
    }

    if (action_id == 6U)
    {
        if ((target_c < 1U) || (target_c > BOARD2_ACTION_C_COUNT))
        {
            board2_link_send_action_status(action_id, BOARD_ACTION_STATUS_ERROR, target_c);
            return;
        }
        Board2_ActionList_StartDefaultToC(target_c);
    }
    else if (action_id == 7U)
    {
        if ((target_c < 1U) || (target_c > BOARD2_ACTION_C_COUNT))
        {
            board2_link_send_action_status(action_id, BOARD_ACTION_STATUS_ERROR, target_c);
            return;
        }
        Board2_ActionList_StartTakeClothesFromC(target_c);
    }
    else if (action_id == 10U)
    {
        board2_link_begin_action_status(action_id, 0U);
        Board2_ActionList_Stop();
        Board2_RunBrushCleanMotion();
        board2_link_finish_action_status(BOARD_ACTION_STATUS_DONE);
        return;
    }
    else if (action_id == 11U)
    {
        if ((target_c < 1U) || (target_c > BOARD2_ACTION_C_COUNT))
        {
            board2_link_send_action_status(action_id, BOARD_ACTION_STATUS_ERROR, target_c);
            return;
        }
        Board2_ActionList_StartDefaultAfterTelescopeToC(target_c);
    }
    else if (action_id == 12U)
    {
        Board2_ActionList_StartStorePrepare();
    }
    else if (action_id == 13U)
    {
        Board2_ActionList_StartStoreRetract();
    }
    else if (action_id == 14U)
    {
        Board2_ActionList_StartFetchReset();
    }
    else
    {
        board2_link_send_action_status(action_id,
                                       BOARD_ACTION_STATUS_ERROR,
                                       target_c);
        return;
    }

    if (Board2_ActionList_GetResult() == BOARD2_ACTION_RESULT_ERROR)
    {
        board2_link_send_action_status(action_id,
                                       BOARD_ACTION_STATUS_ERROR,
                                       target_c);
        return;
    }
    board2_link_begin_action_status(action_id, g_board2_action_target_c);
}

void Board2_ProcessReceivedCommand(void)
{
    Board2CommandFrame_t command;
    StepperChannel_t channel;
    uint16_t speed_rpm;
    uint16_t angle_deg;
    int16_t target_deg;
    uint8_t accel;

    __disable_irq();
    if (s_rx_q_count == 0U)
    {
        __enable_irq();
        return;
    }

    command.header = s_rx_queue[s_rx_q_head].header;
    command.cmd_type = s_rx_queue[s_rx_q_head].cmd_type;
    command.channel = s_rx_queue[s_rx_q_head].channel;
    command.control = s_rx_queue[s_rx_q_head].control;
    command.angle_deg = s_rx_queue[s_rx_q_head].angle_deg;
    command.checksum = s_rx_queue[s_rx_q_head].checksum;
    s_rx_q_head = (uint8_t)((s_rx_q_head + 1U) % BOARD2_LINK_RX_QUEUE_LEN);
    s_rx_q_count--;
    __enable_irq();

    g_board2_link_cmd_count++;
    g_board2_link_last_cmd = command.cmd_type;
    g_board2_link_last_channel = command.channel;
    g_board2_link_last_angle = command.angle_deg;
    s_board2_remote_channel_param = command.channel;

    switch (command.cmd_type)
    {
    case CMD_STEPPER_ENABLE:
        if (board2_decode_channel_from_param(command.channel, &channel) == 0U)
        {
            break;
        }
        board2_stepper_enable_by_channel(channel, (command.control != 0U) ? true : false);
        break;

    case CMD_STEPPER_MOVE_ANGLE:
        if (board2_decode_channel_from_param(command.channel, &channel) == 0U)
        {
            break;
        }
        Board2_ActionList_Stop();
        speed_rpm = board2_fixed_speed_by_channel(channel);
        angle_deg = command.angle_deg;
        accel = board2_fixed_accel_by_channel(channel);
        board2_stepper_move_angle_by_channel(channel,
                                             board2_axis_dir_from_link_control(channel, command.control),
                                             speed_rpm,
                                             accel,
                                             (float)angle_deg);
        break;

    case CMD_STEPPER_GOTO_ANGLE:
        if (board2_decode_channel_from_param(command.channel, &channel) == 0U)
        {
            break;
        }
        Board2_ActionList_Stop();
        speed_rpm = board2_fixed_speed_by_channel(channel);
        target_deg = (int16_t)command.angle_deg;
        accel = board2_fixed_accel_by_channel(channel);
        g_board2_link_last_target = target_deg;
        board2_stepper_goto_angle_by_channel(channel,
                                             target_deg,
                                             speed_rpm,
                                             accel);
        break;

    case CMD_SERVO_SET_ANGLE:
        if ((command.channel != 0U) && (command.channel != TIM1_SERVO_CH3))
        {
            Board2_ActionList_Stop();
        }
        board2_servo_set_absolute(command.channel, command.angle_deg);
        break;

    case CMD_ACTION_START:
        board2_start_action(command.channel, command.control);
        break;

    case CMD_EMERGENCY:
        board2_stop_all_remote_steppers();
        Board2_ActionList_Stop();
        break;

    default:
        break;
    }
}

void Board2_ProcessServoUart6Command(void)
{
    uint8_t data;

    while (usart6_digit_rx_try_get_byte(&data) != 0U)
    {
        if ((Board2_ActionList_IsRunning() != 0U) &&
            (data != '0') &&
            (data != 0x00))
        {
            continue;
        }

        switch (data)
        {
        case '0':
        case 0x00:
            board2_stop_all_remote_steppers();
            Board2_ActionList_Stop();
            g_board2_servo_cmd_count++;
            break;

        case '1':
        case 0x01:
            board2_servo_move_step(TIM1_SERVO_CH1,
                                   &g_board2_servo_ch1_angle_deg,
                                   (int)TIM1_SERVO_STEP_ANGLE_DEG);
            g_board2_servo_cmd_count++;
            break;

        case '2':
        case 0x02:
            board2_servo_move_step(TIM1_SERVO_CH1,
                                   &g_board2_servo_ch1_angle_deg,
                                   -(int)TIM1_SERVO_STEP_ANGLE_DEG);
            g_board2_servo_cmd_count++;
            break;

        case '3':
        case 0x03:
            board2_servo_move_step(TIM1_SERVO_CH2,
                                   &g_board2_servo_ch2_angle_deg,
                                   (int)TIM1_SERVO_STEP_ANGLE_DEG);
            g_board2_servo_cmd_count++;
            break;

        case '4':
        case 0x04:
            board2_servo_move_step(TIM1_SERVO_CH2,
                                   &g_board2_servo_ch2_angle_deg,
                                   -(int)TIM1_SERVO_STEP_ANGLE_DEG);
            g_board2_servo_cmd_count++;
            break;

        case '5':
        case 0x05:
            Board2_MoveTelescopeStep();
            break;

        case '6':
        case 0x06:
            if (BOARD2_LEGACY_DIRECT_ACTION_ENABLE != 0U)
            {
                Board2_ActionList_StartDefault();
                g_board2_servo_cmd_count++;
            }
            break;

        case '7':
        case 0x07:
            if (BOARD2_LEGACY_DIRECT_ACTION_ENABLE != 0U)
            {
                Board2_ActionList_StartTakeClothes();
                g_board2_servo_cmd_count++;
            }
            break;

        default:
            break;
        }
    }
}

void USART1_IRQHandler(void)
{
    uint8_t rec_len;

    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        USART_ReceiveData(USART1);
        rec_len = (uint8_t)(UART1_DATA_LEN - DMA2_Stream5->NDTR);

        if ((rec_len == BOARD_LINK_CMD_FRAME_LEN) && (s_uart1_rx_dma_buffer[0] == CMD_HEADER))
        {
            if (CalculateChecksum(s_uart1_rx_dma_buffer, (uint8_t)(rec_len - 1U)) == s_uart1_rx_dma_buffer[rec_len - 1U])
            {
                if (s_rx_q_count < BOARD2_LINK_RX_QUEUE_LEN)
                {
                    s_rx_queue[s_rx_q_tail].header = s_uart1_rx_dma_buffer[0];
                    s_rx_queue[s_rx_q_tail].cmd_type = s_uart1_rx_dma_buffer[1];
                    s_rx_queue[s_rx_q_tail].channel = s_uart1_rx_dma_buffer[2];
                    s_rx_queue[s_rx_q_tail].control = s_uart1_rx_dma_buffer[3];
                    s_rx_queue[s_rx_q_tail].angle_deg = (uint16_t)(((uint16_t)s_uart1_rx_dma_buffer[4] << 8) |
                                                                    s_uart1_rx_dma_buffer[5]);
                    s_rx_queue[s_rx_q_tail].checksum = s_uart1_rx_dma_buffer[6];
                    s_rx_q_tail = (uint8_t)((s_rx_q_tail + 1U) % BOARD2_LINK_RX_QUEUE_LEN);
                    s_rx_q_count++;
                    g_board2_link_rx_count++;
                }
            }
        }

        DMA2_S5_Reset();
    }
    else if (USART_GetITStatus(USART1, USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_TC);
        USART_ITConfig(USART1, USART_IT_TC, DISABLE);
        DMA2_Stream7->CR &= (uint32_t)(~DMA_SxCR_EN);
    }
}

void USART6_IRQHandler(void)
{
    uint8_t data;

    if (USART_GetITStatus(USART6, USART_IT_RXNE) != RESET)
    {
        data = (uint8_t)USART_ReceiveData(USART6);
        g_board2_servo_uart6_last_byte = data;
        g_board2_servo_uart6_rx_count++;
        if (s_usart6_digit_q_count < USART6_DIGIT_RX_QUEUE_SIZE)
        {
            s_usart6_digit_rx_queue[s_usart6_digit_q_tail] = data;
            s_usart6_digit_q_tail = (uint8_t)((s_usart6_digit_q_tail + 1U) % USART6_DIGIT_RX_QUEUE_SIZE);
            s_usart6_digit_q_count++;
        }
    }

    if (USART_GetFlagStatus(USART6, USART_FLAG_ORE) != RESET)
    {
        (void)USART_ReceiveData(USART6);
    }
}

void DMA2_Stream7_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) != RESET)
    {
        DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7);
        DMA_Cmd(DMA2_Stream7, DISABLE);
        USART_ITConfig(USART1, USART_IT_TC, ENABLE);
    }
}
