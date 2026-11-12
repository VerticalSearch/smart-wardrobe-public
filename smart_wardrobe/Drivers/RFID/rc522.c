/**
  ******************************************************************************
  * @file    rc522.c
  * @brief   MFRC522 RFID读卡器驱动实现
  ******************************************************************************
  */

#include "rc522.h"

/* 私有函数声明 */
static uint8_t RC522_WriteReg(uint8_t reg, uint8_t value);
static uint8_t RC522_ReadReg(uint8_t reg);
static void RC522_SetBitMask(uint8_t reg, uint8_t mask);
static void RC522_ClearBitMask(uint8_t reg, uint8_t mask);
static void RC522_AntennaOn(void);
static void RC522_AntennaOff(void);
static uint8_t RC522_CalculateCRC(uint8_t *data, uint8_t len, uint8_t *result);

/**
  * @brief  初始化RC522模块
  * @retval RC522_OK: 成功, 其他: 失败
  */
uint8_t RC522_Init(void)
{
    RC522_RST_High();
    RC522_Delay(10);
    RC522_RST_Low();
    RC522_Delay(10);
    RC522_RST_High();
    RC522_Delay(10);
    
    // 软复位
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    RC522_Delay(10);
    
    // 检查版本
    uint8_t version = RC522_ReadReg(RC522_REG_VERSION);
    if (version == 0x00 || version == 0xFF) {
        return RC522_ERROR;
    }
    
    // 配置定时器
    RC522_WriteReg(RC522_REG_T_MODE, 0x8D);
    RC522_WriteReg(RC522_REG_T_PRESCALER, 0x3E);
    RC522_WriteReg(RC522_REG_T_RELOAD_L, 0x1E);
    RC522_WriteReg(RC522_REG_T_RELOAD_H, 0x00);
    
    // 配置接收增益
    RC522_WriteReg(RC522_REG_RX_Gain, 0x96);
    
    // 配置MIFARE
    RC522_WriteReg(RC522_REG_MIFARE, 0x00);
    
    // 清中断标志
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
    RC522_WriteReg(RC522_REG_COMIRQ, 0x7F);
    
    // 开启天线
    RC522_AntennaOn();
    
    return RC522_OK;
}

/**
  * @brief  寻卡
  * @param  req_mode: 寻卡模式
  * @param  atq: 返回的ATQ值
  * @retval RC522_OK: 成功, 其他: 失败
  */
uint8_t RC522_Request(uint8_t req_mode, uint8_t *atq)
{
    uint8_t status;
    
    RC522_WriteReg(RC522_REG_BIT_FRAMING, 0x07);
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    RC522_WriteReg(RC522_REG_FIFO_DATA, req_mode);
    
    RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    uint16_t i = 2000;
    do {
        status = RC522_ReadReg(RC522_REG_COMIRQ);
        i--;
    } while ((i != 0) && !(status & 0x30));
    
    RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    if (i != 0 && !(status & 0x01)) {
        if (RC522_ReadReg(RC522_REG_FIFO_LEVEL) == 2) {
            atq[0] = RC522_ReadReg(RC522_REG_FIFO_DATA);
            atq[1] = RC522_ReadReg(RC522_REG_FIFO_DATA);
        }
        if (RC522_ReadReg(RC522_REG_ERROR) & 0x08) {
            return RC522_ERROR;
        }
        return RC522_OK;
    }
    
    return RC522_ERROR;
}

/**
  * @brief  防冲突
  * @param  ser_num: 返回的卡片序列号
  * @retval RC522_OK: 成功, 其他: 失败
  */
uint8_t RC522_Anticoll(uint8_t *ser_num)
{
    uint8_t status;
    uint8_t i;
    uint8_t ser_num_check = 0;
    
    RC522_WriteReg(RC522_REG_BIT_FRAMING, 0x00);
    RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80);
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    RC522_WriteReg(RC522_REG_FIFO_DATA, 0x93);
    RC522_WriteReg(RC522_REG_FIFO_DATA, 0x20);
    
    RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    uint16_t i_timeout = 2000;
    do {
        status = RC522_ReadReg(RC522_REG_COMIRQ);
        i_timeout--;
    } while ((i_timeout != 0) && !(status & 0x30));
    
    RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    if (i_timeout != 0 && !(status & 0x01)) {
        if (RC522_ReadReg(RC522_REG_FIFO_LEVEL) == 5) {
            for (i = 0; i < 4; i++) {
                ser_num[i] = RC522_ReadReg(RC522_REG_FIFO_DATA);
                ser_num_check ^= ser_num[i];
            }
            if (ser_num_check != RC522_ReadReg(RC522_REG_FIFO_DATA)) {
                return RC522_ERROR;
            }
        }
        return RC522_OK;
    }
    
    return RC522_ERROR;
}

