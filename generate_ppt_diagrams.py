import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, Arc
import numpy as np
import os

plt.rcParams['font.sans-serif'] = ['Microsoft YaHei', 'SimHei']
plt.rcParams['axes.unicode_minus'] = False

OUT = r'C:\Users\ECHO\Desktop\家居\ppt_diagrams'
os.makedirs(OUT, exist_ok=True)

BOX_STYLES = {
    'user':    dict(facecolor='#E8F4FD', edgecolor='#2196F3', linewidth=2),
    'app':     dict(facecolor='#E3F2FD', edgecolor='#1976D2', linewidth=2),
    'cloud':   dict(facecolor='#F3E5F5', edgecolor='#9C27B0', linewidth=2),
    'vision':  dict(facecolor='#E0F7FA', edgecolor='#00BCD4', linewidth=2),
    'board1':  dict(facecolor='#E8F5E9', edgecolor='#4CAF50', linewidth=2),
    'board2':  dict(facecolor='#FFF3E0', edgecolor='#FF9800', linewidth=2),
    'mech':    dict(facecolor='#ECEFF1', edgecolor='#607D8B', linewidth=2),
    'backend': dict(facecolor='#FCE4EC', edgecolor='#E91E63', linewidth=2),
}

def rounded_box(ax, xy, width, height, style_key, text='', text_y_offset=0, fontsize=12):
    x, y = xy
    box = FancyBboxPatch((x, y), width, height,
                          boxstyle='round,pad=0.3', **BOX_STYLES[style_key])
    ax.add_patch(box)
    if text:
        ax.text(x + width/2, y + height/2 + text_y_offset, text,
                ha='center', va='center', fontsize=fontsize, fontweight='bold')

def arrow(ax, x1, y1, x2, y2, color='#555555', lw=1.5):
    ax.annotate('', xy=(x2, y2), xytext=(x1, y1),
                arrowprops=dict(arrowstyle='->', color=color, lw=lw))

def double_arrow(ax, x1, y1, x2, y2, color='#555555', lw=1.5):
    ax.annotate('', xy=(x2, y2), xytext=(x1, y1),
                arrowprops=dict(arrowstyle='<->', color=color, lw=lw))

def sub_label(ax, x, y, text, fontsize=9, color='#555555'):
    ax.text(x, y, text, ha='center', va='top', fontsize=fontsize, color=color)

# ============================================================
# FIGURE 1: 系统总体架构 (System Architecture)
# ============================================================
fig, ax = plt.subplots(figsize=(16, 10))
ax.set_xlim(0, 16)
ax.set_ylim(0, 10)
ax.axis('off')
ax.set_facecolor('#FAFAFA')
fig.patch.set_facecolor('#FAFAFA')

ax.text(8, 9.5, '智能衣橱系统总体架构', ha='center', fontsize=26, fontweight='bold', color='#1A237E')

# Layer labels
layers_info = [
    (13.5, 8.3, '用户层', '#2196F3'),
    (13.5, 7.0, '应用层', '#1976D2'),
    (13.5, 5.5, '云端层', '#9C27B0'),
    (13.5, 4.0, '视觉层', '#00BCD4'),
    (13.5, 2.5, '控制层', '#4CAF50'),
    (13.5, 1.0, '执行层', '#FF9800'),
]
for x, y, label, c in layers_info:
    ax.text(x, y, label, fontsize=11, fontweight='bold', color=c, va='center')
    ax.plot([1, 12.8], [y-0.35, y-0.35], color=c, lw=0.5, ls='--', alpha=0.4)

# User layer
rounded_box(ax, (0.8, 7.8), 2.6, 1.1, 'user', '手机 App', fontsize=13)
rounded_box(ax, (3.8, 7.8), 2.6, 1.1, 'user', '触摸屏网页', fontsize=13)
rounded_box(ax, (6.8, 7.8), 2.6, 1.1, 'user', 'Web 网页', fontsize=13)
rounded_box(ax, (9.8, 7.8), 2.4, 1.1, 'user', '串口屏(备用)', fontsize=12)

