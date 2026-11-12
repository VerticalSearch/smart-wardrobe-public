#include "stepper_multi.h"
#include "delay.h"

#define STEPPER_STEPS_PER_REVOLUTION 3200U
#define STEPPER_MAX_SPEED 10000U
#define STEPPER_MAX_ACCELERATION 255U
#define STEPPER_RX_QUEUE_SIZE 64U
#define STEPPER_STATUS_REACHED_MASK 0x02U
#define STEPPER_STATUS_POLL_MS 50U
#define STEPPER_STATUS_READ_TIMEOUT_MS 80U

typedef struct
{
    volatile uint8_t data[STEPPER_RX_QUEUE_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t count;
} StepperRxQueue_t;

static StepperRxQueue_t s_stepper_rx[4];

volatile uint32_t g_stepper_multi_rx_count = 0U;
volatile uint32_t g_stepper_multi_rx_overflow_count = 0U;
volatile uint32_t g_stepper_multi_status_ok_count = 0U;
volatile uint32_t g_stepper_multi_status_timeout_count = 0U;
volatile uint32_t g_stepper_multi_wait_timeout_count = 0U;
volatile uint8_t g_stepper_multi_last_channel = 0U;
volatile uint8_t g_stepper_multi_last_addr = 0U;
volatile uint8_t g_stepper_multi_last_status = 0U;

static USART_TypeDef *stepper_get_usart(StepperChannel_t ch)
{
    switch (ch)
    {
    case STEPPER_CH1_UART2: return USART2;
    case STEPPER_CH2_UART3: return USART3;
    case STEPPER_CH3_UART4: return UART4;
    case STEPPER_CH4_UART5: return UART5;
    case STEPPER_CH5_UART6: return USART6;
    default: return USART2;
    }
}

static uint8_t stepper_channel_has_rx(StepperChannel_t ch)
{
    return (ch < STEPPER_CH5_UART6) ? 1U : 0U;
}

static StepperRxQueue_t *stepper_get_rx_queue(StepperChannel_t ch)
{
    if (stepper_channel_has_rx(ch) == 0U)
    {
        return 0;
    }

    return &s_stepper_rx[(uint8_t)ch];
}

static IRQn_Type stepper_get_irqn(StepperChannel_t ch)
{
    switch (ch)
    {
    case STEPPER_CH1_UART2: return USART2_IRQn;
    case STEPPER_CH2_UART3: return USART3_IRQn;
    case STEPPER_CH3_UART4: return UART4_IRQn;
    case STEPPER_CH4_UART5: return UART5_IRQn;
    default: return USART2_IRQn;
    }
}

static void stepper_rx_push(StepperChannel_t ch, uint8_t data)
{
    StepperRxQueue_t *queue = stepper_get_rx_queue(ch);

    if (queue == 0)
    {
        return;
    }

    if (queue->count >= STEPPER_RX_QUEUE_SIZE)
    {
        queue->tail = (uint8_t)((queue->tail + 1U) % STEPPER_RX_QUEUE_SIZE);
        queue->count--;
        g_stepper_multi_rx_overflow_count++;
    }

    queue->data[queue->head] = data;
    queue->head = (uint8_t)((queue->head + 1U) % STEPPER_RX_QUEUE_SIZE);
    queue->count++;
    g_stepper_multi_rx_count++;
}

static uint8_t stepper_rx_pop(StepperChannel_t ch, uint8_t *data)
{
    StepperRxQueue_t *queue = stepper_get_rx_queue(ch);

    if ((queue == 0) || (data == 0) || (queue->count == 0U))
    {
        return 0U;
    }

    __disable_irq();
    if (queue->count == 0U)
    {
        __enable_irq();
        return 0U;
    }

    *data = queue->data[queue->tail];
    queue->tail = (uint8_t)((queue->tail + 1U) % STEPPER_RX_QUEUE_SIZE);
    queue->count--;
    __enable_irq();
    return 1U;
}

static void stepper_rx_irq_handler(StepperChannel_t ch)
{
    USART_TypeDef *usart = stepper_get_usart(ch);
    uint8_t data;

    if (USART_GetITStatus(usart, USART_IT_RXNE) != RESET)
    {
        data = (uint8_t)(USART_ReceiveData(usart) & 0xFFU);
        stepper_rx_push(ch, data);
        USART_ClearITPendingBit(usart, USART_IT_RXNE);
    }

    if (USART_GetFlagStatus(usart, USART_FLAG_ORE) != RESET)
    {
        (void)USART_ReceiveData(usart);
    }
}

static uint8_t stepper_get_opposite_dir(uint8_t dir)
{
    return (dir == 0U) ? 1U : 0U;
}

static void stepper_send_bytes(StepperChannel_t ch, const uint8_t *data, uint8_t len)
{
    USART_TypeDef *usart;
    uint8_t i;

    if (ch == STEPPER_CH5_UART6)
    {
        return;
    }

    usart = stepper_get_usart(ch);

    /* Keep TX timing aligned with existing stepper implementation */
    while (USART_GetFlagStatus(usart, USART_FLAG_TXE) == RESET) {}

    for (i = 0; i < len; i++)
    {
        USART_SendData(usart, data[i]);
        while (USART_GetFlagStatus(usart, USART_FLAG_TXE) == RESET) {}
    }
    while (USART_GetFlagStatus(usart, USART_FLAG_TC) == RESET) {}

    /* Leave a small gap between command frames */
    delay_ms(10);
}

static void stepper_uart_rx_irq_init(StepperChannel_t ch)
{
    NVIC_InitTypeDef nvic;

    if (stepper_channel_has_rx(ch) == 0U)
    {
        return;
    }

    nvic.NVIC_IRQChannel = stepper_get_irqn(ch);
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = (uint8_t)((uint8_t)ch & 0x03U);
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    USART_ITConfig(stepper_get_usart(ch), USART_IT_RXNE, ENABLE);
}

static void stepper_uart_init(StepperChannel_t ch, uint32_t baud)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;

    switch (ch)
    {
    case STEPPER_CH1_UART2:
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2);
        GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2);
        gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
        GPIO_Init(GPIOD, &gpio);
        USART_DeInit(USART2);
        break;
    case STEPPER_CH2_UART3:
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
        GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);
        gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
        GPIO_Init(GPIOD, &gpio);
        USART_DeInit(USART3);
        break;
    case STEPPER_CH3_UART4:
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
        GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_UART4);
        GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_UART4);
        gpio.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
        GPIO_Init(GPIOC, &gpio);
        USART_DeInit(UART4);
        break;
    case STEPPER_CH4_UART5:
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
        GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_UART5);
        GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_UART5);
        gpio.GPIO_Pin = GPIO_Pin_12;
        GPIO_Init(GPIOC, &gpio);
        gpio.GPIO_Pin = GPIO_Pin_2;
        GPIO_Init(GPIOD, &gpio);
        USART_DeInit(UART5);
        break;
    case STEPPER_CH5_UART6:
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
        GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
        GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);
        gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
        GPIO_Init(GPIOC, &gpio);
        USART_DeInit(USART6);
        break;
    default:
        return;
    }

    usart.USART_BaudRate = baud;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(stepper_get_usart(ch), &usart);
    USART_Cmd(stepper_get_usart(ch), ENABLE);
    stepper_uart_rx_irq_init(ch);
}

