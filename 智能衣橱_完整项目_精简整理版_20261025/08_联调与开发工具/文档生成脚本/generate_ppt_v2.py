from pptx import Presentation
from pptx.util import Inches, Pt, Emu
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
from pptx.enum.shapes import MSO_SHAPE, MSO_CONNECTOR_TYPE
from pptx.oxml.ns import qn
import os, copy

# ─── Color Palette ─────────────────────────────────
NAVY   = RGBColor(0x0F, 0x17, 0x2A)
DARK   = RGBColor(0x1E, 0x29, 0x3B)
WHITE  = RGBColor(0xFF, 0xFF, 0xFF)
GRAY_1 = RGBColor(0xF1, 0xF5, 0xF9)
GRAY_2 = RGBColor(0xE2, 0xE8, 0xF0)
GRAY_3 = RGBColor(0x94, 0xA3, 0xB8)
GRAY_4 = RGBColor(0x64, 0x74, 0x8B)
GRAY_5 = RGBColor(0x33, 0x41, 0x55)
GREEN  = RGBColor(0x10, 0xB9, 0x81)
BLUE   = RGBColor(0x3B, 0x82, 0xF6)
PURPLE = RGBColor(0x8B, 0x5C, 0xF6)
ORANGE = RGBColor(0xF5, 0x9E, 0x0B)
CYAN   = RGBColor(0x06, 0xB6, 0xD4)
RED    = RGBColor(0xEF, 0x44, 0x44)

DIAGRAM_DIR = r'C:\Users\ECHO\Desktop\家居\ppt_diagrams'
OUTPUT = r'C:\Users\ECHO\Desktop\家居\智能衣橱答辩PPT_v2.pptx'

prs = Presentation()
prs.slide_width  = Inches(13.333)
prs.slide_height = Inches(7.5)
W = prs.slide_width
H = prs.slide_height

# ─── Helpers ───────────────────────────────────────
def new_slide():
    return prs.slides.add_slide(prs.slide_layouts[6])

def bg(slide, color=WHITE):
    slide.background.fill.solid()
    slide.background.fill.fore_color.rgb = color

def rect(slide, l, t, w, h, fill=NAVY, border=None, radius=None):
    s = slide.shapes.add_shape(
        MSO_SHAPE.ROUNDED_RECTANGLE if radius else MSO_SHAPE.RECTANGLE,
        Inches(l), Inches(t), Inches(w), Inches(h))
    s.fill.solid()
    s.fill.fore_color.rgb = fill
    if border:
        s.line.color.rgb = border
        s.line.width = Pt(0.5)
    else:
        s.line.fill.background()
    return s

def circle(slide, l, t, d, fill):
    s = slide.shapes.add_shape(MSO_SHAPE.OVAL, Inches(l), Inches(t), Inches(d), Inches(d))
    s.fill.solid()
    s.fill.fore_color.rgb = fill
    s.line.fill.background()
    return s

def text(slide, l, t, w, h, txt, size=16, color=DARK, bold=False, align=PP_ALIGN.LEFT, font='Microsoft YaHei'):
    tb = slide.shapes.add_textbox(Inches(l), Inches(t), Inches(w), Inches(h))
    tf = tb.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.text = txt
    p.font.size = Pt(size)
    p.font.color.rgb = color
    p.font.bold = bold
    p.font.name = font
    p.alignment = align
    return tf

def multi_text(slide, l, t, w, h, lines, size=14, color=DARK, spacing=Pt(8)):
    tb = slide.shapes.add_textbox(Inches(l), Inches(t), Inches(w), Inches(h))
    tf = tb.text_frame
    tf.word_wrap = True
    for i, line in enumerate(lines):
        p = tf.paragraphs[0] if i == 0 else tf.add_paragraph()
        p.text = line
        p.font.size = Pt(size)
        p.font.color.rgb = color
        p.font.name = 'Microsoft YaHei'
        p.space_after = spacing

def img(slide, path, l, t, w, h):
    if os.path.exists(path):
        return slide.shapes.add_picture(path, Inches(l), Inches(t), Inches(w), Inches(h))
    return None

