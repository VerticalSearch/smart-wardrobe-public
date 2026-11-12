#ifndef __BOARD2_ACTION_TUNING_H
#define __BOARD2_ACTION_TUNING_H

/*
 * 主控2动作流程调参入口。
 *
 * 调坐标、等待时间、前置偏移、舵机角度、速度/闭环参数时，优先只改本文件。
 * X/Y/Z 点位和动作组里的 Z 值都是“软件累计角度目标值”，也就是 move_axis_to() 的绝对目标。
 * 伸缩杆参数是相对运动量，正负方向沿用 control/usartdma 里的电机方向定义。
 */

/*
 * 点位表：改坐标只改这里。
 * 写法：_(内部ID, 屏幕/调试显示名, X当前值, Y当前值, Z当前值)
 * 单位：X/Y/Z 都是步进电机软件累计角度。
 */
#define BOARD2_ACTION_POINT_TABLE(_) \
    _(A0,  "A0",     0.0f,    0.0f, 3200.0f) \
    _(A00, "A00",    0.0f,    0.0f,    0.0f) \
    _(A01, "A01", 1220.0f, 2040.0f, 3200.0f) \
    _(A02, "A02", 1220.0f, 1380.0f, 3200.0f) \
    _(A03, "A03", 1220.0f, 760.0f, 3200.0f) \
    _(A04, "A04", 1220.0f,  120.0f, 3200.0f) \
    _(B,   "B",   2600.0f, 2500.0f, 3200.0f) \
    _(B0,  "B0",  1600.0f, 3000.0f, 3200.0f) \
    _(B3,  "B3",  1000.0f, 7200.0f, 3200.0f) \
    _(B4,  "B4",  1000.0f, 4800.0f, 3200.0f) \
    _(C01, "C01", 6150.0f, 8550.0f, 3200.0f) \
    _(C02, "C02", 6150.0f, 7810.0f, 3200.0f) \
    _(C03, "C03", 6150.0f, 7050.0f, 3200.0f) \
    _(C04, "C04", 6150.0f, 6300.0f, 3200.0f) \
    _(C05, "C05", 6150.0f, 5530.0f, 3200.0f) \
    _(C06, "C06", 6150.0f, 4810.0f, 3200.0f) \
    _(C07, "C07", 6150.0f, 4070.0f, 3200.0f) \
    _(C08, "C08", 6150.0f, 3320.0f, 3200.0f) \
    _(C09, "C09", 6170.0f, 2550.0f, 3200.0f) \
    _(C10, "C10", 6170.0f, 1810.0f, 3200.0f) \
    _(C11, "C11", 6170.0f, 1070.0f, 3200.0f) \
    _(C12, "C12", 6170.0f,  240.0f, 3200.0f)

/* 点位循环范围：控制自动流程取哪些 A 点、放到哪些 C 点。 */
#define BOARD2_ACTION_A_FIRST_POINT          BOARD2_ACTION_POINT_A01  /* 当前值：A01，放衣流程第一个 A 区抓取点。 */
#define BOARD2_ACTION_A_COUNT                4U                       /* 当前值：4 个点，按 A01~A04 依次处理。 */
#define BOARD2_ACTION_C_FIRST_POINT          BOARD2_ACTION_POINT_C01  /* 当前值：C01，放衣流程第一个 C 区放置点。 */
#define BOARD2_ACTION_C_COUNT                12U                      /* 当前值：12 个点，按 C01~C12 使用；要用到 C13 就改成 13。 */

/* 舵机目标角度，单位：度。 */
#define BOARD2_ACTION_SERVO2_OPEN_DEG        170                      /* 当前值：120 度，舵机2张开/释放衣服。 */
#define BOARD2_ACTION_SERVO2_CLOSE_DEG       115                       /* 当前值：45 度，舵机2夹紧衣服。 */
#define BOARD2_ACTION_SERVO1_FRONT_DEG       190                      /* 当前值：188 度，舵机1正面姿态，流程启动默认使用。 */
#define BOARD2_ACTION_SERVO1_BACK_DEG        15                       /* 当前值：10 度，舵机1背面姿态，到 B/B0 并降到允许高度后切换。 */
#define BOARD2_ACTION_STORE_SERVO4_DEG       TIM1_SERVO4_INIT_ANGLE_DEG /* 当前值：0 度，数字6存衣流程启动时刷毛舵机4保持当前初始姿态。 */
#define BOARD2_ACTION_BRUSH_SERVO4_HOME_DEG  150U                     /* 当前值：150 度，刷毛动作结束后舵机4回位姿态。 */
#define BOARD2_ACTION_TAKE_SERVO4_DEG        BOARD2_ACTION_BRUSH_SERVO4_HOME_DEG /* 当前值：150 度，数字7取衣流程启动时刷毛舵机4转到刷毛结束姿态。 */

