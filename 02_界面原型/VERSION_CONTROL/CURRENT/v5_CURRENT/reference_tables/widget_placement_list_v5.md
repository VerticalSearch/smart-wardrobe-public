# v5 串口屏控件摆放清单

- 坐标系：800x480 px，左上角为 (0,0)。
- 坐标来源：`reports/backgrounds_*.layout.json`，也就是 v5 自动生成工程导出的真实布局。
- 推荐用法：背景图可直接使用 `previews/backgrounds_*.png`；本表主要列需要交互或运行时更新的控件。
- `optional_dynamic` 表示可做成控件，也可烘焙进背景图，取决于 STM32 是否需要运行时修改。

## 汇总

| 类型 | 数量 |
|---|---:|
| button | 10 |
| image | 6 |
| input_text | 1 |
| progress_bar | 3 |
| text | 59 |
| touch_hotspot | 101 |

## 页面数量

| 页码 | 页面 | 标题 | 推荐控件数 |
|---|---|---|---:|
| 01 | boot | 智能衣橱 | 8 |
| 02 | home | 智能衣橱 | 13 |
| 03 | retrieve_select | 选择衣物 | 20 |
| 04 | retrieve_progress | 取衣中 | 10 |
| 05 | retrieve_done | 取衣完成 | 10 |
| 06 | return_prepare | 归还准备 | 11 |
| 07 | return_progress | 归还中 | 10 |
| 08 | return_done | 归还完成 | 10 |
| 09 | treat_select | 护理选择 | 11 |
| 10 | treat_progress | 护理中 | 10 |
| 11 | treat_done | 护理完成 | 10 |
| 12 | browse | 衣物浏览 | 20 |
| 13 | network | 网络设置 | 14 |
| 14 | wifi_password | 输入密码 | 12 |
| 15 | error | 异常提示 | 11 |

## 控件明细

### 01 boot / 智能衣橱

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p01_txt_status | text | 34 | 98 | 582 | 46 | 初始化中 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p01_txt_action | text | 34 | 146 | 582 | 20 | 请稍候 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p01_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p01_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p01_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p01_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p01_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p01_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |

### 02 home / 智能衣橱

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p02_txt_status | text | 34 | 98 | 582 | 46 | 系统就绪 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p02_txt_action | text | 34 | 146 | 582 | 20 | 选择一个操作 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p02_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p02_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p02_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p02_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p02_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p02_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p02_btn_retrieve | button | 34 | 188 | 357 | 98 | 取衣 | HOME_RETRIEVE | 首页主操作按钮 |
| p02_btn_return | button | 409 | 188 | 357 | 98 | 归还 | HOME_RETURN | 首页主操作按钮 |
| p02_btn_treat | button | 34 | 298 | 237 | 52 | 护理 | HOME_TREAT | 首页主操作按钮 |
| p02_btn_browse | button | 281 | 298 | 237 | 52 | 浏览 | HOME_BROWSE | 首页主操作按钮 |
| p02_btn_network | button | 529 | 298 | 237 | 52 | 网络 | HOME_NETWORK | 首页主操作按钮 |