def section_label(slide, label, color):
    """Small colored label like [ 电控 ] in top-left"""
    bar = rect(slide, 0.5, 0.35, 1.6, 0.32, fill=color, radius=0.16)
    bar.text_frame.paragraphs[0].text = label
    bar.text_frame.paragraphs[0].font.size = Pt(10)
    bar.text_frame.paragraphs[0].font.color.rgb = WHITE
    bar.text_frame.paragraphs[0].font.bold = True
    bar.text_frame.paragraphs[0].alignment = PP_ALIGN.CENTER

def page_title(slide, title, subtitle=None):
    text(slide, 0.5, 0.85, 12, 0.7, title, size=30, color=DARK, bold=True)
    if subtitle:
        text(slide, 0.5, 1.45, 12, 0.4, subtitle, size=13, color=GRAY_4)

def bottom_bar(slide, color=GRAY_1):
    rect(slide, 0, 7.3, 13.333, 0.2, fill=color)

def card(slide, l, t, w, h, title, items, accent, icon_text=''):
    """A card with accent left border"""
    # Card background
    rect(slide, l, t, w, h, fill=GRAY_1, border=GRAY_2, radius=0.15)
    # Accent left stripe
    rect(slide, l, t, 0.06, h, fill=accent)
    # Icon circle
    if icon_text:
        c = circle(slide, l+0.35, t+0.3, 0.55, fill=accent)
        c.text_frame.paragraphs[0].text = icon_text
        c.text_frame.paragraphs[0].font.size = Pt(14)
        c.text_frame.paragraphs[0].font.color.rgb = WHITE
        c.text_frame.paragraphs[0].font.bold = True
        c.text_frame.paragraphs[0].alignment = PP_ALIGN.CENTER
    # Title
    text(slide, l+1.1, t+0.25, w-1.3, 0.45, title, size=16, color=DARK, bold=True)
    # Items
    for i, item in enumerate(items):
        text(slide, l+1.1, t+0.75+i*0.45, w-1.3, 0.4, f'— {item}', size=11, color=GRAY_4)

def placeholder_frame(slide, l, t, w, h, label, color=GRAY_3):
    """Dashed border placeholder for photos"""
    s = slide.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(l), Inches(t), Inches(w), Inches(h))
    s.fill.solid()
    s.fill.fore_color.rgb = GRAY_1
    s.line.color.rgb = color
    s.line.width = Pt(1.5)
    s.line.dash_style = 2  # dash
    # icon
    text(slide, l+w/2-1, t+h/2-0.6, 2, 0.6, '📷', size=36, color=GRAY_3, align=PP_ALIGN.CENTER)
    text(slide, l+w/2-2, t+h/2+0.2, 4, 0.6, label, size=14, color=GRAY_3, align=PP_ALIGN.CENTER)

def flow_arrow(slide, x1, y1, x2, y2, color=GRAY_4):
    connector = slide.shapes.add_connector(
        MSO_CONNECTOR_TYPE.STRAIGHT, Inches(x1), Inches(y1), Inches(x2), Inches(y2))
    connector.line.color.rgb = color
    connector.line.width = Pt(2)
    # arrowhead
    connector.line._ln.append(
        qn('a:tailEnd') if False else None)

def arrow_right(slide, x, y, w, color=GRAY_4):
    """Simple right arrow shape"""
    s = slide.shapes.add_shape(MSO_SHAPE.RIGHT_ARROW, Inches(x), Inches(y), Inches(w), Inches(0.3))
    s.fill.solid()
    s.fill.fore_color.rgb = color
    s.line.fill.background()

def pill_box(slide, l, t, w, h, txt, fill, txt_color=WHITE, size=13):
    s = rect(slide, l, t, w, h, fill=fill, radius=h/2)
    s.text_frame.paragraphs[0].text = txt
    s.text_frame.paragraphs[0].font.size = Pt(size)
    s.text_frame.paragraphs[0].font.color.rgb = txt_color
    s.text_frame.paragraphs[0].font.bold = True
    s.text_frame.paragraphs[0].alignment = PP_ALIGN.CENTER
    s.text_frame.paragraphs[0].font.name = 'Microsoft YaHei'
    return s


