/**
  ******************************************************************************
  * @file    rc522.h
  * @brief   MFRC522 RFID读卡器驱动头文件
  ******************************************************************************
  */

#ifndef __RC522_H
#define __RC522_H

#include <stdint.h>

/* RC522寄存器地址定义 */
#define RC522_REG_COMMAND        0x01
#define RC522_REG_COMIRQ         0x04
#define RC522_REG_DIVIRQ         0x05
#define RC522_REG_ERROR          0x06
#define RC522_REG_STATUS1        0x07
#define RC522_REG_STATUS2        0x08
#define RC522_REG_FIFO_DATA      0x09
#define RC522_REG_FIFO_LEVEL     0x0A
#define RC522_REG_CONTROL        0x0C
#define RC522_REG_BIT_FRAMING    0x0D
#define RC522_REG_MODE           0x11
#define RC522_REG_TX_CONTROL     0x14
#define RC522_REG_TX_AUTO        0x15
#define RC522_REG_TX_SEL         0x16
#define RC522_REG_RX_SEL         0x17
#define RC522_REG_RX_THRESHOLD   0x18
#define RC522_REG_MIFARE         0x1C
#define RC522_REG_TX_MODE        0x1D
#define RC522_REG_RX_MODE        0x1E
#define RC522_REG_PIN_CONTROL    0x1F
#define RC522_REG_TIMER_CONTROL  0x21
#define RC522_REG_CRC_RESULT     0x22
#define RC522_REG_T_MODE         0x2A
#define RC522_REG_T_PRESCALER    0x2B
#define RC522_REG_T_RELOAD_L     0x2C
#define RC522_REG_T_RELOAD_H     0x2D
#define RC522_REG_VERSION        0x37

/* RC522命令 */
#define RC522_CMD_IDLE           0x00
#define RC522_CMD_MEM            0x01
#define RC522_CMD_GENERATE_RANDOM_ID 0x02
#define RC522_CMD_CALC_CRC       0x03
#define RC522_CMD_TRANSMIT       0x04
#define RC522_CMD_NO_CMD_CHANGE  0x07
#define RC522_CMD_RECEIVE        0x08
#define RC522_CMD_TRANSCEIVE     0x0C
#define RC522_CMD_MF_AUTHENT     0x0E
#define RC522_CMD_SOFT_RESET     0x0F

/* 卡片类型 */
#define RC522_CARD_TYPE_MIFARE_1K    0x08
#define RC522_CARD_TYPE_MIFARE_4K    0x18
#define RC522_CARD_TYPE_MIFARE_UL    0x00
#define RC522_CARD_TYPE_NTAG213      0x00
#define RC522_CARD_TYPE_NTAG215      0x00
#define RC522_CARD_TYPE_NTAG216      0x00

/* 函数返回值 */
#define RC522_OK              0x00
#define RC522_ERROR           0x01
#define RC522_NO_CARD         0x02
#define RC522_TIMEOUT         0x03

/* RFID卡片信息结构体 */
typedef struct {
    uint8_t uid[10];       // 卡片UID
    uint8_t uid_len;       // UID长度
    uint8_t sak;           // SAK值
    uint8_t atqa[2];       // ATQA值
} RFID_Card_t;

/* 函数声明 */
uint8_t RC522_Init(void);
uint8_t RC522_Request(uint8_t req_mode, uint8_t *atq);
uint8_t RC522_Anticoll(uint8_t *ser_num);
uint8_t RC522_Select(uint8_t *ser_num, uint8_t len);
uint8_t RC522_ReadCard(uint8_t *uid, uint8_t *uid_len);
uint8_t RC522_WriteBlock(uint8_t block_addr, uint8_t *data);
uint8_t RC522_ReadBlock(uint8_t block_addr, uint8_t *data);
uint8_t RC522_Halt(void);
void RC522_Reset(void);

/* 底层SPI接口函数(需要用户实现) */
uint8_t RC522_SPI_Write(uint8_t reg, uint8_t value);
uint8_t RC522_SPI_Read(uint8_t reg);
void RC522_CS_Low(void);
void RC522_CS_High(void);
void RC522_RST_Low(void);
void RC522_RST_High(void);
void RC522_Delay(uint32_t ms);

#endif /* __RC522_H */