/**
  * @brief  读取卡片UID
  * @param  uid: 返回的UID缓冲区
  * @param  uid_len: 返回的UID长度
  * @retval RC522_OK: 成功, 其他: 失败
  */
uint8_t RC522_ReadCard(uint8_t *uid, uint8_t *uid_len)
{
    uint8_t atq[2];
    
    // 寻卡
    if (RC522_Request(0x26, atq) != RC522_OK) {
        return RC522_NO_CARD;
    }
    
    // 防冲突
    if (RC522_Anticoll(uid) != RC522_OK) {
        return RC522_ERROR;
    }
    
    // 选择卡片
    if (RC522_Select(uid, 4) != RC522_OK) {
        return RC522_ERROR;
    }
    
    *uid_len = 4;
    return RC522_OK;
}

/**
  * @brief  选择卡片
  * @param  ser_num: 卡片序列号
  * @param  len: 序列号长度
  * @retval RC522_OK: 成功, 其他: 失败
  */
uint8_t RC522_Select(uint8_t *ser_num, uint8_t len)
{
    uint8_t i;
    uint8_t status;
    uint8_t size;
    uint8_t ser_num_check = 0;
    uint8_t buffer[9];
    
    buffer[0] = 0x93;
    buffer[1] = 0x70 + len;
    
    for (i = 0; i < len; i++) {
        buffer[i + 2] = ser_num[i];
        ser_num_check ^= ser_num[i];
    }
    
    buffer[len + 2] = ser_num_check;
    
    RC522_CalculateCRC(buffer, len + 3, &buffer[len + 3]);
    
    RC522_WriteReg(RC522_REG_BIT_FRAMING, 0x00);
    RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80);
    
    for (i = 0; i < len + 5; i++) {
        RC522_WriteReg(RC522_REG_FIFO_DATA, buffer[i]);
    }
    
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    uint16_t i_timeout = 2000;
    do {
        status = RC522_ReadReg(RC522_REG_COMIRQ);
        i_timeout--;
    } while ((i_timeout != 0) && !(status & 0x30));
    
    RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    if (i_timeout != 0 && !(status & 0x01)) {
        size = RC522_ReadReg(RC522_REG_FIFO_LEVEL);
        if (size == 0x18) {
            return RC522_OK;
        }
    }
    
    return RC522_ERROR;
}

/**
  * @brief  读取数据块
  * @param  block_addr: 块地址
  * @param  data: 读取的数据缓冲区
  * @retval RC522_OK: 成功, 其他: 失败
  */
uint8_t RC522_ReadBlock(uint8_t block_addr, uint8_t *data)
{
    uint8_t status;
    uint8_t i;
    uint8_t buffer[18];
    
    buffer[0] = 0x30;
    buffer[1] = block_addr;
    RC522_CalculateCRC(buffer, 2, &buffer[2]);
    
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
    RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80);
    
    for (i = 0; i < 4; i++) {
        RC522_WriteReg(RC522_REG_FIFO_DATA, buffer[i]);
    }
    
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    uint16_t i_timeout = 2000;
    do {
        status = RC522_ReadReg(RC522_REG_COMIRQ);
        i_timeout--;
    } while ((i_timeout != 0) && !(status & 0x30));
    
    RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    if (i_timeout != 0 && !(status & 0x01)) {
        if (RC522_ReadReg(RC522_REG_FIFO_LEVEL) == 16) {
            for (i = 0; i < 16; i++) {
                data[i] = RC522_ReadReg(RC522_REG_FIFO_DATA);
            }
            return RC522_OK;
        }
    }
    
    return RC522_ERROR;
}

/**
  * @brief  写入数据块
  * @param  block_addr: 块地址
  * @param  data: 要写入的数据
  * @retval RC522_OK: 成功, 其他: 失败
  */
