# 07_安卓客户端

本地现有安装包及从该安装包读取的内嵌网页；非原生完整构建工程。

`既有安装包/H594B5CF3_20260607_care_schedule_installable_v2v3.apk` 是本地现有的2026-06-07构建，未重新打包或安装，不能据此认定现场版本。

`安装包内网页/www/index.html` 和 `manifest.json` 从上述同一APK的 `assets/apps/H594B5CF3/www/` 原样读取。APK内页面与树莓派8月网页是不同时间的版本，修改02不会自动更新此APK。

没有找到完整Android原生工程；没有附带签名私钥。需要发布新APK时，应从原开发者获取原生/HBuilder工程和受控签名流程。