### 03 retrieve_select / 选择衣物

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p03_txt_status | text | 34 | 98 | 582 | 46 | 选择目标衣物 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p03_txt_action | text | 34 | 146 | 582 | 20 | 确认后开始 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p03_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p03_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p03_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p03_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p03_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p03_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p03_hot_item_1 | touch_hotspot | 34 | 180 | 732 | 66 | item_1 | RETRIEVE_ITEM_1 | 列表行透明触摸区 |
| p03_pic_item_1 | image | 36 | 180 | 66 | 66 |  |  | 衣物缩略图/占位图 |
| p03_txt_item_1_title | text | 116 | 194 | 650 | 22 | 衣物名称 |  | 衣物名称 |
| p03_txt_item_1_meta | text | 116 | 218 | 650 | 16 | 类别 / 挂位 / 状态 |  | 类别 / 挂位 / 状态 |
| p03_hot_item_2 | touch_hotspot | 34 | 260 | 732 | 66 | item_2 | RETRIEVE_ITEM_2 | 列表行透明触摸区 |
| p03_pic_item_2 | image | 36 | 260 | 66 | 66 |  |  | 衣物缩略图/占位图 |
| p03_txt_item_2_title | text | 116 | 272 | 650 | 22 | 待加载 |  | 衣物名称 |
| p03_txt_item_2_meta | text | 116 | 296 | 650 | 16 | 云端数据 |  | 类别 / 挂位 / 状态 |
| p03_hot_item_3 | touch_hotspot | 34 | 336 | 732 | 66 | item_3 | RETRIEVE_ITEM_3 | 列表行透明触摸区 |
| p03_pic_item_3 | image | 36 | 336 | 66 | 66 |  |  | 衣物缩略图/占位图 |
| p03_txt_item_3_title | text | 116 | 350 | 650 | 22 | 待加载 |  | 衣物名称 |
| p03_txt_item_3_meta | text | 116 | 374 | 650 | 16 | 云端数据 |  | 类别 / 挂位 / 状态 |

### 04 retrieve_progress / 取衣中

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p04_txt_status | text | 34 | 98 | 582 | 46 | 正在取衣 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p04_txt_action | text | 34 | 146 | 582 | 20 | 请勿遮挡设备 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p04_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p04_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p04_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p04_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p04_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p04_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p04_txt_progress_percent | text | 34 | 206 | 732 | 66 | 45% |  | 进度百分比 |
| p04_bar_progress | progress_bar | 34 | 296 | 732 | 20 | 45 |  | 使用全宽 0-100 进度条；背景条可烘焙到底图 |

### 05 retrieve_done / 取衣完成

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p05_txt_status | text | 34 | 98 | 582 | 46 | 衣物已送出 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p05_txt_action | text | 34 | 146 | 582 | 20 | 请取走衣物 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p05_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p05_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p05_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p05_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p05_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p05_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p05_txt_result_title | text | 180 | 252 | 586 | 52 | 已完成 |  | 结果主提示 |
| p05_txt_result_hint | text | 180 | 310 | 360 | 24 | 结果已同步 |  | 结果副提示，宽度按代码 wrap(360) 预留 |

### 06 return_prepare / 归还准备

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p06_txt_status | text | 34 | 98 | 582 | 46 | 放置衣物 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p06_txt_action | text | 34 | 146 | 582 | 20 | 挂到归还杆后开始 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p06_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p06_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p06_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p06_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p06_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p06_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p06_hot_return_start | touch_hotspot | 34 | 180 | 732 | 226 | 开始归还 | RETURN_START | 归还准备页整块启动触摸区 |
| p06_txt_prompt_title | text | 178 | 258 | 560 | 42 | 归还杆就绪 |  | 归还准备提示；固定不变可烘焙到底图 |
| p06_txt_prompt_hint | text | 178 | 304 | 560 | 24 | 挂好衣物后点击开始 |  | 归还准备提示；固定不变可烘焙到底图 |

### 07 return_progress / 归还中

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p07_txt_status | text | 34 | 98 | 582 | 46 | 正在归还 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p07_txt_action | text | 34 | 146 | 582 | 20 | 系统自动识别 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p07_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p07_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p07_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p07_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p07_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p07_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p07_txt_progress_percent | text | 34 | 206 | 732 | 66 | 62% |  | 进度百分比 |
| p07_bar_progress | progress_bar | 34 | 296 | 732 | 20 | 62 |  | 使用全宽 0-100 进度条；背景条可烘焙到底图 |

### 08 return_done / 归还完成

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p08_txt_status | text | 34 | 98 | 582 | 46 | 已入库 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p08_txt_action | text | 34 | 146 | 582 | 20 | 信息已更新 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p08_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p08_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p08_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p08_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p08_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p08_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p08_txt_result_title | text | 180 | 252 | 586 | 52 | 已完成 |  | 结果主提示 |
| p08_txt_result_hint | text | 180 | 310 | 360 | 24 | 结果已同步 |  | 结果副提示，宽度按代码 wrap(360) 预留 |