# Cloud layer
rounded_box(ax, (3, 5.0), 6, 1.1, 'cloud', '云端后端服务 (FastAPI + AI模型)', fontsize=14)
sub_label(ax, 6, 4.55, 'HTTP API  ·  任务队列  ·  Gemini 视觉  ·  MQTT 推送', fontsize=9, color='#7B1FA2')

# Vision layer
rounded_box(ax, (4, 3.45), 4, 1.1, 'vision', 'K230 视觉模块 (摄像头 + WiFi)', fontsize=13)
sub_label(ax, 6, 3.0, '拍照  ·  Base64 上传  ·  AI 识别结果回传', fontsize=9, color='#00838F')

# Control layer
rounded_box(ax, (2.5, 1.9), 3.5, 1.1, 'board1', '主板1 (协调调度)', fontsize=13)
rounded_box(ax, (6.5, 1.9), 3.5, 1.1, 'board2', '主板2 (运动执行)', fontsize=13)
sub_label(ax, 4.25, 1.45, '流程编排 · 6路UART', fontsize=9, color='#2E7D32')
sub_label(ax, 8.25, 1.45, '5轴电机 · 舵机 · 传感器', fontsize=9, color='#E65100')

# Mech layer
rounded_box(ax, (3, 0.3), 6, 1.1, 'mech', '机械执行机构 (导轨 · 夹爪 · 伸缩轴)', fontsize=12)
sub_label(ax, 6, -0.15, 'B区毛刷清理  ·  C区热风护理  ·  挂位限位检测', fontsize=9, color='#455A64')

# Arrows between layers
arrow(ax, 6, 7.8, 6, 6.1, '#9C27B0', 2)
arrow(ax, 8, 6.1, 6, 4.55, '#9C27B0', 2)
arrow(ax, 4, 4.55, 4.25, 3.0, '#00BCD4', 2)
arrow(ax, 8.25, 3.0, 8.25, 2.0, '#4CAF50', 2)

plt.tight_layout()
fig.savefig(os.path.join(OUT, '01_系统架构.png'), dpi=180, bbox_inches='tight', facecolor='#FAFAFA')
plt.close()
print('FIG1 done')

# ============================================================
# FIGURE 2: 硬件连接拓扑 (Hardware Topology)
# ============================================================
fig, ax = plt.subplots(figsize=(16, 9))
ax.set_xlim(0, 16)
ax.set_ylim(0, 9)
ax.axis('off')
ax.set_facecolor('#FAFAFA')

ax.text(8, 8.5, '硬件连接拓扑', ha='center', fontsize=26, fontweight='bold', color='#1A237E')

# Central Board1
cx, cy = 8, 4.5
circle = plt.Circle((cx, cy), 1.3, facecolor='#E8F5E9', edgecolor='#4CAF50', linewidth=3, zorder=10)
ax.add_patch(circle)
ax.text(cx, cy, '主板1\nSTM32F407', ha='center', va='center', fontsize=14, fontweight='bold', color='#2E7D32', zorder=11)

# Surrounding modules
modules = [
    (8, 7.2,    'K230 摄像头\n(UART3)',        '#00BCD4', '#00838F'),
    (2, 4.5,    '激光测距\n(UART4)',            '#FF9800', '#E65100'),
    (11.5, 6.5, '串口屏\n(UART6)',             '#9C27B0', '#6A1B9A'),
    (5, 2.0,    '主板2 执行板\n(UART1 DMA)',    '#FF9800', '#E65100'),
    (11.5, 2.5, 'ESP32-S3\n(UART2) 预留',      '#607D8B', '#37474F'),
]

for mx, my, label, ec, tc in modules:
    b = FancyBboxPatch((mx-1.5, my-0.6), 3, 1.2,
                        boxstyle='round,pad=0.3',
                        facecolor='white', edgecolor=ec, linewidth=2)
    ax.add_patch(b)
    ax.text(mx, my, label, ha='center', va='center', fontsize=11, fontweight='bold', color=tc)
    arrow(ax, cx, cy, mx, my, ec, 1.8)

