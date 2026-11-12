# 树莓派衣橱网关

这个目录是树莓派替代 K230 的本地网关服务。它提供触摸屏网页、电脑调试网页、USB 摄像头拍照上传，以及通过 CH340 USB 串口向主板1发送现有控制协议。

## 连接方式

控制任务时要接主板1的 K230 兼容串口入口，也就是 `代码/4.11/主板1` 里的 `UART3_K230`：

- CH340 `TXD` 接主板1 `PD9 / USART3_RX`
- CH340 `RXD` 接主板1 `PD8 / USART3_TX`
- CH340 `GND` 接主板1 `GND`
- 默认不接 `VCC`
- 串口参数：`115200 8N1`

注意：`PC6 / PC7 / USART6` 是旧树莓派检测包或串口屏链路。当前网关发送的是 `@MCMD`、`@SCMD`、`@ACMD`、`@OCMD` 这套 K230 控制协议；如果直接接到 USART6，除非主板1固件已改成在 USART6 解析这套协议，否则网页控制任务不会生效。

服务会优先查找 `/dev/serial/by-id/*`，再退回 `/dev/ttyUSB*` 和 `/dev/ttyACM*`。

## 页面

- 触摸屏：`http://127.0.0.1:8000/`
- 电脑调试：`http://树莓派IP:8000/debug`

`/` 显示本地触摸控制屏。`/cloud` 会优先显示原来的 `cloud_display.html`，并通过网关把 `/api/...` 请求转发到云端。

## 常用接口

- `POST /api/control`：发送控制命令，支持 `todo`、`servo`、`stepper_move`、`stepper_enable`、`set_targets`、`board1_output`、`emergency`。
- `POST /api/camera/capture-upload`：USB 摄像头拍照；默认仅保存本地。只有显式传入 `upload=true`、匹配当前活动存衣事务的 `inventory_command_id`，且事务已有真实 C 点时才允许上传云端。
- `GET /api/status`：查看串口、摄像头、MQTT、最近上传和日志状态。
- `GET /api/inventory`：查看 C01-C12 的持久化占用、预留和异常状态。
- `POST /api/inventory/recover`：恢复重启中断或动作失败后锁定的 C 点。
- `POST /api/wardrobe/clear`：批量把云端衣物设为 `deleted`，并同步释放成功删除衣物占用的本地 C 点；进行中或异常槽位会拒绝删除。
- `GET /api/serial/logs`：查看最近串口收发记录。
- `POST /api/serial/detection`：可选旧树莓派检测包发送接口。

## 对接结论

- 触摸屏页面 `/` 和电脑调试页 `/debug` 都调用同一个 `/api/control` 后端入口。
- 原云端展示页面需要的 `/api/...` 请求由网关转发到 `WARDROBE_CLOUD_API_BASE`，云端断开时本地控制页仍可打开。
- 控制动作按现有 K230 协议转发：`todo -> @ACMD`，`servo -> @SCMD`，`stepper_* -> @MCMD`，`board1_output -> @OCMD`。
- 摄像头只负责拍照，树莓派本地不做 AI 识别；云端建档必须绑定活动存衣事务和真实 C 点，上传结果状态会回写到网页状态和串口结果帧。
- 存衣动作由树莓派先自动预留 C01-C12 空位，再把 `real_position` 随图片上传到云端；主板2返回 `done` 后才提交为在柜。取衣动作根据衣物 ID 解析同一 C 点并下发动作7。
- 正面和背面拍照各要求 3 张有效原始帧，六图合成失败时本次建档直接失败，不会退化为单张图片继续上报。
- MQTT 调试命令仍订阅 `smart_wardrobe/k230-wardrobe-001/cmd`，收到后转成同一套串口命令。

## 树莓派安装

把整个 `raspberry_pi_gateway` 目录放到树莓派 `/home/pi/raspberry_pi_gateway` 后执行：

```bash
cd /home/pi/raspberry_pi_gateway
bash scripts/install_pi.sh
```

检查服务：

```bash
systemctl status wardrobe-gateway.service
journalctl -u wardrobe-gateway.service -f
```

## 配置项

可在 systemd service 里通过环境变量调整：

- `WARDROBE_SERIAL_PORT=auto`
- `WARDROBE_SERIAL_BAUD=115200`
- `WARDROBE_CLOUD_API_BASE=http://119.45.230.229:8000`
- `WARDROBE_UPLOAD_URL=http://119.45.230.229:8000/api/wardrobe/add`
- `WARDROBE_UPLOAD_MULTI_URL=http://119.45.230.229:8000/api/wardrobe/add-multi`
- `WARDROBE_MQTT_ENABLE=true`
- `WARDROBE_MQTT_COMMAND_TOPIC=smart_wardrobe/k230-wardrobe-001/cmd`
- `WARDROBE_MQTT_STATUS_TOPIC=smart_wardrobe/k230-wardrobe-001/status`
