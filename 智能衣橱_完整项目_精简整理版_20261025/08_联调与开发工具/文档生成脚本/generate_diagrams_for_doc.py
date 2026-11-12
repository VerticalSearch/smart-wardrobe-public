import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
import os

plt.rcParams['font.sans-serif'] = ['Microsoft YaHei', 'SimHei']
plt.rcParams['axes.unicode_minus'] = False

OUT = r'C:\Users\ECHO\Desktop\家居\申报书_电控'

C_NAVY  = '#1A237E'; C_GREEN = '#4CAF50'; C_BLUE = '#3B82F6'
C_PURPLE='#8B5CF6'; C_ORANGE='#F59E0B'; C_RED='#EF4444'
C_GRAY1 = '#F1F5F9'; C_GRAY2 = '#E2E8F0'; C_GRAY3 = '#64748B'; C_DARK='#1E293B'

def round_box(ax, x, y, w, h, fill, edge, text='', fs=11, fc='white'):
    box = FancyBboxPatch((x,y), w, h, boxstyle='round,pad=0.3',
                          facecolor=fill, edgecolor=edge, linewidth=2)
    ax.add_patch(box)
    if text:
        ax.text(x+w/2, y+h/2, text, ha='center', va='center', fontsize=fs, fontweight='bold', color=fc)

def arrow(ax, x1,y1,x2,y2, color='#555', lw=2):
    ax.annotate('', xy=(x2,y2), xytext=(x1,y1),
                arrowprops=dict(arrowstyle='->', color=color, lw=lw))

# ============================================================
# FIG 1: 四层架构图
# ============================================================
fig, ax = plt.subplots(figsize=(14, 8))
ax.set_xlim(0,14); ax.set_ylim(0,8); ax.axis('off')
ax.text(7, 7.6, '图1  智能衣橱电控系统四层架构', ha='center', fontsize=18, fontweight='bold', color=C_NAVY)

layers = [
    (1, 6.0, 12, 1.2, C_BLUE,   '#DBEAFE', '意图输入层', '树莓派触摸屏 / App / Web / 云端后端', '产生取衣、放衣、拍照、护理等任务意图'),
    (1, 4.5, 12, 1.2, C_GREEN,  '#DCFCE7', '调度控制层', 'STM32F407 主板1',                       '任务调度 · 流程编排 · 状态判定 · 异常拦截'),
    (1, 3.0, 12, 1.2, C_ORANGE, '#FEF3C7', '运动执行层', 'STM32F407 主板2',                       '5轴电机 · 夹爪舵机 · 伸缩 · 毛刷 · 护理'),
    (1, 1.5, 12, 1.2, C_PURPLE, '#EDE9FE', '状态反馈层', '激光测距 / 限位 / 按键 / 到位信号',      '执行前校验 · 到位判断 · 异常保护'),
]
for x, y, w, h, edge, fill, title, subtitle, desc in layers:
    round_box(ax, x, y, w, h, fill, edge)
    ax.text(x+0.3, y+h-0.35, title, fontsize=14, fontweight='bold', color=edge)
    ax.text(x+0.3, y+h-0.75, f'{subtitle}  —  {desc}', fontsize=10, color=C_GRAY3)

for i in range(3):
    arrow(ax, 7, layers[i][1], 7, layers[i+1][1]+1.2, C_GRAY3, 2)

plt.tight_layout()
fig.savefig(os.path.join(OUT, '图1_四层架构.png'), dpi=180, bbox_inches='tight')
plt.close()

# ============================================================
# FIG 3: 数据流图
# ============================================================
fig, ax = plt.subplots(figsize=(14, 7))
ax.set_xlim(0,14); ax.set_ylim(0,7); ax.axis('off')
ax.text(7, 6.6, '图3  电控系统数据流', ha='center', fontsize=18, fontweight='bold', color=C_NAVY)

nodes = [
    (1, 4.5, 2.2, 1.0, C_BLUE,   '云端任务队列'),
    (3.8, 4.5, 2.2, 1.0, C_GREEN, '主板1 调度'),
    (6.6, 6.0, 2.0, 0.8, C_PURPLE, 'K230 摄像头'),
    (6.6, 3.0, 2.0, 0.8, C_ORANGE, '激光测距'),
    (9.2, 4.5, 2.2, 1.0, C_ORANGE, '主板2 执行'),
    (12, 4.5, 1.5, 1.0, C_RED,    '电机/舵机'),
]
for x, y, w, h, c, text in nodes:
    round_box(ax, x, y, w, h, 'white', c, text, fs=10, fc=C_DARK)

arrow(ax, 3.2, 5.0, 3.8, 5.0, C_BLUE, 2)
arrow(ax, 5.3, 5.7, 6.6, 5.8, C_PURPLE, 1.5)
arrow(ax, 7.6, 5.6, 5.3, 5.3, C_PURPLE, 1.5)
arrow(ax, 6.0, 4.5, 9.2, 4.5, C_GREEN, 2)
arrow(ax, 11.4, 4.5, 12.0, 4.5, C_ORANGE, 2)