### 09 treat_select / 护理选择

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p09_txt_status | text | 34 | 98 | 582 | 46 | 选择护理模式 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p09_txt_action | text | 34 | 146 | 582 | 20 | 选择后开始护理 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p09_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p09_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p09_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p09_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p09_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p09_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p09_btn_hotwind | button | 34 | 180 | 235 | 92 | 热风 | TREAT_HOTWIND | 护理模式选择按钮 |
| p09_btn_ion | button | 283 | 180 | 235 | 92 | 负离子 | TREAT_ION | 护理模式选择按钮 |
| p09_btn_combo | button | 531 | 180 | 235 | 92 | 组合 | TREAT_COMBO | 护理模式选择按钮 |

### 10 treat_progress / 护理中

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p10_txt_status | text | 34 | 98 | 582 | 46 | 正在护理 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p10_txt_action | text | 34 | 146 | 582 | 20 | 完成后自动停止 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p10_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p10_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p10_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p10_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p10_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p10_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p10_txt_progress_percent | text | 34 | 206 | 732 | 66 | 58% |  | 进度百分比 |
| p10_bar_progress | progress_bar | 34 | 296 | 732 | 20 | 58 |  | 使用全宽 0-100 进度条；背景条可烘焙到底图 |

### 11 treat_done / 护理完成

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p11_txt_status | text | 34 | 98 | 582 | 46 | 护理结束 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p11_txt_action | text | 34 | 146 | 582 | 20 | 可继续操作 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p11_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p11_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p11_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p11_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p11_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p11_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p11_txt_result_title | text | 180 | 252 | 586 | 52 | 已完成 |  | 结果主提示 |
| p11_txt_result_hint | text | 180 | 310 | 360 | 24 | 结果已同步 |  | 结果副提示，宽度按代码 wrap(360) 预留 |

### 12 browse / 衣物浏览

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p12_txt_status | text | 34 | 98 | 582 | 46 | 浏览衣物 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p12_txt_action | text | 34 | 146 | 582 | 20 | 查看云端衣物库 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p12_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p12_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p12_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p12_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p12_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p12_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p12_hot_item_1 | touch_hotspot | 34 | 180 | 732 | 66 | item_1 | BROWSE_ITEM_1 | 列表行透明触摸区 |
| p12_pic_item_1 | image | 36 | 180 | 66 | 66 |  |  | 衣物缩略图/占位图 |
| p12_txt_item_1_title | text | 116 | 194 | 650 | 22 | 衣物名称 |  | 衣物名称 |
| p12_txt_item_1_meta | text | 116 | 218 | 650 | 16 | 类别 / 挂位 / 状态 |  | 类别 / 挂位 / 状态 |
| p12_hot_item_2 | touch_hotspot | 34 | 260 | 732 | 66 | item_2 | BROWSE_ITEM_2 | 列表行透明触摸区 |
| p12_pic_item_2 | image | 36 | 260 | 66 | 66 |  |  | 衣物缩略图/占位图 |
| p12_txt_item_2_title | text | 116 | 272 | 650 | 22 | 待加载 |  | 衣物名称 |
| p12_txt_item_2_meta | text | 116 | 296 | 650 | 16 | 云端数据 |  | 类别 / 挂位 / 状态 |
| p12_hot_item_3 | touch_hotspot | 34 | 336 | 732 | 66 | item_3 | BROWSE_ITEM_3 | 列表行透明触摸区 |
| p12_pic_item_3 | image | 36 | 336 | 66 | 66 |  |  | 衣物缩略图/占位图 |
| p12_txt_item_3_title | text | 116 | 350 | 650 | 22 | 待加载 |  | 衣物名称 |
| p12_txt_item_3_meta | text | 116 | 374 | 650 | 16 | 云端数据 |  | 类别 / 挂位 / 状态 |