# ══════════════════════════════════════════════════════
# SLIDE 1 : Cover
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl, NAVY)
# Decorative shapes
rect(sl, 0, 0, 13.333, 0.08, fill=GREEN)
circle(sl, 10.5, 0.8, 2.5, fill=RGBColor(0x1A, 0x23, 0x3F))
circle(sl, 11.2, 4.5, 1.2, fill=RGBColor(0x1A, 0x23, 0x3F))
text(sl, 1.5, 1.8, 10, 1.2, '智能衣橱系统', size=52, color=WHITE, bold=True)
text(sl, 1.5, 3.1, 10, 0.8, '基于云端协同的智能衣物存取与管理系统', size=20, color=GRAY_3)
rect(sl, 1.5, 4.2, 2.5, 0.04, fill=GREEN)
text(sl, 1.5, 4.7, 10, 0.6, '毕业设计答辩', size=16, color=GRAY_4)
text(sl, 1.5, 5.4, 10, 0.5, '项目技术团队', size=13, color=GRAY_4)

# ══════════════════════════════════════════════════════
# SLIDE 2 : 项目背景
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, 'BACKGROUND', NAVY)
page_title(sl, '项目背景', '传统衣柜的痛点催生智能化需求')
bottom_bar(sl)

problems = [
    ('😰', '查找困难', '翻箱倒柜找一件衣服\n平均耗时 3-5 分钟'),
    ('😵', '管理混乱', '衣物堆积、褶皱\n换季收纳耗时费力'),
    ('🤷', '缺乏智能', '传统衣柜是被动容器\n无法主动服务用户'),
    ('📉', '市场空白', '智能家居聚焦安防照明\n衣物管理仍是无人区'),
]
for i, (icon, title, desc) in enumerate(problems):
    x = 0.6 + i * 3.15
    card(sl, x, 2.4, 2.9, 3.5, title, desc.split('\n'), 
         [NAVY, BLUE, PURPLE, CYAN][i], icon_text=icon)

# ══════════════════════════════════════════════════════
# SLIDE 3 : 项目目标
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, 'GOAL', NAVY)
page_title(sl, '我们做了什么', '一套完整的智能衣橱：从用户点击到衣服到手')
bottom_bar(sl)

goals = [
    ('📱', BLUE, '多端操控', '手机App · 触摸屏\nWeb网页 · 串口屏'),
    ('👁', PURPLE, '视觉识别', 'K230摄像头拍照\n云端AI自动识别衣物'),
    ('⚙', GREEN, '自动存取', '双STM32主板驱动\n机械手精准取衣'),
    ('☁', CYAN, '云端管理', '任务队列 · 数据库\nAI上身预览 · 多端同步'),
]
for i, (icon, accent, title, desc) in enumerate(goals):
    x = 0.6 + i * 3.15
    c = card(sl, x, 2.4, 2.9, 3.8, title, desc.split('\n'), accent, icon_text=icon)

# ══════════════════════════════════════════════════════
# SLIDE 4 : 系统架构总览
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, 'ARCHITECTURE', NAVY)
page_title(sl, '系统架构总览')
bottom_bar(sl)

img_path = os.path.join(DIAGRAM_DIR, '01_系统架构.png')
if os.path.exists(img_path):
    img(sl, img_path, 0.4, 1.9, 12.5, 5.2)

# ══════════════════════════════════════════════════════
# SLIDE 5 : 电控 > PCB实物
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '电控 · PCB', GREEN)
page_title(sl, '自研电路板', '从原理图到PCB打样，全部自主完成')
bottom_bar(sl, GREEN)

# Two placeholder frames
placeholder_frame(sl, 0.6, 2.3, 5.8, 4.3, '主板1 协调调度板\n实物照片', GRAY_3)
placeholder_frame(sl, 6.9, 2.3, 5.8, 4.3, '主板2 运动执行板\n实物照片', GRAY_3)

# Key stats below
stats = [('STM32F407', '双板主控'), ('6路UART', '全分工通信'), ('5轴电机', '独立控制'), ('自行打样', '2次迭代')]
for i, (k, v) in enumerate(stats):
    x = 0.6 + i * 3.15
    rect(sl, x, 6.9, 2.9, 0.25, fill=GRAY_1)
    text(sl, x+0.2, 6.85, 2.5, 0.35, f'{k}  —  {v}', size=9, color=GRAY_4, align=PP_ALIGN.CENTER)

# ══════════════════════════════════════════════════════
# SLIDE 6 : 电控 > 双板分工
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '电控 · 架构', GREEN)
page_title(sl, '双板协同：一块管"想"，一块管"做"')
bottom_bar(sl, GREEN)

