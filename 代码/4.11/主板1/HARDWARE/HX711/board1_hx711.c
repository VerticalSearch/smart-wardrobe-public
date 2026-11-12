#include "board1_hx711.h"

#define BOARD1_HX711_READY_TIMEOUT  100000UL
#define BOARD1_HX711_FILTER_SHIFT   3

/* S1-S4 are on GPIOA according to the main-board-1 schematic. */
#define BOARD1_HX7111_SCK_PORT  GPIOA
#define BOARD1_HX7111_SCK_PIN   GPIO_Pin_0  /* S1, U3.23, PA0 */
#define BOARD1_HX7111_DT_PORT   GPIOA
#define BOARD1_HX7111_DT_PIN    GPIO_Pin_1  /* S2, U3.24, PA1 */

#define BOARD1_HX7112_SCK_PORT  GPIOA
#define BOARD1_HX7112_SCK_PIN   GPIO_Pin_2  /* S3, U3.25, PA2 */
#define BOARD1_HX7112_DT_PORT   GPIOA
#define BOARD1_HX7112_DT_PIN    GPIO_Pin_3  /* S4, U3.26, PA3 */

typedef struct
{
    GPIO_TypeDef *sck_port;
    uint16_t sck_pin;
    GPIO_TypeDef *dt_port;
    uint16_t dt_pin;
} Board1_HX711_PinMap_t;

static const Board1_HX711_PinMap_t g_board1_hx711_pins[2] =
{
    {BOARD1_HX7111_SCK_PORT, BOARD1_HX7111_SCK_PIN, BOARD1_HX7111_DT_PORT, BOARD1_HX7111_DT_PIN},
    {BOARD1_HX7112_SCK_PORT, BOARD1_HX7112_SCK_PIN, BOARD1_HX7112_DT_PORT, BOARD1_HX7112_DT_PIN},
};

volatile Board1_HX711_Data_t g_board1_hx711_data[2];

static void Board1_HX711_Delay(void)
{
    volatile uint8_t i;

    for (i = 0U; i < 12U; i++)
    {
        __NOP();
    }
}

static uint8_t Board1_HX711_IsValidIndex(uint8_t index)
{
    return (index < 2U) ? 1U : 0U;
}

void Board1_HX711_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_ResetBits(GPIOA, BOARD1_HX7111_SCK_PIN | BOARD1_HX7112_SCK_PIN);

    gpio.GPIO_Pin = BOARD1_HX7111_SCK_PIN | BOARD1_HX7112_SCK_PIN;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = BOARD1_HX7111_DT_PIN | BOARD1_HX7112_DT_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio);
}

uint8_t Board1_HX711_IsReady(uint8_t index)
{
    const Board1_HX711_PinMap_t *pins;

    if (Board1_HX711_IsValidIndex(index) == 0U)
    {
        return 0U;
    }

    pins = &g_board1_hx711_pins[index];
    return (GPIO_ReadInputDataBit(pins->dt_port, pins->dt_pin) == Bit_RESET) ? 1U : 0U;
}

uint8_t Board1_HX711_ReadRaw(uint8_t index, int32_t *value)
{
    const Board1_HX711_PinMap_t *pins;
    uint32_t timeout = BOARD1_HX711_READY_TIMEOUT;
    uint32_t data = 0UL;
    uint8_t i;

    if ((value == 0) || (Board1_HX711_IsValidIndex(index) == 0U))
    {
        return 0U;
    }

    pins = &g_board1_hx711_pins[index];

    GPIO_ResetBits(pins->sck_port, pins->sck_pin);
    while (GPIO_ReadInputDataBit(pins->dt_port, pins->dt_pin) != Bit_RESET)
    {
        if (timeout == 0UL)
        {
            return 0U;
        }
        timeout--;
    }

    for (i = 0U; i < 24U; i++)
    {
        GPIO_SetBits(pins->sck_port, pins->sck_pin);
        Board1_HX711_Delay();
        data <<= 1;
        GPIO_ResetBits(pins->sck_port, pins->sck_pin);
        Board1_HX711_Delay();
        if (GPIO_ReadInputDataBit(pins->dt_port, pins->dt_pin) != Bit_RESET)
        {
            data |= 1UL;
        }
    }

    /* 25th pulse: select HX711 channel A, gain 128 for next conversion. */
    GPIO_SetBits(pins->sck_port, pins->sck_pin);
    Board1_HX711_Delay();
    GPIO_ResetBits(pins->sck_port, pins->sck_pin);
    Board1_HX711_Delay();

    if ((data & 0x800000UL) != 0UL)
    {
        data |= 0xFF000000UL;
    }

    *value = (int32_t)data;
    return 1U;
}

uint8_t Board1_HX711_Update(uint8_t index)
{
    int32_t raw;
    int32_t delta;
    volatile Board1_HX711_Data_t *data;

    if (Board1_HX711_IsValidIndex(index) == 0U)
    {
        return 0U;
    }

    data = &g_board1_hx711_data[index];
    data->ready = Board1_HX711_IsReady(index);
    data->read_error = 0U;

    if (data->ready == 0U)
    {
        return 0U;
    }

    if (Board1_HX711_ReadRaw(index, &raw) == 0U)
    {
        data->read_error = 1U;
        return 0U;
    }

    data->raw = raw;
    if (data->valid == 0U)
    {
        data->filtered = raw;
        data->valid = 1U;
    }
    else
    {
        delta = raw - data->filtered;
        data->filtered += (delta / (1 << BOARD1_HX711_FILTER_SHIFT));
    }

    return 1U;
}

const volatile Board1_HX711_Data_t *Board1_HX711_GetData(uint8_t index)
{
    if (Board1_HX711_IsValidIndex(index) == 0U)
    {
        return 0;
    }

    return &g_board1_hx711_data[index];
}