### 13 network / 网络设置

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p13_txt_status | text | 34 | 98 | 582 | 46 | 选择网络 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p13_txt_action | text | 34 | 146 | 582 | 20 | 扫描附近 Wi-Fi |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p13_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p13_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p13_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p13_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p13_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p13_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p13_hot_wifi_1 | touch_hotspot | 34 | 196 | 732 | 56 | Wi-Fi 名称 | WIFI_SELECT_1 | Wi-Fi 列表行触摸区 |
| p13_txt_wifi_1 | text | 108 | 214 | 658 | 20 | Wi-Fi 名称 |  | Wi-Fi 列表显示文字 |
| p13_hot_wifi_2 | touch_hotspot | 34 | 266 | 732 | 56 | 信号强度 | WIFI_SELECT_2 | Wi-Fi 列表行触摸区 |
| p13_txt_wifi_2 | text | 108 | 284 | 658 | 20 | 信号强度 |  | Wi-Fi 列表显示文字 |
| p13_hot_wifi_3 | touch_hotspot | 34 | 336 | 732 | 56 | 连接状态 | WIFI_SELECT_3 | Wi-Fi 列表行触摸区 |
| p13_txt_wifi_3 | text | 108 | 354 | 658 | 20 | 连接状态 |  | Wi-Fi 列表显示文字 |

### 14 wifi_password / 输入密码

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p14_txt_status | text | 34 | 98 | 582 | 46 | 输入密码 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p14_txt_action | text | 34 | 146 | 582 | 20 | 连接选中网络 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p14_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p14_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p14_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p14_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p14_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p14_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p14_txt_ssid | text | 34 | 191 | 732 | 58 | SSID |  | Wi-Fi 密码页控件 |
| p14_txt_password | input_text | 34 | 265 | 732 | 58 | 请输入密码 |  | Wi-Fi 密码页控件 |
| p14_btn_connect | button | 34 | 339 | 360 | 54 | 连接 | WIFI_CONNECT | Wi-Fi 密码页控件 |
| p14_btn_cancel | button | 406 | 339 | 360 | 54 | 取消 | WIFI_CANCEL | Wi-Fi 密码页控件 |

### 15 error / 异常提示

| component_id | type | x | y | w | h | text | event | note |
|---|---|---:|---:|---:|---:|---|---|---|
| p15_txt_status | text | 34 | 98 | 582 | 46 | 需要确认 |  | 顶部状态文字；如果固定不变可烘焙到底图 |
| p15_txt_action | text | 34 | 146 | 582 | 20 | 处理后继续 |  | 顶部副提示文字；如果固定不变可烘焙到底图 |
| p15_hot_nav_home | touch_hotspot | 78 | 422 | 104 | 34 | 首页 | NAV_HOME | 底部导航透明触摸区；文字和高亮可用背景图 |
| p15_hot_nav_retrieve | touch_hotspot | 186 | 422 | 104 | 34 | 取衣 | NAV_RETRIEVE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p15_hot_nav_return | touch_hotspot | 294 | 422 | 104 | 34 | 归还 | NAV_RETURN | 底部导航透明触摸区；文字和高亮可用背景图 |
| p15_hot_nav_treat | touch_hotspot | 402 | 422 | 104 | 34 | 护理 | NAV_TREAT | 底部导航透明触摸区；文字和高亮可用背景图 |
| p15_hot_nav_browse | touch_hotspot | 510 | 422 | 104 | 34 | 浏览 | NAV_BROWSE | 底部导航透明触摸区；文字和高亮可用背景图 |
| p15_hot_nav_network | touch_hotspot | 618 | 422 | 104 | 34 | 网络 | NAV_NETWORK | 底部导航透明触摸区；文字和高亮可用背景图 |
| p15_txt_result_title | text | 180 | 252 | 586 | 52 | 等待确认 |  | 结果主提示 |
| p15_txt_result_hint | text | 180 | 310 | 360 | 24 | 查看原因并处理 |  | 结果副提示，宽度按代码 wrap(360) 预留 |
| p15_hot_error_confirm | touch_hotspot | 34 | 180 | 732 | 226 | 确认 | ERROR_CONFIRM | 异常提示确认/继续区域 |