/*
 * 抓取/放置动作组的 Z 目标，单位：软件累计角度。
 * 注意：这里是 Z 轴绝对目标，不是相对上升/下降量。
 */
#define BOARD2_ACTION_GRAB_Z_UP_DEG          3200.0f                  /* 当前值：Z=3200，A 区抓取时的抬起高度。 */
#define BOARD2_ACTION_GRAB_Z_DOWN_DEG        -600.0f                  /* 当前值：Z=-800，A 区抓取时的下降夹取高度。 */
#define BOARD2_ACTION_PLACE_Z_DOWN_DEG       0.0f                     /* 当前值：Z=0，C 区放置时的下降释放高度。 */
#define BOARD2_ACTION_PLACE_Z_UP_DEG         3200.0f                  /* 当前值：Z=3200，C 区放置完成后的抬起高度。 */
#define BOARD2_ACTION_C_GRAB_Z_UP_DEG        3200.0f                  /* 当前值：Z=3200，取衣流程从 C 区抓取前的抬起高度。 */
#define BOARD2_ACTION_C_GRAB_Z_DOWN_DEG      -800.0f                  /* 当前值：Z=-800，取衣流程在 C 区下降夹取高度。 */
#define BOARD2_ACTION_A_PLACE_Z_DOWN_DEG     -600.0f                  /* 当前值：Z=-600，取衣流程回 A 区放置前的第一次下降高度。 */
#define BOARD2_ACTION_A_RELEASE_Z_DEG        -800.0f                  /* 当前值：Z=-800，取衣流程回 A 区松开衣服时的高度。 */
#define BOARD2_ACTION_A_PLACE_Z_UP_DEG       3200.0f                  /* 当前值：Z=3200，取衣流程回 A 区放置完成后的抬起高度。 */
#define BOARD2_ACTION_TAKE_BRUSH_Z_DEG       0.0f                     /* 当前值：Z=0，取衣流程在 B3/B4 刷毛和翻面时的低位高度。 */
#define BOARD2_ACTION_TAKE_BRUSH_Z_UP_DEG    3200.0f                  /* 当前值：Z=3200，双面刷毛完成后去 A 区前的抬起高度。 */

/* 流程运动参数，单位：步进电机软件累计角度或相对角度，按注释区分。 */
#define BOARD2_ACTION_AFTER_GRAB_X_OFFSET_DEG 900.0f                  /* 当前值：900，A 区抓完后 X 轴相对离开 A 点的距离。 */
#define BOARD2_ACTION_AFTER_GRAB_Y_OFFSET1_DEG 320.0f                 /* 当前值：320，A 区夹取并抬升后，暂存衣杆避让点的 Y+ 相对移动距离。 */
#define BOARD2_ACTION_SERVO1_ROTATE_Z_DEG    0.0f                     /* 当前值：Z=0，允许舵机1翻面的 Z 轴基准高度。 */
#define BOARD2_ACTION_B_ROTATE_Z_TOLERANCE_DEG 100.0f                 /* 当前值：100，舵机1翻面允许的 Z 轴高度误差范围。 */
#define BOARD2_ACTION_B_ROTATE_Z_TARGET_DEG  (BOARD2_ACTION_SERVO1_ROTATE_Z_DEG + BOARD2_ACTION_B_ROTATE_Z_TOLERANCE_DEG) /* 当前计算值：Z=100，B 点翻面前实际下降目标。 */
#define BOARD2_ACTION_B_SAFE_Z_DEG           3200.0f                  /* 当前值：Z=2600，B 点翻面拍照后 Z 轴升回的安全高度。 */
#define BOARD2_ACTION_A_PRE_X_OFFSET_DEG     300.0f                   /* 当前值：300，进入 A 点前第一前置点的 X 轴正向偏移距离。 */
#define BOARD2_ACTION_A_PRE_Y_OFFSET_DEG     320.0f                   /* 当前值：320，进入 A 点前两个前置点共同保持的 Y 轴正向偏移。 */
#define BOARD2_ACTION_C_PRE_X_OFFSET_DEG     900.0f                   /* 当前值：900，进入 C 点前第一个前置点的 X 轴负向偏移距离。 */
#define BOARD2_ACTION_C_PRE_Y_OFFSET_DEG     400.0f                   /* 当前值：400，进入 C 点前两个前置点共同保持的 Y 轴正向偏移。 */
#define BOARD2_ACTION_TELESCOPE_EXTEND_DEG   -15500.0f                /* 当前值：-15500，伸缩杆伸出行程，负数表示按当前定义的反方向走。 */
#define BOARD2_ACTION_TELESCOPE_RETRACT_DEG   15500.0f                /* 当前值：15500，伸缩杆缩回行程，正数表示按当前定义的正方向走。 */