# Board2 expansion
b2 = FancyBboxPatch((2.5, 0.3), 5, 1.2, boxstyle='round,pad=0.3',
                     facecolor='#FFF3E0', edgecolor='#FF9800', linewidth=1.5)
ax.add_patch(b2)
ax.text(5, 0.9, 'M1~M5 步进电机 (UART2~UART6)', ha='center', fontsize=11, fontweight='bold', color='#E65100')

arrow(ax, 5, 2.0, 5, 1.5, '#FF9800', 1.5)

# Legend
ax.text(1, 7.8, '通信方式：UART 115200 bps / DMA高速帧传输', fontsize=10, color='#666')
ax.text(1, 7.4, '定时调度：TIM7 1ms中断驱动，10ms周期执行', fontsize=10, color='#666')

plt.tight_layout()
fig.savefig(os.path.join(OUT, '02_硬件连接拓扑.png'), dpi=180, bbox_inches='tight', facecolor='#FAFAFA')
plt.close()
print('FIG2 done')

# ============================================================
# FIGURE 3: 核心操作流程 (Operation Flow)
# ============================================================
fig, ax = plt.subplots(figsize=(16, 6))
ax.set_xlim(0, 16)
ax.set_ylim(0, 6)
ax.axis('off')
ax.set_facecolor('#FAFAFA')

ax.text(8, 5.6, '用户取衣完整流程', ha='center', fontsize=26, fontweight='bold', color='#1A237E')

steps = [
    {'x': 0.5, 'label': '① 手机/屏幕\n    选择衣服', 'color': '#E3F2FD', 'ec': '#2196F3'},
    {'x': 3.0, 'label': '② 云端生成\n    取衣任务', 'color': '#F3E5F5', 'ec': '#9C27B0'},
    {'x': 5.5, 'label': '③ 主板轮询\n    获取任务', 'color': '#E8F5E9', 'ec': '#4CAF50'},
    {'x': 8.0, 'label': '④ K230 拍照\n    确认位置', 'color': '#E0F7FA', 'ec': '#00BCD4'},
    {'x': 10.5, 'label': '⑤ 主板2 驱动\n    电机取衣', 'color': '#FFF3E0', 'ec': '#FF9800'},
    {'x': 13.0, 'label': '⑥ 完成回传\n    更新状态', 'color': '#FCE4EC', 'ec': '#E91E63'},
]

for s in steps:
    box = FancyBboxPatch((s['x'], 2.0), 2.2, 2.2, boxstyle='round,pad=0.3',
                          facecolor=s['color'], edgecolor=s['ec'], linewidth=2)
    ax.add_patch(box)
    ax.text(s['x']+1.1, 3.1, s['label'], ha='center', va='center', fontsize=10.5, fontweight='bold', color='#333')

for i in range(len(steps)-1):
    ax.annotate('', xy=(steps[i+1]['x'], 3.1), xytext=(steps[i]['x']+2.2, 3.1),
                arrowprops=dict(arrowstyle='->', color='#888', lw=2.5, connectionstyle='arc3,rad=0'))

# Bottom timing
ax.text(8, 1.2, 'App端 <1s  →  云端 <50ms  →  硬件轮询 1~3s  →  拍照上传 <2s  →  机械执行 <8s  →  全程 <15s', 
        ha='center', fontsize=11, color='#666', style='italic')

plt.tight_layout()
fig.savefig(os.path.join(OUT, '03_操作流程.png'), dpi=180, bbox_inches='tight', facecolor='#FAFAFA')
plt.close()
print('FIG3 done')

# ============================================================
# FIGURE 4: 模块功能概览 (Module Overview)
# ============================================================
fig, ax = plt.subplots(figsize=(16, 8))
ax.set_xlim(0, 16)
ax.set_ylim(0, 8)
ax.axis('off')
ax.set_facecolor('#FAFAFA')

ax.text(8, 7.6, '三大核心模块功能概览', ha='center', fontsize=26, fontweight='bold', color='#1A237E')