# Board 1
rect(sl, 0.6, 2.5, 5.8, 4.2, fill=GRAY_1, border=GREEN, radius=0.2)
rect(sl, 0.6, 2.5, 5.8, 0.65, fill=GREEN)
text(sl, 3.5, 2.55, 5, 0.5, '主板 1  —  协调调度中心', size=18, color=WHITE, bold=True, align=PP_ALIGN.CENTER)
items1 = [
    '接收云端任务，编排执行流程',
    '调度 K230 摄像头拍照识别',
    '6 路 UART 连接全部外设',
    'DMA 高速帧协议与主板2 通信',
    'OLED 实时显示系统运行状态',
]
for i, item in enumerate(items1):
    rect(sl, 0.9, 3.4+i*0.65, 5.2, 0.5, fill=WHITE, border=GRAY_2, radius=0.08)
    text(sl, 1.2, 3.43+i*0.65, 4.8, 0.45, item, size=12, color=DARK)

# Board 2
rect(sl, 6.9, 2.5, 5.8, 4.2, fill=GRAY_1, border=ORANGE, radius=0.2)
rect(sl, 6.9, 2.5, 5.8, 0.65, fill=ORANGE)
text(sl, 9.8, 2.55, 5, 0.5, '主板 2  —  运动执行单元', size=18, color=WHITE, bold=True, align=PP_ALIGN.CENTER)
items2 = [
    '5 轴步进电机独立串口控制',
    'UART2 双电机反向联动（夹爪）',
    '预编动作序列：取衣/归还/护理',
    '接收主板1 指令并反馈执行状态',
    '急停保护 + 加速度限幅 + 回零',
]
for i, item in enumerate(items2):
    rect(sl, 7.2, 3.4+i*0.65, 5.2, 0.5, fill=WHITE, border=GRAY_2, radius=0.08)
    text(sl, 7.5, 3.43+i*0.65, 4.8, 0.45, item, size=12, color=DARK)

# Arrow between
arrow_right(sl, 6.55, 4.6, 0.2, GREEN)

# ══════════════════════════════════════════════════════
# SLIDE 7 : 电控 > UART拓扑
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '电控 · 通信', GREEN)
page_title(sl, '6 路 UART 全分工，互不冲突')
bottom_bar(sl, GREEN)

img_path = os.path.join(DIAGRAM_DIR, '02_硬件连接拓扑.png')
if os.path.exists(img_path):
    img(sl, img_path, 0.4, 1.9, 12.5, 5.2)

# ══════════════════════════════════════════════════════
# SLIDE 8 : 电控 > 动作闭环
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '电控 · 执行', GREEN)
page_title(sl, '从"看到"到"拿到"：激光测距 → 精准取衣')
bottom_bar(sl, GREEN)

steps_data = [
    ('🔍', BLUE, '激光测距', 'UART4 采集\n距离数据'),
    ('📊', PURPLE, '区间判定', '4 个预设区间\n判断衣物位置'),
    ('📋', NAVY, '匹配任务', '云端任务\n+ 位置信息'),
    ('⚡', ORANGE, '电机驱动', '5轴步进电机\n精准移动'),
    ('✅', GREEN, '取衣完成', '夹爪闭合\n衣物送出'),
]

for i, (icon, color, title, desc) in enumerate(steps_data):
    x = 0.5 + i * 2.5
    # Card
    rect(sl, x, 2.5, 2.2, 3.5, fill=GRAY_1, border=color, radius=0.15)
    rect(sl, x, 2.5, 2.2, 0.6, fill=color)
    text(sl, x+1.1, 2.55, 2, 0.5, title, size=14, color=WHITE, bold=True, align=PP_ALIGN.CENTER)
    # Icon
    circle(sl, x+0.65, 3.4, 0.9, fill=color)
    s = circle(sl, x+0.65, 3.4, 0.9, fill=color)
    s.text_frame.paragraphs[0].text = icon
    s.text_frame.paragraphs[0].font.size = Pt(22)
    s.text_frame.paragraphs[0].alignment = PP_ALIGN.CENTER
    # Desc
    multi_text(sl, x+0.2, 4.5, 1.8, 1.2, desc.split('\n'), size=11, color=GRAY_4, spacing=Pt(4))
    # Arrow
    if i < 4:
        arrow_right(sl, x+2.3, 4.3, 0.15, GRAY_3)

