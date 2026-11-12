# 02_树莓派网关与网页

现场触摸屏网页、Python 网关、摄像头、串口桥、库存管理和测试。

`raspberry_pi_gateway` 是完整本地网关工程。`src/wardrobe_gateway` 是Python后端，`static` 是页面，`tests` 是原测试，`scripts` 和 `systemd` 是安装/服务模板。

`02_界面原型/cloud_display.html` 是网关默认配置和测试依赖的同级副本。这两份网页必须同步，不能为了去重删掉其中一份。网关目录内部结构保持不变。

树莓派部署入口仍为网关README和 `scripts/install_pi.sh`，其默认用户与路径是 `pi` 和 `/home/pi/raspberry_pi_gateway`。部署时核对现场用户、路径、串口和环境变量。当前运行页面使用 `/cloud`；直接双击HTML只能查看静态文件，不能证明设备链路已连接。

`现有桌面启动模板` 保存本机已有的启动shell、桌面快捷方式和labwc自启动条目。shell对应 `/home/pi/raspberry_pi_gateway`，快捷方式和自启动条目调用 `/home/pi/start_wardrobe_gateway.sh`；涉及Chromium、ffmpeg与v4l2-ctl。它们是部署参考，未与本次现场设备核对、未执行。

本包未包含实时库存data、相机captures、虚拟环境或凭据；它不是现场状态备份。独立远端云服务需后端人员另行提供。