# Three columns
col_data = [
    {
        'x': 1.0, 'title': '电控系统', 'color': '#4CAF50', 'bg': '#E8F5E9',
        'items': [
            '双 STM32F407 主控',
            '主板1 流程调度',
            '主板2 运动执行',
            '6 路 UART 通信',
            '5 轴步进电机',
            '激光测距定位',
            'OLED 双板状态',
            'DMA 高速帧传输',
            'TIM7 1ms 中断',
        ]
    },
    {
        'x': 6.0, 'title': '视觉识别', 'color': '#00BCD4', 'bg': '#E0F7FA',
        'items': [
            'K230 CanMV 平台',
            'RISC-V + KPU AI',
            '实时拍照采集',
            'WiFi 直连云端',
            'Base64 图片上传',
            '云端 Gemini 识别',
            '可摆动摄像头',
            '多网络自动配网',
            '结果回传主板',
        ]
    },
    {
        'x': 11.0, 'title': '后端服务', 'color': '#9C27B0', 'bg': '#F3E5F5',
        'items': [
            'FastAPI 框架',
            '任务队列机制',
            'RESTful API',
            '衣物数据库管理',
            'AI 上身预览',
            'MQTT 消息推送',
            '断电任务不丢失',
            '多端状态同步',
            '云端一键部署',
        ]
    },
]

for col in col_data:
    x = col['x']
    # Title box
    box = FancyBboxPatch((x, 6.0), 4.0, 1.0, boxstyle='round,pad=0.2',
                          facecolor=col['color'], edgecolor=col['color'])
    ax.add_patch(box)
    ax.text(x+2, 6.5, col['title'], ha='center', va='center', fontsize=16, fontweight='bold', color='white')
    
    # Items
    for i, item in enumerate(col['items']):
        y = 5.3 - i * 0.58
        bx = FancyBboxPatch((x+0.2, y-0.18), 3.6, 0.5, boxstyle='round,pad=0.1',
                             facecolor=col['bg'], edgecolor=col['color'], linewidth=0.8, alpha=0.7)
        ax.add_patch(bx)
        ax.text(x+2, y+0.07, f'>  {item}', ha='center', va='center', fontsize=10, color='#333')

plt.tight_layout()
fig.savefig(os.path.join(OUT, '04_模块功能概览.png'), dpi=180, bbox_inches='tight', facecolor='#FAFAFA')
plt.close()
print('FIG4 done')

# ============================================================
# FIGURE 5: 电控双板分工 (Board Division)
# ============================================================
fig, ax = plt.subplots(figsize=(16, 7))
ax.set_xlim(0, 16)
ax.set_ylim(0, 7)
ax.axis('off')
ax.set_facecolor('#FAFAFA')

ax.text(8, 6.6, '双板协同工作机制', ha='center', fontsize=26, fontweight='bold', color='#1A237E')

# Board1
rounded_box(ax, (1, 1.5), 6.5, 4.5, 'board1', '', fontsize=13)
ax.text(4.25, 5.0, '主板 1 — 协调调度中心', ha='center', fontsize=18, fontweight='bold', color='#2E7D32')
items1 = [
    '流程编排：接收云端任务 → 调度 K230 拍照 → 下发运动指令',
    '6 路 UART 全分工：K230 / 激光 / 串口屏 / ESP32 / 主板2',
    '激光测距：4 区间判定衣物位置，精准定位',
    'OLED 显示：实时展示距离、区间、系统状态',
    'DMA 通信：高吞吐帧协议与主板2 高效交互',
]
for i, item in enumerate(items1):
    ax.text(4.25, 4.2 - i*0.65, f'>  {item}', ha='center', fontsize=10.5, color='#1B5E20')

# Board2
rounded_box(ax, (8.5, 1.5), 6.5, 4.5, 'board2', '', fontsize=13)
ax.text(11.75, 5.0, '主板 2 — 运动执行单元', ha='center', fontsize=18, fontweight='bold', color='#E65100')
items2 = [
    '5 轴步进电机：独立串口控制，UART2 双电机反向联动',
    '动作列表引擎：预编动作序列，支持取衣/归还/清理/护理',
    '传感器闭环：按键调试、限位开关、回零校准',
    '板间命令解析：接收主板1 运动指令并反馈执行状态',
    '安全保护：急停指令、加速度限幅、超程检测',
]
for i, item in enumerate(items2):
    ax.text(11.75, 4.2 - i*0.65, f'>  {item}', ha='center', fontsize=10.5, color='#BF360C')

