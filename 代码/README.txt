家居代码分成两部分 主控板1和主控板2 然后每个文件夹里面都会有对应电路板的网表文件 然后你参考电路板的的引脚分配和农装代码里面的文件对这两块电路板进行代码编写

[2026-04-11 4.11项目改动记录]
1) 主板1 串口功能分配
- UART1: 与主板2通信（沿用农装帧头0xFF与校验）
- UART2: ESP32-S3（仅初始化 + 基础发送）
- UART3: K230（仅初始化 + 基础发送）
- UART4: 激光测距模块（移植农装激光解析）
- UART5: 步进电机（移植农装步进协议，修正引脚 TX=PC12 RX=PD2）
- UART6: 串口屏（陶晶驰，仅初始化 + 基础发送）

2) 主板2 串口功能分配
- UART1: 与主板1通信（沿用农装帧格式）
- UART2: 步进电机M1
- UART3: 步进电机M2
- UART4: 步进电机M3
- UART5: 步进电机M4
- UART6: 步进电机M5

3) 新增/移植模块
- 主板1: HARDWARE/USART1.DMA, HARDWARE/Laser_Common, HARDWARE/USART4.DMA, HARDWARE/Stepper
- 主板1新增: HARDWARE/UART2_ESP32, HARDWARE/UART3_K230, HARDWARE/UART6_SCREEN
- 主板2新增: HARDWARE/USART1.DMA, HARDWARE/Stepper_Multi

4) 调度与时基
- 两块板均使用 TIM7 1ms 定时中断
- 在 TIM7 中仅做轻量tick和10ms/20ms任务标志
- 业务处理在主循环按标志执行

5) 串口波特率
- 本轮统一为 115200

6) 工程同步
- 已同步更新主板1/主板2 USER/MPU9250.uvprojx：
  IncludePath 增加新增模块目录，HARDWARE组增加新增.c/.h文件，保证Keil可直接编译。

[2026-04-11 补充修正]
- 主板1 UART4 激光模块重写：中断仅解析并置位帧就绪标志，20ms 软任务里执行 data4_process。
- 主板1 USART1.DMA 修复 CalculateChecksum 文本损坏与 C90 块内声明问题。
- 主板2 USART1.DMA 新增命令分发：CMD_GET_SENSOR / CMD_STEPPER_ENABLE / CMD_STEPPER_MOVE_ANGLE / CMD_EMERGENCY。
- 主板2 步进命令默认映射：param1=通道(1~5)，param2=使能(0/1)或角度(度)。
- UART5 引脚最终按 STM32F407 硬件复用与网表对应：TX=PC12，RX=PD2。

[2026-04-12 步进/激光回归纯移植]
- 主板1 步进模块（HARDWARE/Stepper）回归为农装原始移植代码。
- 主板1 激光模块（HARDWARE/USART4.DMA + HARDWARE/Laser_Common）回归为农装原始移植代码。
- 仅保留步进 UART5 引脚修正：TX=PC12，RX=PD2；未再做其它逻辑改动。
- 为兼容农装原激光文件依赖，补充 USART3.DMA/usart3dma.h 头文件并加入 IncludePath。

[2026-04-13 控制调度改为TIM7中断内直接执行]
- 主板1/主板2 的 control 模块取消 10ms/20ms 任务标志位与 Control_Run10ms/Control_Run20ms 分发函数。
- 改为在 TIM7_IRQHandler 中按计数直接执行定时任务：
  主板1每10ms执行 Master_ProcessReceivedData()；
  主板2每10ms执行 Board2_ProcessReceivedCommand()。
- main.c 主循环不再做任务标志判断，仅保留初始化与空循环。
- 本次仅调整调度方式，不改步进和激光模块业务逻辑。

[2026-04-13 注释乱码修复]
- 修复 4.11 主板1/主板2 工程注释显示乱码问题。
- 将两工程源码文件（.c/.h）统一转换为 GBK 编码，便于 Keil 在中文环境正常显示中文注释。
- 对主板1 的 Stepper 与 USART1.DMA 注释内容进行中文恢复；保留既有功能逻辑。
- 保留步进 UART5 引脚修正：TX=PC12，RX=PD2。
[2026-04-13 注释乱码修复补充]
- 清理主板1 USART1.DMA/usartdma.c 中残留的字面量 "`r`n" 文本，恢复为正常两行 C 语句。
- 复查主板1/主板2 共 211 个 .c/.h 文件：未发现常见乱码字形与异常前缀字符。
[2026-04-13 板2按键调试与control参数宏化]
- 板2 control.h 完成参数宏化：按键扫描参数、X/Y/Z轴映射、每轴角度/方向/速度/加速度、KEY4回零策略。
- 板2按键扩展为4键并按网表引脚定义：KEY1=PE4，KEY2=PE5，KEY3=PE6，KEY4=PC13。
- 板2 TIM7中断内执行按键调试动作：
  KEY1控制X轴(UART2)，KEY2控制Y轴(UART3)，KEY3控制Z轴(UART4)，KEY4三轴反向回初始位。
