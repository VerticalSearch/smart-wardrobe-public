# 样例用途

这些是人工构造的接口审查样例，不是现场采集记录，也不会自动发送。

- `prepare_action12_mqtt.json`：动作12只准备挂衣；完成不代表整次存衣结束。
- `store_after_retract_http.json`：供网关`POST /api/hardware/task-dispatch`使用的请求结构，前提是挂衣/缩回阶段已完成；网关分配可用C槽。`capture_mode`沿用当前页面字符串，实际拍照逻辑还需查看camera/app，不应据该名称断言只拍一帧。
- `mqtt_terminal.json`：展示当前终态消息的字段布局，故意保留外层空command_id；ts=0为占位值。
- 新任务使用不同短ASCII ID；重试同一任务保留原ID，并先查其运行/完成状态。

真实故障仍需记录动作13的发送与done，不能直接拿动作11样例跳过当前机构状态检查。