# Arrow between
ax.annotate('', xy=(8.5, 3.75), xytext=(7.5, 3.75),
            arrowprops=dict(arrowstyle='<->', color='#FF5722', lw=3))
ax.text(8, 4.15, 'UART1\nDMA', ha='center', fontsize=9, fontweight='bold', color='#FF5722')

# Timeline at bottom
ax.text(8, 0.8, '时序：TIM7 1ms中断 → 每10ms执行一次控制循环 → 双板时钟同步', 
        ha='center', fontsize=11, color='#666', style='italic')

plt.tight_layout()
fig.savefig(os.path.join(OUT, '05_双板协同.png'), dpi=180, bbox_inches='tight', facecolor='#FAFAFA')
plt.close()
print('FIG5 done')

# ============================================================
# FIGURE 6: 视觉识别链路 (Vision Pipeline)
# ============================================================
fig, ax = plt.subplots(figsize=(16, 5.5))
ax.set_xlim(0, 16)
ax.set_ylim(0, 5.5)
ax.axis('off')
ax.set_facecolor('#FAFAFA')

ax.text(8, 5.2, '视觉识别链路', ha='center', fontsize=26, fontweight='bold', color='#1A237E')

v_steps = [
    {'x': 0.3, 'label': '1', 'title': '摄像头采集', 'sub': 'K230 拍照\n320x240 RGB'},
    {'x': 3.0, 'label': '2', 'title': 'WiFi 上传', 'sub': 'JPEG -> Base64\nHTTP POST'},
    {'x': 5.7, 'label': '3', 'title': '云端识别', 'sub': 'Gemini 视觉\nAI 分析衣物'},
    {'x': 8.4, 'label': '4', 'title': '结果入库', 'sub': '款式 / 颜色\n位置关联'},
    {'x': 11.1, 'label': '5', 'title': '任务匹配', 'sub': '取衣指令\n与衣物绑定'},
    {'x': 13.8, 'label': '6', 'title': '执行取衣', 'sub': '主板驱动\n机械手送达'},
]

for s in v_steps:
    box = FancyBboxPatch((s['x'], 1.5), 2.5, 2.8, boxstyle='round,pad=0.3',
                          facecolor='#E0F7FA', edgecolor='#00BCD4', linewidth=1.5, alpha=0.6)
    ax.add_patch(box)
    c = plt.Circle((s['x']+1.25, 3.6), 0.4, facecolor='#00BCD4', edgecolor='white', linewidth=2)
    ax.add_patch(c)
    ax.text(s['x']+1.25, 3.6, s['label'], ha='center', va='center', fontsize=14, fontweight='bold', color='white')
    ax.text(s['x']+1.25, 2.8, s['title'], ha='center', fontsize=12, fontweight='bold', color='#00838F')
    ax.text(s['x']+1.25, 2.0, s['sub'], ha='center', fontsize=9, color='#555')

for i in range(len(v_steps)-1):
    ax.annotate('', xy=(v_steps[i+1]['x'], 2.9), xytext=(v_steps[i]['x']+2.5, 2.9),
                arrowprops=dict(arrowstyle='->', color='#00BCD4', lw=2))

ax.text(8, 0.8, '端侧：K230 RISC-V + KPU  │  云侧：Gemini 视觉模型  │  传输：WiFi 2.4G / HTTP JSON', 
        ha='center', fontsize=11, color='#666', style='italic')

plt.tight_layout()
fig.savefig(os.path.join(OUT, '06_视觉链路.png'), dpi=180, bbox_inches='tight', facecolor='#FAFAFA')
plt.close()
print('FIG6 done')

# ============================================================
# FIGURE 7: 后端任务队列 (Backend Task Queue)
# ============================================================
fig, ax = plt.subplots(figsize=(16, 7))
ax.set_xlim(0, 16)
ax.set_ylim(0, 7)
ax.axis('off')
ax.set_facecolor('#FAFAFA')