rect(sl, 0.6, 6.5, 12, 0.5, fill=GRAY_1, radius=0.25)
text(sl, 6.6, 6.55, 11, 0.4, '闭环链路：激光测距 → 区间判定 → 任务匹配 → 电机驱动 → 取衣完成', 
     size=12, color=GRAY_5, bold=True, align=PP_ALIGN.CENTER)

# ══════════════════════════════════════════════════════
# SLIDE 9 : 视觉 > K230实物
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '视觉 · 硬件', BLUE)
page_title(sl, 'K230 CanMV 视觉模块', 'RISC-V 双核 + KPU AI加速，摄像头+WiFi一体化')
bottom_bar(sl, BLUE)

placeholder_frame(sl, 0.6, 2.3, 5.5, 4.3, 'K230 摄像头模块实物照片', GRAY_3)

# Specs on the right
specs = [
    ('芯片架构', 'RISC-V 64 位双核'),
    ('AI 算力', 'KPU 神经网络加速器'),
    ('摄像头', '320×240 实时采集'),
    ('联网方式', 'WiFi 2.4G 直连云端'),
    ('通信接口', 'UART 与主板1 通信'),
    ('安装方式', '舵机驱动可摆动云台'),
]
for i, (k, v) in enumerate(specs):
    y = 2.3 + i * 0.7
    rect(sl, 6.8, y, 5.8, 0.55, fill=GRAY_1, border=GRAY_2, radius=0.1)
    text(sl, 7.0, y+0.05, 1.8, 0.45, k, size=12, color=GRAY_4, bold=True)
    text(sl, 8.8, y+0.05, 3.5, 0.45, v, size=13, color=DARK, bold=True)

# ══════════════════════════════════════════════════════
# SLIDE 10 : 视觉 > 识别结果
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '视觉 · 识别', BLUE)
page_title(sl, '拍照 → AI 识别：证明链路通了')
bottom_bar(sl, BLUE)

# Before
rect(sl, 0.6, 2.5, 5.5, 4.3, fill=GRAY_1, border=GRAY_2, radius=0.15)
text(sl, 3.35, 2.1, 5, 0.4, '拍摄的衣物照片', size=13, color=GRAY_4, align=PP_ALIGN.CENTER)
placeholder_frame(sl, 0.9, 2.8, 4.9, 3.7, '摄像头拍摄的衣物原图', BLUE)

# Arrow
arrow_right(sl, 6.2, 4.6, 0.3, BLUE)

# After
rect(sl, 7.2, 2.5, 5.5, 4.3, fill=GRAY_1, border=BLUE, radius=0.15)
text(sl, 9.95, 2.1, 5, 0.4, '云端返回的识别结果', size=13, color=GRAY_4, align=PP_ALIGN.CENTER)
placeholder_frame(sl, 7.5, 2.8, 4.9, 3.7, 'AI 识别返回 JSON 截图', BLUE)

# ══════════════════════════════════════════════════════
# SLIDE 11 : 视觉 > 拍照到入库流程
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '视觉 · 流程', BLUE)
page_title(sl, '拍照到入库：全自动，3 秒完成')
bottom_bar(sl, BLUE)

flow = [
    ('📸', 'K230 拍照', '320×240 JPEG', BLUE),
    ('📡', 'WiFi 上传', 'Base64 编码\nHTTP POST', PURPLE),
    ('🧠', '云端识别', 'Gemini 视觉模型\n分析款式颜色', CYAN),
    ('💾', '存入数据库', '衣物信息 + 位置\n自动归档', GREEN),
]

for i, (icon, title, desc, color) in enumerate(flow):
    x = 0.8 + i * 3.1
    rect(sl, x, 2.5, 2.7, 3.5, fill=GRAY_1, border=color, radius=0.15)
    c = circle(sl, x+0.85, 2.8, 1.0, fill=color)
    c.text_frame.paragraphs[0].text = icon
    c.text_frame.paragraphs[0].font.size = Pt(24)
    c.text_frame.paragraphs[0].alignment = PP_ALIGN.CENTER
    text(sl, x+1.35, 4.0, 2.5, 0.5, title, size=16, color=color, bold=True, align=PP_ALIGN.CENTER)
    multi_text(sl, x+0.3, 4.6, 2.1, 1.0, desc.split('\n'), size=11, color=GRAY_4, spacing=Pt(4))
    if i < 3:
        arrow_right(sl, x+2.85, 4.3, 0.15, GRAY_3)

