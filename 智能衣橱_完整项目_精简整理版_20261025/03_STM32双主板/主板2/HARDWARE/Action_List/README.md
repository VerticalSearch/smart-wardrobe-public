# 主板2动作流程说明

本文档对应 `HARDWARE/Action_List/board2_action_list.c`。当前代码使用固定状态机流程：点位表只保存 X/Y/Z 三个步进电机的实际累计目标角度，抓取和放置使用独立动作组，伸缩杆不写进点位表。

## 一、方向和坐标统一规则

- KEY1 按下后的 X 轴运动方向，就是 X 轴正方向。
- KEY2 按下后的 Y 轴运动方向，就是 Y 轴正方向。
- KEY3 按下后的 Z 轴运动方向，就是 Z 轴正方向；当前规定 Z 轴正方向为上升。
- USART6 收到数字 `5` 后伸缩杆转动的方向，就是伸缩杆正方向。
- 正方向运动时累计量增加，反方向运动时累计量减少，累计量允许为负数。
- 点位表里的 X/Y/Z 都是实际累计位置，不是相对移动量。
- 动作组里的 Z 轴角度是相对当前位置的移动量：Z 正数表示上升，Z 负数表示下降。
- 自动流程去任意 A/B/C 点位时，会先把 Z 抬到当前高度和目标高度中较高的那个高度，再移动 X/Y，最后校正 Z，避免低高度横移。
- 到达 `Axx` 抓取/放置点前，会先到 `A.x + BOARD2_ACTION_A_PRE_X_OFFSET_DEG, A.y + BOARD2_ACTION_A_PRE_Y_OFFSET_DEG` 的第一前置点，再到 `A.x, A.y + BOARD2_ACTION_A_PRE_Y_OFFSET_DEG` 的第二前置点，最后移动到真实 `Axx`。`A0` 回家点不走这个偏置流程。
- 到达 `Cxx` 放置点前，会先到 `Cxx` 的 X 轴负方向偏移 `BOARD2_ACTION_C_PRE_X_OFFSET_DEG`、Y 轴偏移 `get_c_pre_y_offset(Cxx)` 的前置停靠点，再到 `Cxx` 的 Y 轴偏移预进入位置，最后移动到真实 `Cxx`；当前 `C01` 使用 `Y-400`，其余 C 点使用 `Y+400`。
- 板间步进命令 `CMD_STEPPER_MOVE_ANGLE` 里 `control=0` 表示正方向，`control=1` 表示反方向，代码内部会换算成每个轴自己的驱动器方向码。

## 二、当前完整流程

当前默认上电不自动执行，`BOARD2_ACTION_LIST_AUTOSTART` 为 `0U`。USART6 收到命令 `6` 后开始自动放衣流程；收到命令 `12` 后只执行“到 A0 后伸出伸缩杆并停住”的存放准备流程；收到命令 `13` 后只执行存衣阶段的固定行程缩回；收到命令 `11` 后从“缩回并拍照后”的后续存衣流程继续；收到命令 `7` 后开始取衣流程；收到命令 `14` 后执行取衣结束整机复位。

1. X/Y/Z 先移动到回家点 `A0`。
2. 伸缩杆按 `BOARD2_ACTION_TELESCOPE_EXTEND_DEG` 相对运动，先伸出。
3. 等待 `BOARD2_ACTION_WAIT_HANG_MS`，用于人工挂衣服。
4. 伸缩杆按 `BOARD2_ACTION_TELESCOPE_RETRACT_DEG` 相对运动，挂完衣服后缩回。
5. 只移动 Z 轴到 `A0` 的高度，X/Y 不动。
6. 舵机2张开到 `BOARD2_ACTION_SERVO2_OPEN_DEG`，舵机1此时不动作。
7. X/Y/Z 按 A 区单个 X 偏置路径移动到当前抓取点 `Axx`。
8. 到达 `Axx` 后，舵机1保持当前角度，不再转回固定角度。
9. 执行 `s_grab_group[]` 抓取动作组；抓取时舵机1继续保持当前角度。
10. X 轴按 `BOARD2_ACTION_AFTER_GRAB_X_OFFSET_DEG` 相对移动固定距离。
11. X/Y/Z 先移动到 `B` 中转导航点。
12. 再从 `B` 中转导航点移动到拍照识别点 `B0`。
13. 主板2向主板1发送 `CMD_K230_CAPTURE`，主板1通知 K230 按当前舵机1角度拍照并上传。
14. Z 轴下降到 `BOARD2_ACTION_SERVO1_ROTATE_Z_DEG` 附近，默认基准是 `0`，当前允许 `±100` 度偏差。
15. 舵机1只在这个允许高度范围内翻到另一个角度：205 和 40 两个角度来回切换。
16. 主板2再次发送 `CMD_K230_CAPTURE`，主板1通知 K230 拍翻面后的照片并上传。
17. Z 轴升回 `BOARD2_ACTION_B_SAFE_Z_DEG`。
18. 先移动到当前 `Cxx` 的第一个前置停靠点：X 为 `Cxx - 900`，Y 为 `Cxx + get_c_pre_y_offset(Cxx)`，Z 按 `Cxx`。
19. 在第一个前置停靠点停顿 `BOARD2_ACTION_WAIT_AFTER_MOVE_TO_C_X_PRE_MS`，用于观察确认。
20. 再移动到当前 `Cxx` 的预进入点：X 和 Z 按 `Cxx`，Y 为 `Cxx + get_c_pre_y_offset(Cxx)`。
21. 再移动到真实 `Cxx` 放置点。
22. 执行 `s_place_group[]` 放置动作组；放置时只松开舵机2，不转舵机1。
23. A/C 索引加一，回到第 5 步；从 `Cxx` 去 `Axx+1`、抓取、到 `B`、再到 `B0` 之前，舵机1都保持刚才放置时的角度。
24. A 点处理完，或者 C 点用完，流程停止。

