# K230 云端任务控制台

这个页面用于通过云端向 K230 发送任务码，让 K230 按现有协议转发给主板1，再触发主板2里的动作流程。页面可以直接运行在树莓派触摸屏的浏览器里，作为现场网页 HMI 的任务入口参考；它不是串口屏 HMI。

## 打开方式

在树莓派触摸屏浏览器或电脑浏览器中直接打开：

```text
C:\Users\ECHO\Desktop\家居\02_界面原型\k230_task_control\index.html
```

页面依赖公网 `https://unpkg.com/mqtt/dist/mqtt.min.js` 加载 MQTT WebSocket 库，电脑需要能访问外网。

## 默认云端参数

- Broker：`wss://broker.emqx.io:8084/mqtt`
- 命令主题：`smart_wardrobe/k230-wardrobe-001/cmd`
- 状态主题：`smart_wardrobe/k230-wardrobe-001/status`

## 下发格式

主板2任务按钮发送 JSON：

```json
{
  "id": "task-6-...",
  "source": "web-task-console",
  "device_id": "k230-wardrobe-001",
  "action": "todo",
  "action_id": 6,
  "note": "主板2自动放衣流程"
}
```

K230 当前 `main.py` 会把它转换成：

```text
@ACMD|6|task-6-...
```

主板1收到 `@ACMD` 后会先判断是否为本地任务码；`8/9` 由主板1本地执行，`6/7` 通过板间链路触发主板2流程。当前含义：

- `6`：启动 `Board2_ActionList_StartDefault()`，也就是完整自动放衣流程。
- `12`：启动 `Board2_ActionList_StartStorePrepare()`，只到 A0 后伸出伸缩杆并停住，给现场网页“存放”第一步使用。
- `11`：启动 `Board2_ActionList_StartDefaultAfterTelescope()`，给现场网页缩回并拍照后的后续存衣流程使用。
- `7`：启动 `Board2_ActionList_StartTakeClothes()`，也就是自动取衣流程。
- `8`：主板1打开风机风扇、保持加热关闭，并通过 UART4 驱动风机移动步进电机。
- `9`：主板1停止 UART4 风机移动步进电机，并关闭风机风扇与加热。
- `0`：急停，停止动作流程并失能步进轴。

## 使用步骤

1. 确认 K230 已联网，并运行包含 MQTT 云端控制逻辑的 `main.py`。
2. 打开本页面，点击“连接云端”。
3. 点击“查询 K230 状态”，如果 K230 在线，日志里应看到状态回传。
4. 点击“发送数字 6/7”触发主板2任务，或点击“发送数字 8/9”直接触发主板1风机移动任务。

如果网页显示已发送，但硬件没有动作，优先看页面日志里是否出现 K230 的 `cloud_rx` 和 `uart_tx` 回传；如果没有，问题在云端到 K230；如果有，继续检查 K230 到主板1、主板1 到主板2链路。
