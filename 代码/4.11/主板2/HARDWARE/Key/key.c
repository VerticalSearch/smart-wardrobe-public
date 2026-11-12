#include "key.h"
#include "delay.h"

void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

u8 KEY_Scan(u8 mode)
{
    static u8 key_up = 1;

    if (mode)
    {
        key_up = 1;
    }

    if (key_up && ((KEY1 == 0U) || (KEY2 == 0U) || (KEY3 == 0U) || (KEY4 == 0U)))
    {
        delay_ms(10);
        key_up = 0;

        if (KEY1 == 0U)
        {
            return KEY1_PRES;
        }
        else if (KEY2 == 0U)
        {
            return KEY2_PRES;
        }
        else if (KEY3 == 0U)
        {
            return KEY3_PRES;
        }
        else if (KEY4 == 0U)
        {
            return KEY4_PRES;
        }
    }
    else if ((KEY1 != 0U) && (KEY2 != 0U) && (KEY3 != 0U) && (KEY4 != 0U))
    {
        key_up = 1;
    }

    return 0;
}