注意：启动后完整到 `A0` 和伸缩杆动作只执行一次。后面每件衣服循环时，只会再把 Z 轴抬回 `A0` 高度，不再重复伸缩杆动作。现场两步“存放”按钮使用命令 `12`，它只到 `A0` 并伸出伸缩杆，等待用户放好衣物后由网页缩回、拍照，再下发命令 `11` 继续后续存衣。

## 三、数字7取衣流程

数字 `7` 是单独的取衣流程，默认衣服挂在 `C01`，取下后放到 `A01`：

1. X/Y/Z 先移动到回家点 `A0`。
2. X/Y/Z 移动到 `C01` 的第一个前置点：X 为 `C01 - 900`，Y 为 `C01 - 400`，Z 按 `C01`。
3. X/Y/Z 移动到 `C01` 的第二个前置点：X 和 Z 按 `C01`，Y 为 `C01 - 400`。
4. X/Y/Z 移动到默认取衣点 `C01`。
5. 执行 `s_c_grab_group[]`，这是 C 区专用抓取流程。
6. 按进入 C01 的逆过程，先退到 `C01` 的第二个前置点：X 和 Z 按 `C01`，Y 为 `C01 - 400`。
7. 再退到 `C01` 的第一个前置点：X 为 `C01 - 900`，Y 为 `C01 - 400`，Z 按 `C01`。
8. X/Y/Z 移动到 `B3` 刷毛点，并保持安全高度进入。
9. 在 `B3` 把 Z 轴降到 `BOARD2_ACTION_TAKE_BRUSH_Z_DEG`，当前为 `0`。
10. 在 `B3` 执行第一次 `Board2_RunBrushCleanMotion()` 刷毛。
11. 保持低位 Z，只移动 X/Y 到 `B4`。
12. 在 `B4` 切换舵机1方向完成翻面。
13. 保持低位 Z，从 `B4` 回到 `B3` 后执行第二次刷毛。
14. Z 轴升到 `BOARD2_ACTION_TAKE_BRUSH_Z_UP_DEG`，当前为 `3200`。
15. X/Y/Z 按 A 区两段正向偏置路径移动到默认放衣点 `A01`。
16. 执行 `s_a_place_group[]`，这是 A 区专用放置流程。
17. X/Y/Z 回到 `A0`。
18. 伸缩杆按 `BOARD2_ACTION_TELESCOPE_EXTEND_DEG` 相对运动，最后伸出。
19. 流程结束。

取衣完成后，用户先从伸出的挂衣杆上取下衣物，再由网页发送动作 `14`。动作 `14` 会先根据伸缩轴软件累计位置绝对回到 `0`，确认缩回后再让 X/Y/Z 回到 `A0`，最后把舵机1至舵机4恢复到上电初始角度 `190/170/250/0`，同时关闭拍照补光。因此网页超时重试动作 `14` 时不会再次固定缩回一个完整行程。动作 `13` 仍只属于存衣流程，不承担取衣后的整机复位。

## 四、当前点位表

点位现在统一在 `board2_action_tuning.h` 的 `BOARD2_ACTION_POINT_TABLE(_)` 中修改，`board2_action_list.c` 会从这张表自动生成点位编号和 `s_points[]`，不要再手动维护两份顺序。