ax.text(8, 6.6, '云端任务队列机制', ha='center', fontsize=26, fontweight='bold', color='#1A237E')

# Left: task creation
ax.text(4.25, 5.8, '任务创建', ha='center', fontsize=16, fontweight='bold', color='#9C27B0')
sources = [
    ('手机App\n一键取衣', '#E3F2FD', '#2196F3'),
    ('触摸屏\n点击取衣', '#E3F2FD', '#2196F3'),
    ('AI推荐\n确认取衣', '#F3E5F5', '#9C27B0'),
    ('手动\n归还衣物', '#FFF3E0', '#FF9800'),
]
for i, (text, bg, ec) in enumerate(sources):
    bx = FancyBboxPatch((0.5 + i*2.3, 4.3), 2.0, 1.2, boxstyle='round,pad=0.2',
                          facecolor=bg, edgecolor=ec, linewidth=1.5)
    ax.add_patch(bx)
    ax.text(0.5 + i*2.3 + 1.0, 4.9, text, ha='center', va='center', fontsize=10, fontweight='bold', color='#333')

# Arrow down to queue
ax.annotate('', xy=(4.25, 4.0), xytext=(4.25, 4.3),
            arrowprops=dict(arrowstyle='->', color='#9C27B0', lw=2.5))

# Queue visualization
queue_box = FancyBboxPatch((1, 1.5), 6.5, 2.3, boxstyle='round,pad=0.3',
                            facecolor='#F3E5F5', edgecolor='#9C27B0', linewidth=2)
ax.add_patch(queue_box)
ax.text(4.25, 3.5, '┃  任务队列 (FIFO)    ┃', ha='center', fontsize=14, fontweight='bold', color='#6A1B9A')

tasks = ['取衣: 衬衫-01', '取衣: 外套-03', '归还: 裤子-05', '护理: C区消毒', '取衣: T恤-02']
for i, t in enumerate(tasks):
    color = '#E1BEE7' if i == 0 else '#F3E5F5'
    tk_box = FancyBboxPatch((1.3 + i*1.25, 1.8), 1.1, 0.7, boxstyle='round,pad=0.1',
                             facecolor=color, edgecolor='#CE93D8', linewidth=0.8)
    ax.add_patch(tk_box)
    ax.text(1.3 + i*1.25 + 0.55, 2.15, t, ha='center', va='center', fontsize=7, color='#4A148C')

# Right: execution cycle
ax.text(11.5, 5.8, '硬件执行循环', ha='center', fontsize=16, fontweight='bold', color='#4CAF50')
cycle_steps = [
    '① 主板轮询 GET /api/tasks/pending (每1~3s)',
    '② 取出最早任务，解析任务类型和目标',
    '③ 调度 K230 拍照确认衣物位置',
    '④ 下发运动指令到主板2 执行',
    '⑤ 完成 → POST /api/tasks/{id}/complete',
    '⑥ 云端更新衣物状态，删除已完成任务',
]
for i, step in enumerate(cycle_steps):
    y = 4.5 - i * 0.68
    bx = FancyBboxPatch((8.5, y-0.15), 7.0, 0.55, boxstyle='round,pad=0.1',
                         facecolor='#E8F5E9', edgecolor='#A5D6A7', linewidth=0.8)
    ax.add_patch(bx)
    ax.text(12, y+0.12, step, ha='center', va='center', fontsize=9, color='#1B5E20')

# Key feature
    ax.text(8, 1.0, '> 核心优势：硬件断电/断网 -> 任务仍在云端 -> 恢复后自动继续执行',
        ha='center', fontsize=12, fontweight='bold', color='#C62828')

plt.tight_layout()
fig.savefig(os.path.join(OUT, '07_任务队列.png'), dpi=180, bbox_inches='tight', facecolor='#FAFAFA')
plt.close()
print('FIG7 done')

print(f'\nAll diagrams saved to: {OUT}')
print('Files:')
for f in sorted(os.listdir(OUT)):
    print(f'  {f}')
