# K230 云端运动与任务控制台

入口文件：
- `index.html`
- `app.js`
- `styles.css`

网页通过 MQTT 给 K230 发送 JSON，K230 再转成串口文本发给主板1。该页面可以直接运行在树莓派触摸屏的浏览器里，作为现场网页 HMI 的调试/控制入口参考；它不是串口屏 HMI。

## 默认云端通道

- Web MQTT 地址：`wss://broker.emqx.io:8084/mqtt`
- K230 MQTT 地址：`broker.emqx.io:1883`
- 命令主题：`smart_wardrobe/k230-wardrobe-001/cmd`
- 状态主题：`smart_wardrobe/k230-wardrobe-001/status`

## K230 转发格式

```text
@MCMD|cmd|channel|control|angle|id
@SCMD|channel|angle|id
@ACMD|action_id|id
@OCMD|device|enable|id
```

`@OCMD` 是主板1本地开关量命令，`device=uv_lamp` 控制 `DIRC` 紫外灯，`device=vent_fan` 控制 `PWMB` 换气风扇；`enable=1` 拉高开启，`enable=0` 拉低关闭。

## 任务码约定

- `6`：主板2自动放衣流程。
- `7`：主板2自动取衣流程。
- `8`：主板1本地任务，打开风机风扇、保持加热关闭，并通过 UART4 驱动风机移动步进电机。
- `9`：主板1本地任务，停止 UART4 风机移动步进电机，并关闭风机风扇与加热。
- `0`：急停任务。

## 步进电机通道约定

- 通道 `1`：主板2 USART2，X轴两个步进电机
- 通道 `2`：主板2 USART3，Y轴步进电机
- 通道 `3`：主板2 UART4，Z轴步进电机
- 通道 `4`：主板2 UART5，地址2，伸缩杆步进电机
- 通道 `5`：主板2 UART5，地址1，刷毛步进电机
- 通道 `6`：主板1 UART4，热风机移动步进电机

网页累计目标和单步调试都按上面的通道号发送。K230 代码需要允许通道 `1~6`。

## 舵机通道约定

- 舵机通道 `1`：主板2原舵机1
- 舵机通道 `2`：主板2原舵机2
- 舵机通道 `3`：主板2 PWME，对应新增舵机3
- 舵机通道 `4`：主板2 PWMF，对应新增舵机4

网页“只发舵机目标”“同时发送目标”“舵机回中”都会按 `1~4` 全部发送。网页里的“单发舵机1/2/3/4”只发送一个舵机通道，格式为 `action=servo, channel=通道号, angle=角度`，K230 会转发为 `@SCMD|channel|angle|id`。

注意：主板2当前把网页舵机命令视为手动调试命令，收到 `CMD_SERVO_SET_ANGLE` 且 `channel != 0` 时会先停止自动动作流程，再执行该舵机角度。