```c
#define BOARD2_ACTION_POINT_TABLE(_) \
    _(A0,  "A0",     0.0f,    0.0f, 3200.0f) \
    _(A01, "A01", 1250.0f, 1900.0f, 3200.0f) \
    _(A02, "A02", 1260.0f, 1700.0f, 3200.0f) \
    _(A03, "A03", 1260.0f, 1200.0f, 3200.0f) \
    _(A04, "A04", 1260.0f,  700.0f, 3200.0f) \
    _(A05, "A05", 1320.0f,  150.0f, 3200.0f) \
    _(B,   "B",   2600.0f, 4000.0f, 3200.0f) \
    _(B0,  "B0",  2000.0f, 4000.0f, 3200.0f) \
    _(C01, "C01", 6170.0f,  240.0f, 3200.0f) \
    _(C02, "C02", 6170.0f, 7810.0f, 3200.0f)
```

当前 `BOARD2_ACTION_A_COUNT` 为 `5U`，所以会处理 `A01` 到 `A05`。当前 `BOARD2_ACTION_C_COUNT` 为 `10U`，所以默认使用 `C01` 到 `C10`；如果要启用 `C11/C12`，先在 `BOARD2_ACTION_POINT_TABLE(_)` 里保留对应行，再调整 `BOARD2_ACTION_C_COUNT`。

## 四、集中调试参数

主要参数都集中在 `board2_action_tuning.h`。调试时优先改这个文件，不要到状态机中间改硬编码。

常用参数如下：

```c
/* 点位循环 */
#define BOARD2_ACTION_A_COUNT                5U
#define BOARD2_ACTION_C_COUNT                10U

/* 舵机角度 */
#define BOARD2_ACTION_SERVO2_OPEN_DEG        120
#define BOARD2_ACTION_SERVO1_FRONT_DEG       185
#define BOARD2_ACTION_SERVO1_BACK_DEG        10

/* 运动位置和偏置 */
#define BOARD2_ACTION_AFTER_GRAB_X_OFFSET_DEG 900.0f
#define BOARD2_ACTION_SERVO1_ROTATE_Z_DEG    0.0f
#define BOARD2_ACTION_B_ROTATE_Z_TOLERANCE_DEG 100.0f
#define BOARD2_ACTION_B_SAFE_Z_DEG           2600.0f
#define BOARD2_ACTION_A_PRE_X_OFFSET_DEG     300.0f
#define BOARD2_ACTION_A_PRE_Y_OFFSET_DEG     320.0f
#define BOARD2_ACTION_C_PRE_X_OFFSET_DEG     900.0f
#define BOARD2_ACTION_C_PRE_Y_OFFSET_DEG     400.0f
#define BOARD2_ACTION_TELESCOPE_EXTEND_DEG   -18000.0f
#define BOARD2_ACTION_TELESCOPE_RETRACT_DEG   18000.0f

/* 放衣流程等待 */
#define BOARD2_ACTION_WAIT_HANG_MS           5000U
#define BOARD2_ACTION_WAIT_CAPTURE_FRONT_MS  8000U
#define BOARD2_ACTION_WAIT_CAPTURE_BACK_MS   8000U
#define BOARD2_ACTION_WAIT_AFTER_MOVE_TO_C_X_PRE_MS 500U

/* 电机和闭环 */
#define BOARD2_ACTION_MOTOR_MAX_SPEED_RPM    2000U
#define BOARD2_ACTION_MOTOR_MAX_ACCEL        150U
#define BOARD2_ACTION_CLOSED_LOOP_ENABLE     1U
#define BOARD2_ACTION_X_MAX_SPEED_RPM        1000U
#define BOARD2_ACTION_XY_SYNC_ENABLE         1U
#define BOARD2_ACTION_XY_SYNC_MIN_SPEED_RPM  80U
#define BOARD2_ACTION_XY_SYNC_DELAY_ENABLE   1U
```

抓取/放置动作组的每一步等待时间也在同一个调试区里，例如 `BOARD2_ACTION_WAIT_GRAB_DOWN_MS`、`BOARD2_ACTION_WAIT_PLACE_RELEASE_MS`、`BOARD2_ACTION_WAIT_A_PLACE_DOWN_MS`。
## 五、抓取动作组

抓取动作组是 `s_grab_group[]`：

```c
static const Board2ActionGroupStep_t s_grab_group[] =
{
    /* name,    Z absolute, servo1,                    servo2,                    wait */
    { "open",     2400.0f, BOARD2_ACTION_SERVO_KEEP, 150,                       300U },
    { "down",        0.0f, BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,  5000U },
    { "close",       0.0f, BOARD2_ACTION_SERVO_KEEP, 50,                        400U },
    { "up",       2400.0f, BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,  5000U },
};
```

- 动作组里的 Z 是累计坐标目标值，不再是相对移动量。
- `down` 写成 `0.0f` 表示下降后实际累计位置到 `0`，`up` 写成 `2400.0f` 表示回到实际累计位置 `2400`。
- 舵机角度写 `BOARD2_ACTION_SERVO_KEEP` 时表示该舵机不动作。

