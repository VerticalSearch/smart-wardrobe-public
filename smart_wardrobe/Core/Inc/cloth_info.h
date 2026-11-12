/**
  ******************************************************************************
  * @file    cloth_info.h
  * @brief   衣物信息数据结构定义
  ******************************************************************************
  */

#ifndef __CLOTH_INFO_H
#define __CLOTH_INFO_H

#include <stdint.h>

/* 衣物类别定义 */
typedef enum {
    CLOTH_CATEGORY_TOP = 0,        // 上衣
    CLOTH_CATEGORY_BOTTOM,         // 下装
    CLOTH_CATEGORY_OUTERWEAR,      // 外套
    CLOTH_CATEGORY_DRESS,          // 连衣裙
    CLOTH_CATEGORY_SHOES,          // 鞋子
    CLOTH_CATEGORY_ACCESSORY,      // 配饰
    CLOTH_CATEGORY_MAX
} ClothCategory_t;

/* 颜色定义 */
typedef enum {
    COLOR_BLACK = 0,
    COLOR_WHITE,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_GRAY,
    COLOR_BROWN,
    COLOR_PINK,
    COLOR_PURPLE,
    COLOR_MAX
} Color_t;

/* 季节定义 */
typedef enum {
    SEASON_SPRING = 0x01,
    SEASON_SUMMER = 0x02,
    SEASON_AUTUMN = 0x04,
    SEASON_WINTER = 0x08,
    SEASON_ALL = 0x0F
} Season_t;

/* 衣物信息结构体 */
typedef struct {
    uint8_t uid[10];           // RFID标签UID
    ClothCategory_t category;   // 类别
    Color_t color;              // 颜色
    uint8_t season;             // 季节(位掩码)
    uint32_t last_use_time;     // 最后使用时间戳
    uint16_t use_count;         // 使用次数
    uint8_t position;           // 位置(0-255, 对应RFID读卡器编号)
    uint8_t reserved[3];        // 保留字段
} ClothInfo_t;

/* 搭配方案结构体 */
typedef struct {
    ClothInfo_t *top;           // 上衣
    ClothInfo_t *bottom;         // 下装
    ClothInfo_t *outerwear;     // 外套(可选)
    ClothInfo_t *shoes;         // 鞋子(可选)
    uint8_t score;              // 搭配评分(0-100)
} Outfit_t;

/* 函数声明 */
void ClothInfo_Init(void);
uint8_t ClothInfo_Add(ClothInfo_t *cloth);
uint8_t ClothInfo_Remove(uint8_t *uid);
ClothInfo_t* ClothInfo_Find(uint8_t *uid);
uint8_t ClothInfo_GetCount(void);
void ClothInfo_UpdateUseTime(uint8_t *uid);
Outfit_t* Outfit_Recommend(uint8_t season, uint8_t occasion);

#endif /* __CLOTH_INFO_H */