- X轴按“单逻辑电机”处理：同默认地址仅发送一条命令，不再做双地址/双命令。
- 板1 control.h 新增激光区间宏(C1~C4)与阈值宏，control.c 增加区间判定占位逻辑（不接K230/ESP32业务链路）。
- 本轮未改动步进与激光底层协议模块，仅在control层调度与参数入口改动。
[2026-04-13 主板2工程编译修复]
- 修复主板2链接错误 Undefined symbol KEY_Init：在 主板2/USER/MPU9250.uvprojx 的 HARDWARE 组中补充 key.c 与 key.h。
- 主板2 control.c 清理条件编译相关写法，消除未使用变量与文件结尾换行警告。
- 命令行编译验证：主板1、主板2均为 0 Error(s), 0 Warning(s)。
[2026-04-15 OLED显示补充]
- 主板1 main.c 新增OLED状态页刷新（200ms）：显示距离D、区间Z、布料判定C、系统节拍T。
- 主板2 main.c 新增OLED状态页刷新（200ms）：显示最近按键K、系统节拍T、X/Y/Z累计角度。
- 本次仅增加显示逻辑，未改动步进和激光底层协议模块。
[2026-04-15 主板2步进不转排查修正]
- 修正主板2 HARDWARE/Stepper_Multi/stepper_multi.c 发送时序，与农装步进模块保持一致。
- 在每帧发送完成后增加 10ms 间隔，避免“使能命令后紧跟运动命令”被驱动器忽略。
- 本次仅调整主板2步进串口发送节奏，不改激光模块与协议帧格式。
[2026-04-18 主板2 UART2双步进反向联动]
- 按新需求修改主板2步进逻辑：UART2同时控制ID1与ID2两台电机，发送两条方向相反的串口命令。
- 新增 stepper_multi 接口：
  stepper_multi_enable_dual_opposite_uart2()
  stepper_multi_move_angle_dual_opposite_uart2()
- control层已接入：KEY1触发X轴时，改为UART2对ID1/ID2反向联动；KEY4回零时X轴同样按双电机反向执行。
- 板间命令层已接入：当通道参数映射到UART2时，CMD_STEPPER_ENABLE / CMD_STEPPER_MOVE_ANGLE / CMD_EMERGENCY 均按ID1/ID2双电机处理。
- 补充了相关中文注释，说明双电机反向联动和加速度限幅逻辑。
- 新增加速度限幅宏 BOARD2_ACCEL_U8(x)，避免超出协议1字节范围导致编译截断告警。
- 命令行编译验证：主板2 0 Error(s), 0 Warning(s)。
[2026-04-18 代码规范整理与注释补充]
- 对主板2 USART1.DMA 模块进行逻辑整理：抽取通道使能、角度运动、急停三个内部函数，减少重复分支。
- 对主板2 USART1.DMA 模块进行命名规范化：
  内部DMA缓存与命令状态改为模块私有前缀变量（s_），避免无关模块直接依赖内部状态。
- 主板2 UART2双电机逻辑保持不变：ID1/ID2反向联动，同时覆盖按键链路与板间命令链路。
- 新增/补充注释：
  stepper_multi.h 新增接口语义注释；
  control.c 新增按键扫描、回零流程、TIM7调度注释；
  usartdma.c 新增双电机分发与协议处理注释。
- 主板2 control.h 新增加速度限幅宏 BOARD2_ACCEL_U8(x) 说明，确保协议1字节参数边界清晰。
- 命令行编译验证：主板2 0 Error(s), 0 Warning(s)；主板1复核编译 0 Error(s), 0 Warning(s)。
[2026-04-18 control.h乱码与参数注释修复]
- 重写主板2 HARDWARE/control/control.h 注释，修复显示乱码问题（按中文环境保存，便于Keil直接查看）。
- 将需要手动改参的宏在行尾补充中文说明：
  时基参数、按键参数、轴映射、X/Y/Z地址/方向/速度/加速度/步距角、KEY4回零策略。
- 明确加速度限幅行为：BOARD2_ACCEL_U8(x) 将参数限制为0~255，避免协议字段溢出。
- 命令行编译验证：主板2 0 Error(s), 0 Warning(s)。
[2026-04-18 control.h转UTF-8]
- 按需求将主板2 HARDWARE/control/control.h 编码转换为 UTF-8（带BOM）。
- 保留原有参数注释与宏定义，仅做编码格式调整。
- 编译验证：主板2 0 Error(s), 0 Warning(s)。
[2026-04-18 主板1全量UTF-8转换]
- 按需求将主板1工程全部 .c/.h 文件统一转换为 UTF-8（带BOM），后续中文注释按 UTF-8 维护。
- 本次转换覆盖主板1 USER/HARDWARE/STM32F4xx_FWLIB 等源码目录，共处理 111 个文件。
- 保留原有业务逻辑与接口，仅做编码格式统一，便于 Keil/Cursor 下中文显示一致。
- 编译验证：主板1 0 Error(s), 0 Warning(s)。