取衣流程新增了 C 区抓取动作组 `s_c_grab_group[]`。默认 C 区取衣高度沿用 C 区放置高度，下降到累计 Z=`0.0f` 后夹紧，再抬回 `2600.0f`。

## 六、放置动作组

放置动作组是 `s_place_group[]`：

```c
static const Board2ActionGroupStep_t s_place_group[] =
{
    /* name,    Z absolute, servo1,                    servo2,                    wait */
    { "down",        0.0f, BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,  5000U },
    { "release",     0.0f, BOARD2_ACTION_SERVO_KEEP, 150,                       400U },
    { "up",       2400.0f, BOARD2_ACTION_SERVO_KEEP, BOARD2_ACTION_SERVO_KEEP,  5000U },
};
```

如果放置时夹爪张开太小，就调 `release` 那一行的舵机2角度。放置期间舵机1保持不动，等移动到下一个 `Axx` 后也保持当前角度，直到先经过 `B`、到达 `B0` 且 Z 轴降到 `0` 后才翻到另一个角度。如果 Z 轴下降不够，就调 `down` 那一行的 Z 累计目标位置；当前写 `0.0f` 表示下降到实际累计位置 `0`，`up` 写 `2400.0f` 表示回到实际累计位置 `2400`。

取衣流程新增了 A 区放置动作组 `s_a_place_group[]`。默认 A 区放衣高度沿用 A 区抓取高度，下降到累计 Z=`-1200.0f` 后松开，再抬回 `2600.0f`。

## 七、运动控制说明

X/Y 到点时会尽量同步到达：

- 现在使用“目标时间反推速度”算法，不再只按距离比例缩放。
- 程序先计算 X/Y 在各自最高速度下的最快运行时间，取较慢的那一轴作为共同目标时间。
- 另一轴按共同目标时间反推出转速，并按转速比例降低加速度，减少短轴提前冲到位。
- 如果短轴已经降到最低转速还是会早到，并且早到时间超过 `BOARD2_ACTION_XY_SYNC_DELAY_THRESHOLD_MS`，程序会延后启动短轴。
- 最大速度限制为 `BOARD2_ACTION_MOTOR_MAX_SPEED_RPM`，当前 `2000RPM`。
- 自动流程里 X 轴还有单独限速 `BOARD2_ACTION_X_MAX_SPEED_RPM`，当前 `1000RPM`；即使全局最大速度更高，Action_List 里的 X 轴动作也不会超过这个值。
- 最大加速度限制为 `BOARD2_ACTION_MOTOR_MAX_ACCEL`，当前 `150`。

每次发出步进电机运动命令后，程序会估算本次运动需要的时间，并进入阻塞等待。时间没到，不会执行下一步任务。

舵机也做了软件缓动：

- 距离目标角度远时走得快。
- 接近目标角度时走得慢。
- 如果舵机当前角度和目标角度一样，不会重复设置 PWM，用来减少舵机1抖动。

## 八、常用调试建议

- 上电后伸缩杆不应自动动作；完整自动放衣流程需要 USART6 收到命令 `6` 才会开始；现场网页“存放”第一步使用命令 `12`，先到 `A0` 再伸出伸缩杆并停住；网页缩回并拍照后，后续存衣流程使用命令 `11`，从 `Z_TO_A0` 之后继续；取衣流程需要收到命令 `7` 才会开始。
- 自动流程运行期间，KEY1~KEY4 和 USART6 的数字 `1~7` 手动命令会被忽略，防止手动动作插入后坐标跑偏。
- 自动流程运行期间，USART6 数字 `0` 会停止流程并失能四个步进轴。
- 伸缩杆伸出方向不对：先确认 USART6 数字 `5` 触发的方向是否被定义为伸缩杆正方向，再调整 `BOARD2_ACTION_TELESCOPE_EXTEND_DEG` 的正负号。
- 伸缩杆缩回方向不对：调整 `BOARD2_ACTION_TELESCOPE_RETRACT_DEG` 的正负号。
- 抓取点位置不准：改对应 `Axx` 的 X/Y/Z。
- B中转点位置不准：改 `B` 的 X/Y/Z。
- 拍照点位置不准：改 `B0` 的 X/Y/Z。
- 放置点位置不准：改对应 `Cxx` 的 X/Y/Z。
- B 点翻转高度不对：调整 `BOARD2_ACTION_SERVO1_ROTATE_Z_DEG` 和 `BOARD2_ACTION_B_ROTATE_Z_TOLERANCE_DEG`，舵机1只在这个Z轴高度范围内翻面。
- 舵机1两个翻面角度不对：调整 `BOARD2_ACTION_SERVO1_FRONT_DEG` 和 `BOARD2_ACTION_SERVO1_BACK_DEG`。
- 舵机2张开角度不对：调整 `BOARD2_ACTION_SERVO2_OPEN_DEG`。
