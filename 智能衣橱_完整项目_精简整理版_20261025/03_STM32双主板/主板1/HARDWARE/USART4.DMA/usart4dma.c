#include "usart4dma.h"
#include "Stepper.h"
#include "usartdma.h"
#include "delay.h"

/* Board1 UART4 is used by the hot-air mover stepper motor.
 * UART4_TX -> PC10, UART4_RX -> PC11.
 */

volatile uint32_t g_board1_hot_air_stepper_tx_count = 0U;
volatile uint32_t g_board1_hot_air_stepper_rx_count = 0U;
volatile uint8_t g_board1_hot_air_stepper_last_rx = 0U;
volatile uint8_t g_board1_hot_air_stepper_last_cmd = 0U;
volatile uint8_t g_board1_hot_air_stepper_last_dir = 0U;
volatile uint64_t g_board1_hot_air_stepper_last_angle = 0U;
volatile int64_t g_board1_hot_air_stepper_target_deg = 0;
volatile int64_t g_board1_hot_air_stepper_position_deg = 0;

#define BOARD1_HOT_AIR_RX_BUF_LEN 32U

static volatile uint8_t s_hot_air_rx_buf[BOARD1_HOT_AIR_RX_BUF_LEN];
static volatile uint8_t s_hot_air_rx_index = 0U;

#define BOARD1_HOT_AIR_MAX_STEP_CHUNK_DEG \
    ((uint32_t)((4294967295ULL * 360ULL) / (uint64_t)STEPPER_STEPS_PER_REVOLUTION))

static uint64_t board1_hot_air_abs_i64(int64_t value)
{
    if (value >= 0)
    {
        return (uint64_t)value;
    }
    if (value == (-9223372036854775807LL - 1LL))
    {
        return 9223372036854775808ULL;
    }
    return (uint64_t)(-value);
}

static void board1_hot_air_add_position(uint8_t dir, uint64_t angle_deg)
{
    if (dir == BOARD1_HOT_AIR_DIR_REVERSE)
    {
        g_board1_hot_air_stepper_position_deg -= (int64_t)angle_deg;
    }
    else
    {
        g_board1_hot_air_stepper_position_deg += (int64_t)angle_deg;
    }
}

static void board1_hot_air_send_move_angle(uint8_t dir, uint64_t angle_deg)
{
    uint32_t chunk_deg;
    uint32_t steps;

    while (angle_deg > 0U)
    {
        chunk_deg = (angle_deg > BOARD1_HOT_AIR_MAX_STEP_CHUNK_DEG) ?
                    BOARD1_HOT_AIR_MAX_STEP_CHUNK_DEG :
                    angle_deg;
        steps = (uint32_t)(((uint64_t)chunk_deg *
                            (uint64_t)STEPPER_STEPS_PER_REVOLUTION) / 360ULL);

        stepper_move_steps_on_uart(UART4,
                                   BOARD1_HOT_AIR_STEPPER_ADDR,
                                   dir,
                                   BOARD1_HOT_AIR_STEPPER_SPEED_RPM,
                                   BOARD1_HOT_AIR_STEPPER_ACCEL,
                                   steps);

        angle_deg -= chunk_deg;
    }
}

void uart4_init(uint32_t bound)
{
    Board1_HotAirStepper_Init(bound);
}

void Board1_HotAirStepper_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_UART4);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_UART4);

    gpio.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio);

    usart.USART_BaudRate = baudrate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UART4, &usart);

    nvic.NVIC_IRQChannel = UART4_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
    USART_Cmd(UART4, ENABLE);

    g_board1_hot_air_stepper_tx_count = 0U;
    g_board1_hot_air_stepper_rx_count = 0U;
    g_board1_hot_air_stepper_last_rx = 0U;
    g_board1_hot_air_stepper_last_cmd = 0U;
    g_board1_hot_air_stepper_last_dir = 0U;
    g_board1_hot_air_stepper_last_angle = 0U;
    g_board1_hot_air_stepper_target_deg = 0;
    g_board1_hot_air_stepper_position_deg = 0;
    s_hot_air_rx_index = 0U;
}

void Board1_HotAirStepper_Enable(uint8_t enable)
{
    stepper_enable_on_uart(UART4,
                           BOARD1_HOT_AIR_STEPPER_ADDR,
                           (enable != 0U) ? true : false);
    g_board1_hot_air_stepper_last_cmd = CMD_STEPPER_ENABLE;
    g_board1_hot_air_stepper_tx_count++;
    delay_ms(5);
}

void Board1_HotAirStepper_Stop(void)
{
    stepper_stop_on_uart(UART4, BOARD1_HOT_AIR_STEPPER_ADDR);
    g_board1_hot_air_stepper_last_cmd = 0U;
    g_board1_hot_air_stepper_tx_count++;
}

void Board1_HotAirStepper_MoveAngle(uint8_t dir, uint64_t angle_deg)
{
    board1_hot_air_send_move_angle(dir, angle_deg);
    board1_hot_air_add_position(dir, angle_deg);
    g_board1_hot_air_stepper_target_deg = g_board1_hot_air_stepper_position_deg;
    g_board1_hot_air_stepper_last_cmd = CMD_STEPPER_MOVE_ANGLE;
    g_board1_hot_air_stepper_last_dir = dir;
    g_board1_hot_air_stepper_last_angle = angle_deg;
    g_board1_hot_air_stepper_tx_count++;
}

void Board1_HotAirStepper_GotoAngle(int64_t target_deg)
{
    uint64_t move_deg;

    if (target_deg > g_board1_hot_air_stepper_position_deg)
    {
        move_deg = (uint64_t)(target_deg - g_board1_hot_air_stepper_position_deg);
        Board1_HotAirStepper_MoveAngle(BOARD1_HOT_AIR_DIR_FORWARD, move_deg);
    }
    else if (target_deg < g_board1_hot_air_stepper_position_deg)
    {
        if ((target_deg < 0) && (g_board1_hot_air_stepper_position_deg > 0))
        {
            move_deg = board1_hot_air_abs_i64(target_deg) +
                       (uint64_t)g_board1_hot_air_stepper_position_deg;
        }
        else
        {
            move_deg = (uint64_t)(g_board1_hot_air_stepper_position_deg - target_deg);
        }
        Board1_HotAirStepper_MoveAngle(BOARD1_HOT_AIR_DIR_REVERSE, move_deg);
    }

    g_board1_hot_air_stepper_target_deg = target_deg;
    g_board1_hot_air_stepper_position_deg = target_deg;
    g_board1_hot_air_stepper_last_cmd = CMD_STEPPER_GOTO_ANGLE;
}

void UART4_IRQHandler(void)
{
    if (USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)
    {
        uint8_t byte = (uint8_t)USART_ReceiveData(UART4);

        USART_ClearITPendingBit(UART4, USART_IT_RXNE);
        g_board1_hot_air_stepper_last_rx = byte;
        s_hot_air_rx_buf[s_hot_air_rx_index] = byte;
        s_hot_air_rx_index = (uint8_t)((s_hot_air_rx_index + 1U) % BOARD1_HOT_AIR_RX_BUF_LEN);
        g_board1_hot_air_stepper_rx_count++;
    }
}
