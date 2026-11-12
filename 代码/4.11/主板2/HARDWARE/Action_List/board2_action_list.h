#ifndef __BOARD2_ACTION_LIST_H
#define __BOARD2_ACTION_LIST_H

#include "sys.h"
#include <stdint.h>

/* 1U：上电自动执行完整流程；0U：上电不动，等待USART6收到命令6后再启动 */
#define BOARD2_ACTION_LIST_AUTOSTART       0U

#define BOARD2_ACTION_POINT_NONE           0xFFU
#define BOARD2_ACTION_GROUP_NONE           0xFFU
#define BOARD2_ACTION_SERVO_KEEP           (-1)
#define BOARD2_ACTION_RESULT_IDLE          0U
#define BOARD2_ACTION_RESULT_RUNNING       1U
#define BOARD2_ACTION_RESULT_DONE          2U
#define BOARD2_ACTION_RESULT_STOPPED       3U
#define BOARD2_ACTION_RESULT_ERROR         4U

typedef enum
{
    BOARD2_ACTION_GROUP_GRAB = 0,
    BOARD2_ACTION_GROUP_PLACE,
    BOARD2_ACTION_GROUP_C_GRAB,
    BOARD2_ACTION_GROUP_A_PLACE,
    BOARD2_ACTION_GROUP_COUNT
} Board2ActionGroupId_t;

typedef enum
{
    BOARD2_ACTION_WORKFLOW_HOME_TO_A0 = 0,
    BOARD2_ACTION_WORKFLOW_TELESCOPE_EXTEND,
    BOARD2_ACTION_WORKFLOW_HANG_WAIT,
    BOARD2_ACTION_WORKFLOW_TELESCOPE_RETRACT,
    BOARD2_ACTION_WORKFLOW_Z_TO_A0,
    BOARD2_ACTION_WORKFLOW_OPEN_SERVO2,
    BOARD2_ACTION_WORKFLOW_MOVE_TO_A,
    BOARD2_ACTION_WORKFLOW_KEEP_SERVO1_AT_A,
    BOARD2_ACTION_WORKFLOW_GRAB,
    BOARD2_ACTION_WORKFLOW_MOVE_Y_AFTER_GRAB_1,
    BOARD2_ACTION_WORKFLOW_MOVE_X_AFTER_GRAB,
    BOARD2_ACTION_WORKFLOW_MOVE_TO_B,
    BOARD2_ACTION_WORKFLOW_MOVE_TO_B0,
    BOARD2_ACTION_WORKFLOW_CAPTURE_FRONT,
    BOARD2_ACTION_WORKFLOW_Z_DOWN_FOR_BACK,
    BOARD2_ACTION_WORKFLOW_ROTATE_BACK,
    BOARD2_ACTION_WORKFLOW_CAPTURE_BACK,
    BOARD2_ACTION_WORKFLOW_Z_UP_AFTER_BACK,
    BOARD2_ACTION_WORKFLOW_MOVE_TO_C_X_PRE,
    BOARD2_ACTION_WORKFLOW_MOVE_TO_C_PRE,
    BOARD2_ACTION_WORKFLOW_MOVE_TO_C,
    BOARD2_ACTION_WORKFLOW_PLACE,
    BOARD2_ACTION_WORKFLOW_NEXT_ITEM,
    BOARD2_ACTION_WORKFLOW_RETURN_A00,
    BOARD2_ACTION_WORKFLOW_TAKE_HOME_TO_A0,
    BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01_X_PRE,
    BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01_PRE,
    BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_C01,
    BOARD2_ACTION_WORKFLOW_TAKE_GRAB_FROM_C,
    BOARD2_ACTION_WORKFLOW_TAKE_EXIT_C01_PRE,
    BOARD2_ACTION_WORKFLOW_TAKE_EXIT_C01_X_PRE,
    BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_B3,
    BOARD2_ACTION_WORKFLOW_TAKE_Z_DOWN_AT_B3,
    BOARD2_ACTION_WORKFLOW_TAKE_BRUSH_FRONT_AT_B3,
    BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_B4,
    BOARD2_ACTION_WORKFLOW_TAKE_ROTATE_SERVO1_AT_B4,
    BOARD2_ACTION_WORKFLOW_TAKE_MOVE_BACK_TO_B3,
    BOARD2_ACTION_WORKFLOW_TAKE_BRUSH_BACK_AT_B3,
    BOARD2_ACTION_WORKFLOW_TAKE_Z_UP_AFTER_BRUSH,
    BOARD2_ACTION_WORKFLOW_TAKE_MOVE_TO_A01,
    BOARD2_ACTION_WORKFLOW_TAKE_PLACE_TO_A,
    BOARD2_ACTION_WORKFLOW_TAKE_RETURN_A0,
    BOARD2_ACTION_WORKFLOW_TAKE_TELESCOPE_EXTEND,
    BOARD2_ACTION_WORKFLOW_FETCH_RESET_TELESCOPE_HOME,
    BOARD2_ACTION_WORKFLOW_FETCH_RESET_HOME_TO_A0,
    BOARD2_ACTION_WORKFLOW_FETCH_RESET_SERVOS,
    BOARD2_ACTION_WORKFLOW_END
} Board2ActionWorkflowState_t;

typedef struct
{
    const char *name;
    float x_deg;
    float y_deg;
    float z_deg;
} Board2ActionPoint_t;

typedef struct
{
    const char *name;
    float z_target_deg;
    int servo1_deg;
    int servo2_deg;
    uint32_t wait_ms;
} Board2ActionGroupStep_t;

extern volatile uint8_t g_board2_action_running;
extern volatile uint8_t g_board2_action_current_step;
extern volatile uint8_t g_board2_action_current_group;
extern volatile uint8_t g_board2_action_current_group_step;
extern volatile uint8_t g_board2_action_last_point;
extern volatile uint8_t g_board2_action_next_c_index;
extern volatile uint8_t g_board2_action_target_c;
extern volatile uint8_t g_board2_action_result;

void Board2_ActionList_Init(void);
void Board2_ActionList_Task(void);
void Board2_ActionList_StartDefault(void);
void Board2_ActionList_StartDefaultToC(uint8_t target_c);
void Board2_ActionList_StartStorePrepare(void);
void Board2_ActionList_StartStoreRetract(void);
void Board2_ActionList_StartFetchReset(void);
void Board2_ActionList_StartDefaultAfterTelescope(void);
void Board2_ActionList_StartDefaultAfterTelescopeToC(uint8_t target_c);
void Board2_ActionList_StartTakeClothes(void);
void Board2_ActionList_StartTakeClothesFromC(uint8_t target_c);
void Board2_ActionList_Stop(void);
uint8_t Board2_ActionList_IsRunning(void);
uint8_t Board2_ActionList_GetResult(void);

#endif