/* 抓取/放置动作组等待时间，单位：ms。数值越大，动作间停顿越久。 */
#define BOARD2_ACTION_WAIT_GRAB_OPEN_MS                 30U           /* 当前值：30ms，A 区抓取前舵机2张开后的等待。 */
#define BOARD2_ACTION_WAIT_GRAB_DOWN_MS               5000U           /* 当前值：5000ms，A 区 Z 轴下降到夹取高度后的等待。 */
#define BOARD2_ACTION_WAIT_GRAB_CLOSE_MS              3000U           /* 当前值：3000ms，A 区舵机2夹紧后的等待。 */
#define BOARD2_ACTION_WAIT_GRAB_UP_MS                 4000U           /* 当前值：3000ms，A 区夹紧后 Z 轴抬起的等待。 */
#define BOARD2_ACTION_WAIT_PLACE_DOWN_MS              3000U           /* 当前值：3000ms，C 区 Z 轴下降到放置高度后的等待。 */
#define BOARD2_ACTION_WAIT_PLACE_RELEASE_MS            400U           /* 当前值：400ms，C 区舵机2释放后的等待。 */
#define BOARD2_ACTION_WAIT_PLACE_UP_MS                3000U           /* 当前值：3000ms，C 区释放后 Z 轴抬起的等待。 */
#define BOARD2_ACTION_WAIT_C_GRAB_OPEN_MS               30U           /* 当前值：30ms，取衣流程 C 区抓取前舵机2张开后的等待。 */
#define BOARD2_ACTION_WAIT_C_GRAB_DOWN_MS             3000U           /* 当前值：3000ms，取衣流程 C 区下降到夹取高度后的等待。 */
#define BOARD2_ACTION_WAIT_C_GRAB_CLOSE_MS             400U           /* 当前值：400ms，取衣流程 C 区舵机2夹紧后的等待。 */
#define BOARD2_ACTION_WAIT_C_GRAB_UP_MS               4000U           /* 当前值：4000ms，取衣流程 C 区夹紧后 Z 轴抬起的等待。 */
#define BOARD2_ACTION_WAIT_A_PLACE_DOWN_MS            5000U           /* 当前值：5000ms，取衣流程回 A 区下降到放置高度后的等待。 */
#define BOARD2_ACTION_WAIT_A_PLACE_RELEASE_MS          400U           /* 当前值：400ms，取衣流程回 A 区舵机2释放后的等待。 */
#define BOARD2_ACTION_WAIT_A_PLACE_UP_MS              3000U           /* 当前值：3000ms，取衣流程回 A 区释放后 Z 轴抬起的等待。 */

