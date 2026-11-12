#ifndef __BOARD1_HX711_H
#define __BOARD1_HX711_H

#include "stm32f4xx.h"

#define BOARD1_HX711_LEFT   0U
#define BOARD1_HX711_RIGHT  1U

typedef struct
{
    int32_t raw;
    int32_t filtered;
    uint8_t valid;
    uint8_t ready;
    uint8_t read_error;
    uint8_t reserved;
} Board1_HX711_Data_t;

extern volatile Board1_HX711_Data_t g_board1_hx711_data[2];

void Board1_HX711_Init(void);
uint8_t Board1_HX711_IsReady(uint8_t index);
uint8_t Board1_HX711_ReadRaw(uint8_t index, int32_t *value);
uint8_t Board1_HX711_Update(uint8_t index);
const volatile Board1_HX711_Data_t *Board1_HX711_GetData(uint8_t index);

#endif