uint8_t RC522_WriteBlock(uint8_t block_addr, uint8_t *data)
{
    uint8_t status;
    uint8_t i;
    uint8_t buffer[18];
    
    buffer[0] = 0xA0;
    buffer[1] = block_addr;
    RC522_CalculateCRC(buffer, 2, &buffer[2]);
    
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
    RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80);
    
    for (i = 0; i < 4; i++) {
        RC522_WriteReg(RC522_REG_FIFO_DATA, buffer[i]);
    }
    
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    uint16_t i_timeout = 2000;
    do {
        status = RC522_ReadReg(RC522_REG_COMIRQ);
        i_timeout--;
    } while ((i_timeout != 0) && !(status & 0x30));
    
    RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    if (i_timeout != 0 && !(status & 0x01)) {
        // 写入数据
        RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
        RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80);
        
        for (i = 0; i < 16; i++) {
            RC522_WriteReg(RC522_REG_FIFO_DATA, data[i]);
        }
        
        buffer[0] = 0xA0;
        buffer[1] = block_addr;
        RC522_CalculateCRC(buffer, 2, &buffer[2]);
        
        for (i = 0; i < 4; i++) {
            RC522_WriteReg(RC522_REG_FIFO_DATA, buffer[i]);
        }
        
        RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
        RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);
        
        i_timeout = 2000;
        do {
            status = RC522_ReadReg(RC522_REG_COMIRQ);
            i_timeout--;
        } while ((i_timeout != 0) && !(status & 0x30));
        
        RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);
        
        if (i_timeout != 0 && !(status & 0x01)) {
            return RC522_OK;
        }
    }
    
    return RC522_ERROR;
}

/**
  * @brief  卡片休眠
  * @retval RC522_OK: 成功
  */
uint8_t RC522_Halt(void)
{
    uint8_t buffer[4];
    
    buffer[0] = 0x50;
    buffer[1] = 0x00;
    RC522_CalculateCRC(buffer, 2, &buffer[2]);
    
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80);
    
    for (uint8_t i = 0; i < 4; i++) {
        RC522_WriteReg(RC522_REG_FIFO_DATA, buffer[i]);
    }
    
    RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80);
    
    return RC522_OK;
}

/**
  * @brief  复位RC522
  */
void RC522_Reset(void)
{
    RC522_RST_Low();
    RC522_Delay(10);
    RC522_RST_High();
    RC522_Delay(10);
}

/* ========== 私有函数实现 ========== */

static uint8_t RC522_WriteReg(uint8_t reg, uint8_t value)
{
    RC522_CS_Low();
    RC522_SPI_Write(reg, value);
    RC522_CS_High();
    return RC522_OK;
}

static uint8_t RC522_ReadReg(uint8_t reg)
{
    uint8_t value;
    RC522_CS_Low();
    value = RC522_SPI_Read(reg);
    RC522_CS_High();
    return value;
}

static void RC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = RC522_ReadReg(reg);
    RC522_WriteReg(reg, tmp | mask);
}

static void RC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = RC522_ReadReg(reg);
    RC522_WriteReg(reg, tmp & (~mask));
}

static void RC522_AntennaOn(void)
{
    uint8_t temp = RC522_ReadReg(RC522_REG_TX_CONTROL);
    if (!(temp & 0x03)) {
        RC522_SetBitMask(RC522_REG_TX_CONTROL, 0x03);
    }
}

static void RC522_AntennaOff(void)
{
    RC522_ClearBitMask(RC522_REG_TX_CONTROL, 0x03);
}

static uint8_t RC522_CalculateCRC(uint8_t *data, uint8_t len, uint8_t *result)
{
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE);
    RC522_WriteReg(RC522_REG_DIVIRQ, 0x04);
    RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80);
    
    for (uint8_t i = 0; i < len; i++) {
        RC522_WriteReg(RC522_REG_FIFO_DATA, data[i]);
    }
    
    RC522_WriteReg(RC522_REG_COMMAND, RC522_CMD_CALC_CRC);
    
    uint16_t i_timeout = 2000;
    uint8_t n;
    do {
        n = RC522_ReadReg(RC522_REG_DIVIRQ);
        i_timeout--;
    } while ((i_timeout != 0) && !(n & 0x04));
    
    result[0] = RC522_ReadReg(RC522_REG_CRC_RESULT);
    result[1] = RC522_ReadReg(RC522_REG_CRC_RESULT + 1);
    
    return RC522_OK;
}