/* 放衣流程状态等待时间，单位：ms。 */
#define BOARD2_ACTION_WAIT_AFTER_HOME_TO_A0_MS         1000U          /* 当前值：1000ms，启动后回到 A0 的稳定等待。 */
#define BOARD2_ACTION_WAIT_AFTER_TELESCOPE_EXTEND_MS   6000U          /* 当前值：6000ms，伸缩杆伸出后的等待。 */
#define BOARD2_ACTION_WAIT_HANG_MS                     5000U          /* 当前值：5000ms，挂衣动作/衣架稳定等待。 */
#define BOARD2_ACTION_WAIT_AFTER_TELESCOPE_RETRACT_MS  3000U          /* 当前值：3000ms，伸缩杆缩回后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_Z_TO_A0_MS            2000U          /* 当前值：2000ms，每轮回到 A0 高度后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_OPEN_SERVO2_MS        1000U          /* 当前值：1000ms，进入 A 区前舵机2张开后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_MOVE_TO_A_MS           500U          /* 当前值：500ms，移动到当前 A 点后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_KEEP_SERVO1_AT_A_MS    500U          /* 当前值：500ms，A 点保持舵机1姿态后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_MOVE_Y_AFTER_GRAB1_MS   500U          /* 当前值：500ms，A 区抓完抬升后 Y+ 避让点等待。 */
#define BOARD2_ACTION_WAIT_AFTER_MOVE_X_AFTER_GRAB_MS   500U          /* 当前值：500ms，A 区抓完后 X 轴离开 A 点的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_MOVE_TO_B_MS          1000U          /* 当前值：1000ms，移动到 B 点后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_MOVE_TO_B0_MS         1000U          /* 当前值：1000ms，移动到 B0 点后的等待。 */
#define BOARD2_ACTION_WAIT_CAPTURE_FRONT_MS            8000U          /* 当前值：8000ms，正面拍照等待 K230 识别/上传。 */
#define BOARD2_ACTION_WAIT_AFTER_Z_DOWN_FOR_BACK_MS    3000U          /* 当前值：3000ms，B 点为背面翻转下降 Z 后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_ROTATE_BACK_MS        3000U          /* 当前值：3000ms，舵机1翻到背面后的等待。 */
#define BOARD2_ACTION_WAIT_CAPTURE_BACK_MS             8000U          /* 当前值：8000ms，背面拍照等待 K230 识别/上传。 */
#define BOARD2_ACTION_WAIT_AFTER_Z_UP_AFTER_BACK_MS    1000U          /* 当前值：1000ms，背面拍照后 Z 抬到安全高度的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_MOVE_TO_C_X_PRE_MS     500U          /* 当前值：500ms，到 C 点第一个前置点后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_MOVE_TO_C_PRE_MS       500U          /* 当前值：500ms，到 C 点第二个前置点后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_MOVE_TO_C_MS           500U          /* 当前值：500ms，到真实 C 点后的等待。 */
#define BOARD2_ACTION_WAIT_AFTER_RETURN_A00_MS         1000U          /* 当前值：1000ms，默认存放完成后回 A00 起始点的等待。 */

/* 取衣流程状态等待时间，单位：ms。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_HOME_TO_A0_MS    1000U          /* 当前值：1000ms，取衣流程启动后回 A0 的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_C01_X_PRE_MS 1000U      /* 当前值：1000ms，到 C01 第一个前置点后的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_C01_PRE_MS   1000U      /* 当前值：1000ms，到 C01 第二个前置点后的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_C01_MS   1000U          /* 当前值：1000ms，到真实 C01 后的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_EXIT_C01_PRE_MS  1000U          /* 当前值：1000ms，C01 夹取后退到第二个前置点的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_EXIT_C01_X_PRE_MS 1000U         /* 当前值：1000ms，C01 夹取后退到第一个前置点的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_B3_MS    1000U          /* 当前值：1000ms，C01 取到衣服后移动到 B3 的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_Z_DOWN_TO_BRUSH_MS 3000U        /* 当前值：3000ms，B3 下降到 Z=0 刷毛低位后的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_B4_MS    1000U          /* 当前值：1000ms，低位从 B3 移动到 B4 后的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_ROTATE_SERVO1_MS 3000U          /* 当前值：3000ms，B4 点舵机1翻面后的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_BACK_TO_B3_MS 1000U        /* 当前值：1000ms，低位从 B4 回到 B3 后的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_Z_UP_AFTER_BRUSH_MS 3000U       /* 当前值：3000ms，第二次刷毛后 Z 升到 3200 的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_MOVE_TO_A01_MS   1000U          /* 当前值：1000ms，移动到 A01 后的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_RETURN_A0_MS     1000U          /* 当前值：1000ms，A 区放置完成后返回 A0 的等待。 */
#define BOARD2_ACTION_WAIT_TAKE_AFTER_TELESCOPE_EXTEND_MS 3000U       /* 当前值：3000ms，取衣流程末尾伸缩杆伸出后的等待。 */