rect(sl, 0.6, 6.4, 12, 0.5, fill=GRAY_1, radius=0.25)
text(sl, 6.6, 6.45, 11, 0.4, '端侧采集 + 云侧 AI  =  兼顾实时性与识别精度', 
     size=12, color=GRAY_5, bold=True, align=PP_ALIGN.CENTER)

# ══════════════════════════════════════════════════════
# SLIDE 12 : 后端 > API接口
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '后端 · API', PURPLE)
page_title(sl, '后端服务已上线运行', 'FastAPI + 腾讯云服务器，RESTful API')
bottom_bar(sl, PURPLE)

placeholder_frame(sl, 0.6, 2.3, 7.5, 4.5, 'Swagger 文档截图\n或 Postman 返回 JSON', PURPLE)

# Right side: key APIs
apis = [
    ('GET', '/api/wardrobe', '获取衣橱列表'),
    ('POST', '/api/wardrobe/add', '拍照入库'),
    ('GET', '/api/tasks/pending', '轮询任务'),
    ('POST', '/api/tasks/{id}/complete', '完成任务'),
    ('POST', '/api/hardware/fetch', '发起取衣'),
    ('POST', '/api/ai/generate', 'AI上身预览'),
]
for i, (method, path, desc) in enumerate(apis):
    y = 2.3 + i * 0.7
    rect(sl, 8.8, y, 4.0, 0.55, fill=GRAY_1, border=GRAY_2, radius=0.1)
    method_color = GREEN if method == 'GET' else BLUE
    pill_box(sl, 8.9, y+0.08, 0.7, 0.38, method, method_color, WHITE, 9)
    text(sl, 9.7, y+0.05, 1.8, 0.45, path, size=10, color=DARK, bold=True)
    text(sl, 11.3, y+0.05, 1.3, 0.45, desc, size=10, color=GRAY_4)

# ══════════════════════════════════════════════════════
# SLIDE 13 : 后端 > 任务队列
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '后端 · 队列', PURPLE)
page_title(sl, '任务队列：断电也不丢任务')
bottom_bar(sl, PURPLE)

img_path = os.path.join(DIAGRAM_DIR, '07_任务队列.png')
if os.path.exists(img_path):
    img(sl, img_path, 0.4, 1.9, 12.5, 5.2)

# ══════════════════════════════════════════════════════
# SLIDE 14 : 后端 > 多端截图
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '后端 · 多端', PURPLE)
page_title(sl, '一套后端，多端共用')
bottom_bar(sl, PURPLE)

placeholder_frame(sl, 0.6, 2.3, 3.8, 4.3, 'App 界面截图', BLUE)
placeholder_frame(sl, 4.8, 2.3, 3.8, 4.3, '触摸屏网页截图', GREEN)
placeholder_frame(sl, 9.0, 2.3, 3.8, 4.3, 'Web 端截图', PURPLE)

# ══════════════════════════════════════════════════════
# SLIDE 15 : 串联 > 全链路
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '全链路', NAVY)
page_title(sl, '从点击到到手：一条完整的自动化链路')
bottom_bar(sl)

img_path = os.path.join(DIAGRAM_DIR, '03_操作流程.png')
if os.path.exists(img_path):
    img(sl, img_path, 0.4, 1.9, 12.5, 4.5)

# Time breakdown
times = [('App点击', '<1s', GREEN), ('云端响应', '<50ms', PURPLE), 
         ('AI识别', '<2s', BLUE), ('机械执行', '<8s', ORANGE), ('全程', '<15s', RED)]
for i, (label, t, color) in enumerate(times):
    x = 0.8 + i * 2.5
    pill_box(sl, x, 6.6, 1.8, 0.55, f'{label}  {t}', color, WHITE, 13)

# ══════════════════════════════════════════════════════
# SLIDE 16 : 串联 > 演示
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, '演示', RED)
page_title(sl, '系统演示', '实际运行效果最有说服力')
bottom_bar(sl, RED)

placeholder_frame(sl, 0.8, 2.3, 11.7, 4.5, '【此处插入演示视频或实物运行照片】\n\n建议内容：用户点击取衣 → 机械手移动到目标位置 → 夹取衣物 → 送至出口', RED)