void stepper_multi_init_all(uint32_t baud)
{
    stepper_uart_init(STEPPER_CH1_UART2, baud);
    stepper_uart_init(STEPPER_CH2_UART3, baud);
    stepper_uart_init(STEPPER_CH3_UART4, baud);
    stepper_uart_init(STEPPER_CH4_UART5, baud);
}

void stepper_multi_enable(StepperChannel_t ch, uint8_t addr, bool en)
{
    uint8_t cmd[6];

    cmd[0] = addr;
    cmd[1] = 0xF3;
    cmd[2] = 0xAB;
    cmd[3] = en ? 1 : 0;
    cmd[4] = 0;
    cmd[5] = 0x6B;

    stepper_send_bytes(ch, cmd, 6);
}

void stepper_multi_move_angle(StepperChannel_t ch, uint8_t addr, uint8_t dir, uint16_t speed, uint8_t acc, float angle)
{
    uint8_t cmd[13];
    uint32_t steps;

    if (speed > STEPPER_MAX_SPEED)
    {
        speed = STEPPER_MAX_SPEED;
    }
    if (acc > STEPPER_MAX_ACCELERATION)
    {
        acc = STEPPER_MAX_ACCELERATION;
    }

    if (angle < 0.0f)
    {
        angle = 0.0f;
    }
    steps = (uint32_t)((angle / 360.0f) * (float)STEPPER_STEPS_PER_REVOLUTION);

    stepper_multi_clear_rx(ch);

    cmd[0] = addr;
    cmd[1] = 0xFD;
    cmd[2] = dir;
    cmd[3] = (uint8_t)(speed >> 8);
    cmd[4] = (uint8_t)(speed & 0xFF);
    cmd[5] = acc;
    cmd[6] = (uint8_t)(steps >> 24);
    cmd[7] = (uint8_t)(steps >> 16);
    cmd[8] = (uint8_t)(steps >> 8);
    cmd[9] = (uint8_t)(steps & 0xFF);
    cmd[10] = 0;
    cmd[11] = 0;
    cmd[12] = 0x6B;

    stepper_send_bytes(ch, cmd, 13);
}

void stepper_multi_enable_dual_opposite_uart2(uint8_t addr_id1, uint8_t addr_id2, bool en)
{
    /* UART2 dual motor: send enable/disable to both IDs */
    stepper_multi_enable(STEPPER_CH1_UART2, addr_id1, en);
    stepper_multi_enable(STEPPER_CH1_UART2, addr_id2, en);
}