/* 取衣结束后动作14整机复位等待时间，单位：ms。 */
#define BOARD2_ACTION_WAIT_FETCH_RESET_AFTER_HOME_MS      1000U       /* XYZ 回到 A0 后的稳定等待。 */
#define BOARD2_ACTION_WAIT_FETCH_RESET_AFTER_SERVOS_MS     500U       /* 四路舵机回上电初始角度后的额外稳定等待。 */
#define BOARD2_ACTION_WAIT_FETCH_RESET_AFTER_TELESCOPE_MS 3000U       /* 伸缩轴按累计位置绝对回零后的稳定等待。 */

/* 舵机缓动参数：控制舵机每次更新走多少度，避免一次性跳变。 */
#define BOARD2_ACTION_SERVO_UPDATE_PERIOD_MS 20U                     /* 当前值：20ms，每隔 20ms 更新一次舵机角度。 */
#define BOARD2_ACTION_SERVO_FAST_STEP_DEG     6                      /* 当前值：6 度，距离目标很远时每次快走 6 度。 */
#define BOARD2_ACTION_SERVO_MID_STEP_DEG      3                      /* 当前值：3 度，距离目标中等时每次走 3 度。 */
#define BOARD2_ACTION_SERVO_SLOW_STEP_DEG     1                      /* 当前值：1 度，接近目标时每次慢走 1 度。 */
#define BOARD2_ACTION_SERVO_FAST_ZONE_DEG    60                      /* 当前值：60 度，误差大于 60 度使用快速步进。 */
#define BOARD2_ACTION_SERVO_MID_ZONE_DEG     30                      /* 当前值：30 度，误差大于 30 度使用中速步进，否则慢速。 */

/* 电机速度、加速度、估算等待和闭环等待参数。 */
#define BOARD2_ACTION_MOTOR_MAX_SPEED_RPM  2000U                    /* 当前值：2000rpm，动作流程给电机下发的最高转速上限。 */
#define BOARD2_ACTION_MOTOR_MAX_ACCEL        150U                    /* 当前值：150，动作流程给电机下发的最高加速度上限。 */
#define BOARD2_ACTION_MOTION_TIME_MARGIN_MS  150U                    /* 当前值：150ms，按角度估算运动时间后额外加的余量。 */
#define BOARD2_ACTION_CLOSED_LOOP_ENABLE       1U                    /* 当前值：1，启用闭环反馈等待；改 0 就只按估算时间等待。 */
#define BOARD2_ACTION_CLOSED_LOOP_TIMEOUT_MARGIN_MS 6000U            /* 当前值：6000ms，闭环反馈在估算时间外额外允许的超时余量。 */
#define BOARD2_ACTION_X_MAX_SPEED_RPM       1000U                    /* 当前值：1000rpm，自动流程里 X 轴最高转速，所有 Action_List 的 X 轴动作都会被这个值限制。 */
#define BOARD2_ACTION_XY_SYNC_ENABLE           1U                    /* 当前值：1，启用 X/Y 同步到点；改 0 后 X/Y 使用相同速度直接走。 */
#define BOARD2_ACTION_XY_SYNC_MIN_SPEED_RPM   80U                    /* 当前值：80rpm，X/Y 同步时较短轴的最低转速。 */
#define BOARD2_ACTION_XY_SYNC_MIN_ACCEL        8U                    /* 当前值：8，X/Y 同步时较短轴的最低加速度。 */
#define BOARD2_ACTION_XY_SYNC_DELAY_ENABLE     1U                    /* 当前值：1，短轴已经降到最低转速还会早到时，延后启动短轴。 */
#define BOARD2_ACTION_XY_SYNC_DELAY_THRESHOLD_MS 20U                 /* 当前值：20ms，预计早到超过 20ms 才做延后启动，小误差直接忽略。 */

#endif