# ══════════════════════════════════════════════════════
# SLIDE 17 : 创新点
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, 'HIGHLIGHTS', NAVY)
page_title(sl, '创新点与工程亮点')
bottom_bar(sl)

innovations = [
    ('🧩', GREEN, '双板解耦架构', '调度与执行分离\n模块独立，易扩展维护'),
    ('📷', BLUE, '视觉识别闭环', 'K230拍照 + 云端AI\n\"所见即所得\"取衣'),
    ('🔌', PURPLE, '任务队列容错', '断电断网不丢任务\n业界少见的高可靠设计'),
    ('🌐', ORANGE, '云边端协同', '端采集 + 边控制 + 云AI\n三级架构，各取所长'),
]
for i, (icon, color, title, desc) in enumerate(innovations):
    x = 0.6 + i * 3.15
    c = card(sl, x, 2.2, 2.9, 2.8, title, desc.split('\n'), color, icon_text=icon)

# Engineering highlights
rect(sl, 0.6, 5.3, 12.1, 1.5, fill=GRAY_1, border=GRAY_2, radius=0.15)
text(sl, 1.0, 5.45, 11, 0.4, '工程规范', size=14, color=DARK, bold=True)
items = [
    '200+ 源文件，0 Error / 0 Warning 编译通过',
    '48 轮迭代日志，全量 UTF-8 编码统一',
    '完整技术文档：引脚配置、协议规范、API契约、部署指南'
]
for i, item in enumerate(items):
    text(sl, 1.2, 5.9+i*0.3, 11, 0.3, f'>  {item}', size=11, color=GRAY_4)

# ══════════════════════════════════════════════════════
# SLIDE 18 : 总结
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl)
section_label(sl, 'SUMMARY', NAVY)
page_title(sl, '总结与展望')
bottom_bar(sl)

# Three columns
summaries = [
    ('✅', GREEN, '已完成', [
        '双STM32主板固件',
        'K230视觉+WiFi上传',
        '云端后端API服务',
        '多端控制界面',
        'PCB设计+打样',
        '任务队列机制',
    ]),
    ('🔧', BLUE, '可扩展', [
        'RFID标签库存管理',
        'K230端侧AI推理',
        '更多衣物材质识别',
        '能耗优化与监控',
        '语音控制接入',
        '多衣橱组网互联',
    ]),
    ('📦', PURPLE, '项目产出', [
        'PCB设计文件',
        '固件源码 200+文件',
        '云端API接口文档',
        'App APK安装包',
        '技术答辩PPT',
        '完整开发日志',
    ]),
]

for i, (icon, color, title, items) in enumerate(summaries):
    x = 0.6 + i * 4.1
    rect(sl, x, 2.3, 3.7, 4.7, fill=GRAY_1, border=color, radius=0.15)
    rect(sl, x, 2.3, 3.7, 0.65, fill=color)
    text(sl, x+1.85, 2.35, 3.5, 0.5, title, size=16, color=WHITE, bold=True, align=PP_ALIGN.CENTER)
    for j, item in enumerate(items):
        y = 3.2 + j * 0.58
        text(sl, x+0.3, y, 3.1, 0.5, f'— {item}', size=12, color=DARK)

# ══════════════════════════════════════════════════════
# SLIDE 19 : Thanks
# ══════════════════════════════════════════════════════
sl = new_slide()
bg(sl, NAVY)
rect(sl, 0, 0, 13.333, 0.08, fill=GREEN)
circle(sl, -1, 5, 3, fill=RGBColor(0x1A, 0x23, 0x3F))
circle(sl, 11, -1, 2, fill=RGBColor(0x1A, 0x23, 0x3F))

text(sl, 4, 2.5, 5, 1, '感谢聆听', size=48, color=WHITE, bold=True, align=PP_ALIGN.CENTER)
rect(sl, 5.5, 3.7, 2.3, 0.04, fill=GREEN)
text(sl, 4, 4.2, 5, 0.8, '请各位老师批评指正', size=22, color=GRAY_3, align=PP_ALIGN.CENTER)

# ─── Save ────────────────────────────────────────────
prs.save(OUTPUT)
print(f'PPT saved: {OUTPUT}')
print(f'Slides: {len(prs.slides)}')