# Labels
ax.text(3.5, 5.3, 'HTTP 轮询', fontsize=9, color=C_GRAY3, ha='center')
ax.text(5.9, 6.2, 'UART3 拍照/识别', fontsize=9, color=C_GRAY3, ha='center')
ax.text(7.6, 4.2, 'UART1 DMA 帧', fontsize=9, color=C_GRAY3, ha='center')
# Laser flow
ax.text(6.6, 4.0, 'UART4', fontsize=8, color=C_GRAY3, ha='center')
arrow(ax, 7.6, 3.4, 6.0, 4.0, C_ORANGE, 1.5)

# Timer
ax.text(2, 2.0, 'TIM7 1ms中断驱动，10ms控制周期', fontsize=10, color=C_GRAY3, style='italic')

plt.tight_layout()
fig.savefig(os.path.join(OUT, '图3_数据流.png'), dpi=180, bbox_inches='tight')
plt.close()

# ============================================================
# FIG 5: 安全保护层级
# ============================================================
fig, ax = plt.subplots(figsize=(14, 6))
ax.set_xlim(0,14); ax.set_ylim(0,6); ax.axis('off')
ax.text(7, 5.6, '图5  三级安全保护机制', ha='center', fontsize=18, fontweight='bold', color=C_NAVY)

levels = [
    ('执行前校验', C_GREEN, ['检查任务类型合法性', '确认目标挂位状态', '机构安全状态检查', '传感器在线验证']),
    ('执行中监测', C_BLUE,  ['电机到位反馈+超时', '限位开关软件边界', '激光测距实时避障', '动作超时视为故障']),
    ('异常处理',   C_RED,   ['急停最高优先级', '超时→回零待命', '限位触发→反向脱离', '通信超时→待机']),
]
for i, (title, color, items) in enumerate(levels):
    x = 1.0 + i * 4.2
    round_box(ax, x, 0.5, 3.5, 4.5, C_GRAY1, color, '', fs=10)
    ax.text(x+1.75, 4.6, title, ha='center', fontsize=14, fontweight='bold', color=color)
    for j, item in enumerate(items):
        y = 3.8 - j * 0.8
        round_box(ax, x+0.3, y, 2.9, 0.6, 'white', color, item, fs=9, fc=C_DARK)

# Arrows between
for i in range(2):
    x1 = 4.5 + i*4.2
    arrow(ax, x1, 2.8, x1+0.7, 2.8, C_GRAY3, 2)

plt.tight_layout()
fig.savefig(os.path.join(OUT, '图5_安全保护.png'), dpi=180, bbox_inches='tight')
plt.close()

# ============================================================
# FIG 4b: XY同步轨迹对比
# ============================================================
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
ax1.set_xlim(0,10); ax1.set_ylim(0,10)
ax2.set_xlim(0,10); ax2.set_ylim(0,10)
ax1.set_aspect('equal'); ax2.set_aspect('equal')
ax1.set_title('不同步：折线轨迹', fontsize=12, fontweight='bold', color=C_RED)
ax2.set_title('同步到点：平滑轨迹', fontsize=12, fontweight='bold', color=C_GREEN)

# Left: unsynchronized
ax1.plot([1,6,6,9], [1,1,9,9], 'o-', color=C_RED, lw=2.5, markersize=8)
ax1.text(1,0.5,'起点', ha='center', fontsize=9, color=C_DARK)
ax1.text(9,9.5,'终点', ha='center', fontsize=9, color=C_DARK)
ax1.text(3.5,0.5,'先斜向', ha='center', fontsize=7, color=C_GRAY3)
ax1.text(7.5,5,'再横向', ha='center', fontsize=7, color=C_GRAY3)
ax1.plot([6],[1],'o', color='orange', markersize=10, alpha=0.5)
ax1.text(6.3,0.7,'短轴停止点', fontsize=7, color=C_GRAY3)
# Boundary
ax1.plot([0,4,4,0,0],[0,0,3,3,0], '--', color=C_RED, alpha=0.3, lw=1)
ax1.text(2,3.3,'柜体边界', fontsize=7, color=C_RED, alpha=0.5)

# Right: synchronized
ax2.plot([1,9], [1,9], 'o-', color=C_GREEN, lw=2.5, markersize=8)
ax2.text(1,0.5,'起点', ha='center', fontsize=9, color=C_DARK)
ax2.text(9,9.5,'终点', ha='center', fontsize=9, color=C_DARK)
ax2.text(5,4.5,'连续平滑', ha='center', fontsize=9, color=C_GREEN, fontweight='bold')
ax2.plot([0,4,4,0,0],[0,0,3,3,0], '--', color=C_GREEN, alpha=0.3, lw=1)

for ax in [ax1, ax2]:
    ax.set_xticks([]); ax.set_yticks([])
    for spine in ax.spines.values(): spine.set_visible(False)

fig.suptitle('图4b  X/Y双轴同步到点轨迹对比', fontsize=16, fontweight='bold', color=C_NAVY, y=1.02)
fig.savefig(os.path.join(OUT, '图4b_XY轨迹对比.png'), dpi=180, bbox_inches='tight')
plt.close()

print('All 4 diagrams saved to', OUT)