void stepper_multi_move_angle_dual_opposite_uart2(uint8_t addr_id1,
                                                   uint8_t addr_id2,
                                                   uint8_t dir_id1,
                                                   uint16_t speed,
                                                   uint8_t acc,
                                                   float angle)
{
    uint8_t dir_id2;

    /* UART2 dual motor: ID2 always uses opposite direction */
    dir_id2 = stepper_get_opposite_dir(dir_id1);
    stepper_multi_move_angle(STEPPER_CH1_UART2, addr_id1, dir_id1, speed, acc, angle);
    stepper_multi_move_angle(STEPPER_CH1_UART2, addr_id2, dir_id2, speed, acc, angle);
}

void stepper_multi_clear_rx(StepperChannel_t ch)
{
    StepperRxQueue_t *queue = stepper_get_rx_queue(ch);

    if (queue == 0)
    {
        return;
    }

    __disable_irq();
    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;
    __enable_irq();
}

uint8_t stepper_multi_read_status(StepperChannel_t ch, uint8_t addr, uint8_t *status, uint16_t timeout_ms)
{
    uint8_t cmd[3];
    uint8_t rx;
    uint8_t state = 0U;
    uint16_t elapsed = 0U;

    if ((status == 0) || (stepper_channel_has_rx(ch) == 0U))
    {
        return 0U;
    }

    *status = 0U;
    cmd[0] = addr;
    cmd[1] = 0x3A;
    cmd[2] = 0x6B;
    stepper_send_bytes(ch, cmd, 3U);

    while (elapsed < timeout_ms)
    {
        while (stepper_rx_pop(ch, &rx) != 0U)
        {
            if (state == 0U)
            {
                if (rx == addr)
                {
                    state = 1U;
                }
            }
            else if (state == 1U)
            {
                if ((rx == 0x3A) || (rx == 0xFD))
                {
                    state = 2U;
                }
                else
                {
                    *status = rx;
                    state = 3U;
                }
            }
            else if (state == 2U)
            {
                *status = rx;
                state = 3U;
            }
            else
            {
                if (rx == 0x6B)
                {
                    g_stepper_multi_status_ok_count++;
                    g_stepper_multi_last_channel = (uint8_t)ch;
                    g_stepper_multi_last_addr = addr;
                    g_stepper_multi_last_status = *status;
                    return 1U;
                }

                state = (rx == addr) ? 1U : 0U;
            }
        }

        delay_ms(1U);
        elapsed++;
    }

    g_stepper_multi_status_timeout_count++;
    g_stepper_multi_last_channel = (uint8_t)ch;
    g_stepper_multi_last_addr = addr;
    return 0U;
}

uint8_t stepper_multi_wait_reached(StepperChannel_t ch, uint8_t addr, uint32_t timeout_ms)
{
    uint32_t elapsed = 0U;
    uint8_t status = 0U;

    while (elapsed < timeout_ms)
    {
        if (stepper_multi_read_status(ch, addr, &status, STEPPER_STATUS_READ_TIMEOUT_MS) != 0U)
        {
            if ((status & STEPPER_STATUS_REACHED_MASK) != 0U)
            {
                return 1U;
            }
        }

        delay_ms(STEPPER_STATUS_POLL_MS);
        elapsed += STEPPER_STATUS_POLL_MS;
    }

    g_stepper_multi_wait_timeout_count++;
    return 0U;
}

uint8_t stepper_multi_wait_reached_dual_opposite_uart2(uint8_t addr_id1, uint8_t addr_id2, uint32_t timeout_ms)
{
    uint32_t elapsed = 0U;
    uint8_t status1 = 0U;
    uint8_t status2 = 0U;
    uint8_t reached1 = 0U;
    uint8_t reached2 = 0U;

    while (elapsed < timeout_ms)
    {
        if (stepper_multi_read_status(STEPPER_CH1_UART2, addr_id1, &status1, STEPPER_STATUS_READ_TIMEOUT_MS) != 0U)
        {
            reached1 = ((status1 & STEPPER_STATUS_REACHED_MASK) != 0U) ? 1U : 0U;
        }

        if (stepper_multi_read_status(STEPPER_CH1_UART2, addr_id2, &status2, STEPPER_STATUS_READ_TIMEOUT_MS) != 0U)
        {
            reached2 = ((status2 & STEPPER_STATUS_REACHED_MASK) != 0U) ? 1U : 0U;
        }

        if ((reached1 != 0U) && (reached2 != 0U))
        {
            return 1U;
        }

        delay_ms(STEPPER_STATUS_POLL_MS);
        elapsed += STEPPER_STATUS_POLL_MS;
    }

    g_stepper_multi_wait_timeout_count++;
    return 0U;
}

void USART2_IRQHandler(void)
{
    stepper_rx_irq_handler(STEPPER_CH1_UART2);
}

void USART3_IRQHandler(void)
{
    stepper_rx_irq_handler(STEPPER_CH2_UART3);
}

void UART4_IRQHandler(void)
{
    stepper_rx_irq_handler(STEPPER_CH3_UART4);
}

void UART5_IRQHandler(void)
{
    stepper_rx_irq_handler(STEPPER_CH4_UART5);
}
