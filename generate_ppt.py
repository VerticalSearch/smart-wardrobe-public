from pptx import Presentation
from pptx.util import Inches, Pt, Cm, Emu
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
from pptx.enum.shapes import MSO_SHAPE
import os

DIAGRAM_DIR = r'C:\Users\ECHO\Desktop\家居\ppt_diagrams'
OUTPUT = r'C:\Users\ECHO\Desktop\家居\智能衣橱答辩PPT.pptx'

# Color scheme
C_PRIMARY = RGBColor(0x1A, 0x23, 0x7E)
C_ACCENT  = RGBColor(0x4C, 0xAF, 0x50)
C_DARK    = RGBColor(0x33, 0x33, 0x33)
C_GRAY    = RGBColor(0x88, 0x88, 0x88)
C_WHITE   = RGBColor(0xFF, 0xFF, 0xFF)
C_BG      = RGBColor(0xFA, 0xFA, 0xFA)
C_LINE    = RGBColor(0xE0, 0xE0, 0xE0)

prs = Presentation()
prs.slide_width  = Inches(13.333)
prs.slide_height = Inches(7.5)

SLIDE_W = Inches(13.333)
SLIDE_H = Inches(7.5)

def add_bg(slide, color=C_WHITE):
    bg = slide.background
    fill = bg.fill
    fill.solid()
    fill.fore_color.rgb = color

def add_title_bar(slide, title, subtitle=''):
    # Top bar
    bar = slide.shapes.add_shape(MSO_SHAPE.RECTANGLE, 0, 0, SLIDE_W, Inches(1.1))
    bar.fill.solid()
    bar.fill.fore_color.rgb = C_PRIMARY
    bar.line.fill.background()
    # Title text
    tf = bar.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.text = title
    p.font.size = Pt(32)
    p.font.color.rgb = C_WHITE
    p.font.bold = True
    p.alignment = PP_ALIGN.LEFT
    tf.margin_left = Inches(0.8)
    # Bottom accent line
    line = slide.shapes.add_shape(MSO_SHAPE.RECTANGLE, 0, Inches(1.1), SLIDE_W, Inches(0.05))
    line.fill.solid()
    line.fill.fore_color.rgb = C_ACCENT
    line.line.fill.background()
    if subtitle:
        tb = slide.shapes.add_textbox(Inches(0.8), Inches(1.3), Inches(11), Inches(0.5))
        tf2 = tb.text_frame
        p2 = tf2.paragraphs[0]
        p2.text = subtitle
        p2.font.size = Pt(14)
        p2.font.color.rgb = C_GRAY

def add_body_text(slide, text, left=1, top=1.8, width=11, height=5, size=18, color=C_DARK, bold=False):
    tb = slide.shapes.add_textbox(Inches(left), Inches(top), Inches(width), Inches(height))
    tf = tb.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.text = text
    p.font.size = Pt(size)
    p.font.color.rgb = color
    p.font.bold = bold
    p.alignment = PP_ALIGN.LEFT
    return tf

def add_image(slide, path, left=0, top=1.3, width=None, height=None):
    if not os.path.exists(path):
        return None
    if width is None:
        width = Inches(11.5)
    if height is None:
        height = Inches(5.5)
    return slide.shapes.add_picture(path, Inches(left), Inches(top), width, height)

def add_section_number(slide, num):
    tb = slide.shapes.add_textbox(Inches(0.3), Inches(0.15), Inches(0.5), Inches(0.6))
    tf = tb.text_frame
    p = tf.paragraphs[0]
    p.text = f'{num:02d}'
    p.font.size = Pt(14)
    p.font.color.rgb = C_WHITE
    p.font.bold = True

def bullet_slide(slide, title, items, subtitle='', top=1.8):
    add_title_bar(slide, title, subtitle)
    tb = slide.shapes.add_textbox(Inches(1), Inches(top), Inches(11), Inches(5))
    tf = tb.text_frame
    tf.word_wrap = True
    for i, item in enumerate(items):
        if i == 0:
            p = tf.paragraphs[0]
        else:
            p = tf.add_paragraph()
        p.text = item
        p.font.size = Pt(22)
        p.font.color.rgb = C_DARK
        p.space_after = Pt(18)
        p.level = 0
    return tf

def add_simple_text(slide, text, top=2.5, size=24):
    tb = slide.shapes.add_textbox(Inches(1.5), Inches(top), Inches(10), Inches(3))
    tf = tb.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.text = text
    p.font.size = Pt(size)
    p.font.color.rgb = C_DARK
    p.alignment = PP_ALIGN.CENTER

# ============================================================
# SLIDE 1: Cover
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])  # blank
add_bg(slide, C_PRIMARY)
# Title
tb = slide.shapes.add_textbox(Inches(1.5), Inches(2.2), Inches(10), Inches(1.5))
tf = tb.text_frame
p = tf.paragraphs[0]
p.text = '智能衣橱系统'
p.font.size = Pt(48)
p.font.color.rgb = C_WHITE
p.font.bold = True
p.alignment = PP_ALIGN.CENTER
# Subtitle
tb2 = slide.shapes.add_textbox(Inches(2), Inches(3.8), Inches(9), Inches(1))
tf2 = tb2.text_frame
p2 = tf2.paragraphs[0]
p2.text = '基于云端协同的智能衣物存取与管理系统'
p2.font.size = Pt(22)
p2.font.color.rgb = RGBColor(0xCC, 0xCC, 0xCC)
p2.alignment = PP_ALIGN.CENTER
# Bottom line
line = slide.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(2), Inches(5.2), Inches(9), Inches(0.03))
line.fill.solid()
line.fill.fore_color.rgb = C_ACCENT
line.line.fill.background()
# Bottom text
tb3 = slide.shapes.add_textbox(Inches(2), Inches(5.6), Inches(9), Inches(1))
tf3 = tb3.text_frame
p3 = tf3.paragraphs[0]
p3.text = '毕业设计答辩'
p3.font.size = Pt(18)
p3.font.color.rgb = RGBColor(0xAA, 0xAA, 0xAA)
p3.alignment = PP_ALIGN.CENTER

# ============================================================
# SLIDE 2: Background
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '项目背景与痛点')
bullet_slide(slide, '项目背景与痛点', [
    '传统衣柜依赖人工翻找，查找效率低，衣物易堆积混乱',
    '缺乏智能化管理手段，难以快速定位目标衣物位置',
    '换季收纳、日常取用、衣物护理缺乏一体化解决方案',
    '现有智能家居产品多聚焦安防、照明，衣物管理领域仍属空白',
], top=2.0)

# ============================================================
# SLIDE 3: Goals
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '项目目标')
bullet_slide(slide, '项目目标', [
    '设计并实现一套智能衣橱系统，实现衣物的自动存取、识别与管理',
    '用户通过手机 App / 触摸屏 / Web 即可一键取衣',
    '摄像头拍照 + 云端 AI 自动识别衣物信息并入库',
    '双 STM32 主板协同控制机械装置完成精准取衣动作',
], top=2.0)

# ============================================================
# SLIDE 4: System Architecture
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '系统总体架构')
img_path = os.path.join(DIAGRAM_DIR, '01_系统架构.png')
if os.path.exists(img_path):
    add_image(slide, img_path, left=0.3, top=1.3, width=Inches(12.7), height=Inches(5.8))

# ============================================================
# SLIDE 5: Tech Stack
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '技术栈总览')
# Table-like layout
cols = [
    {'title': '电控', 'items': ['STM32F407 双主板', '6 路 UART 通信', '5 轴步进电机', '激光测距', 'DMA 高速传输', 'TIM7 实时调度']},
    {'title': '视觉', 'items': ['K230 CanMV', 'RISC-V + KPU', '摄像头拍照采集', 'WiFi 直连云端', 'JPEG Base64 上传', '云端 Gemini 识别']},
    {'title': '后端', 'items': ['FastAPI 框架', 'RESTful API', '任务队列机制', 'MQTT 消息推送', 'MySQL 数据库', 'DCloud UniApp']},
]
for i, col in enumerate(cols):
    x = 0.8 + i * 4.1
    # Title
    shape = slide.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, Inches(x), Inches(1.8), Inches(3.7), Inches(0.7))
    shape.fill.solid()
    shape.fill.fore_color.rgb = C_PRIMARY
    shape.line.fill.background()
    shape.text_frame.paragraphs[0].text = col['title']
    shape.text_frame.paragraphs[0].font.size = Pt(20)
    shape.text_frame.paragraphs[0].font.color.rgb = C_WHITE
    shape.text_frame.paragraphs[0].font.bold = True
    shape.text_frame.paragraphs[0].alignment = PP_ALIGN.CENTER
    # Items
    for j, item in enumerate(col['items']):
        y = 2.8 + j * 0.7
        box = slide.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, Inches(x+0.1), Inches(y), Inches(3.5), Inches(0.55))
        box.fill.solid()
        box.fill.fore_color.rgb = RGBColor(0xF5, 0xF5, 0xF5)
        box.line.color.rgb = C_LINE
        box.line.width = Pt(0.5)
        box.text_frame.paragraphs[0].text = f'  {item}'
        box.text_frame.paragraphs[0].font.size = Pt(13)
        box.text_frame.paragraphs[0].font.color.rgb = C_DARK

# ============================================================
# SLIDE 6: Electronic Control - Board Division
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '电控系统 — 双板协同')
img_path = os.path.join(DIAGRAM_DIR, '05_双板协同.png')
if os.path.exists(img_path):
    add_image(slide, img_path, left=0.2, top=1.3, width=Inches(13), height=Inches(5.8))

# ============================================================
# SLIDE 7: Hardware Topology
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '电控系统 — 硬件连接拓扑')
img_path = os.path.join(DIAGRAM_DIR, '02_硬件连接拓扑.png')
if os.path.exists(img_path):
    add_image(slide, img_path, left=0.2, top=1.3, width=Inches(13), height=Inches(5.8))

# ============================================================
# SLIDE 8: Electronic Control Summary
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '电控系统 — 关键技术点')
bullet_slide(slide, '电控系统 — 关键技术点', [
    '双 STM32F407 架构：主板1 负责流程调度与通信，主板2 专注运动执行，职责分离',
    '6 路 UART 全分工：K230 / 激光测距 / 串口屏 / ESP32 / 主板2 各占独立串口，无冲突',
    '5 轴步进电机：独立串口控制，UART2 双电机反向联动实现夹爪开合',
    '激光测距定位：4 区间判定衣物位置，精准引导机械手取衣',
    'TIM7 1ms 中断驱动 + DMA 高速帧传输，确保实时性与数据吞吐',
    'OLED 双板状态显示 + USMART 命令行调试，现场排障方便',
], top=2.0)

# ============================================================
# SLIDE 9: Vision - Pipeline
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '视觉识别 — 完整链路')
img_path = os.path.join(DIAGRAM_DIR, '06_视觉链路.png')
if os.path.exists(img_path):
    add_image(slide, img_path, left=0.2, top=1.5, width=Inches(13), height=Inches(4.5))
# Key points below
tb = slide.shapes.add_textbox(Inches(1), Inches(6.2), Inches(11), Inches(1))
tf = tb.text_frame
p = tf.paragraphs[0]
p.text = '端侧：K230 RISC-V + KPU AI加速    |    云侧：Gemini 视觉模型识别    |    传输：WiFi 2.4G / HTTP JSON'
p.font.size = Pt(13)
p.font.color.rgb = C_GRAY
p.alignment = PP_ALIGN.CENTER

# ============================================================
# SLIDE 10: Vision Summary
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '视觉识别 — 关键技术点')
bullet_slide(slide, '视觉识别 — 关键技术点', [
    'K230 CanMV 视觉模块：集成摄像头 + WiFi，单模块完成图像采集与云端上传',
    '非阻塞 WiFi 配网：支持多候选网络自动扫描，主板1 转发 SSID/密码，掉线自动重连',
    '拍照 → JPEG 压缩 → Base64 编码 → HTTP POST 上传，全链路自动化',
    '云端 Gemini 视觉模型负责衣物识别，返回款式、颜色等结构化信息',
    '可摆动摄像头：舵机控制拍摄角度，适配不同挂位衣物',
    '识别结果回传主板，与取衣任务绑定，实现\"所见即所得\"的精准取衣',
], top=1.8)

# ============================================================
# SLIDE 11: Backend - Task Queue
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '后端服务 — 任务队列机制')
img_path = os.path.join(DIAGRAM_DIR, '07_任务队列.png')
if os.path.exists(img_path):
    add_image(slide, img_path, left=0.2, top=1.3, width=Inches(13), height=Inches(5.8))

# ============================================================
# SLIDE 12: Backend Summary
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '后端服务 — 关键技术点')
bullet_slide(slide, '后端服务 — 关键技术点', [
    'FastAPI + Conda 环境部署，提供 RESTful API 接口，支持多端并发访问',
    '任务队列机制：FIFO 顺序执行，硬件轮询获取任务，完成回写，状态同步',
    '断电/断网容错：任务持久化在云端，硬件恢复后自动继续执行，无需人工干预',
    '衣物数据库管理：存储衣物信息、位置、状态，支持取衣/归还/护理全流程',
    'Gemini AI 集成：衣物视觉识别 + AI 上身效果预览，提升用户体验',
    '多端统一接口：手机 App / 触摸屏 / Web 通过同一 API 与云端交互',
], top=2.0)

# ============================================================
# SLIDE 13: Module Overview
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '三大核心模块概览')
img_path = os.path.join(DIAGRAM_DIR, '04_模块功能概览.png')
if os.path.exists(img_path):
    add_image(slide, img_path, left=0.2, top=1.3, width=Inches(13), height=Inches(5.8))

# ============================================================
# SLIDE 14: Operation Flow
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '用户操作完整流程')
img_path = os.path.join(DIAGRAM_DIR, '03_操作流程.png')
if os.path.exists(img_path):
    add_image(slide, img_path, left=0.3, top=1.5, width=Inches(12.7), height=Inches(4.5))
# Bottom note
tb = slide.shapes.add_textbox(Inches(1), Inches(6.2), Inches(11), Inches(1))
tf = tb.text_frame
p = tf.paragraphs[0]
p.text = 'App 端 <1s  |  云端响应 <50ms  |  硬件轮询 1~3s  |  视觉识别 <2s  |  机械执行 <8s  |  全流程 <15s'
p.font.size = Pt(13)
p.font.color.rgb = C_GRAY
p.alignment = PP_ALIGN.CENTER

# ============================================================
# SLIDE 15: Multi-terminal
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '多端协同控制')
bullet_slide(slide, '多端协同控制', [
    '手机 App（Android APK / UniApp）：远程取衣、AI 推荐、上身效果预览',
    '树莓派触摸屏网页：现场主交互界面，触控操作，响应流畅',
    'Web 网页端：PC 浏览器访问，功能与 App 同步',
    '串口屏（备用）：800x480 TJC 触摸屏，调试配网 + 状态显示',
    '所有终端共享同一云端后端，数据实时同步，操作互不冲突',
], top=2.2)

# ============================================================
# SLIDE 16: Innovation
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '创新点与工程亮点')
bullet_slide(slide, '创新点与工程亮点', [
    '双板协同架构：调度与执行分离，模块化设计，便于扩展和维护',
    'K230 视觉 + WiFi 一体化：单模块实现\"看\"和\"传\"，减少硬件节点',
    '任务队列断电恢复：云端持久化任务，硬件故障不丢指令，业界少见',
    '云边协同：端侧采集 + 云侧 AI，兼顾实时性与计算能力',
    '工程规范：200+ 源文件 0 Error 0 Warning，全量 UTF-8 统一编码',
    '完整文档：技术文档、引脚配置、迭代日志、API 契约一应俱全',
], top=2.0)

# ============================================================
# SLIDE 17: Demo placeholder
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '系统演示')
add_simple_text(slide, '【此处插入演示视频或实物照片】', top=2.5, size=28)
add_simple_text(slide, '演示内容建议：App 选衣 → 云端派任务 → 摄像头拍照 → 机械手取衣 → 送到出口', top=3.8, size=18)

# ============================================================
# SLIDE 18: Scene
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '应用场景与扩展')
bullet_slide(slide, '应用场景与扩展', [
    'B 区毛刷清理：衣物取出后自动清洁除尘',
    'C 区热风护理：负离子 + 通风，衣物除味除菌',
    '挂位限位检测：实时监测衣物在位状态，防止空取',
    'RFID 库存管理（规划中）：衣物标签识别，彻底杜绝错拿',
    '端侧 AI 推理（规划中）：K230 KPU 本地识别，降低云端依赖',
], top=2.2)

# ============================================================
# SLIDE 19: Summary
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide)
add_title_bar(slide, '总结')
bullet_slide(slide, '总结', [
    '完成了一套完整的智能衣橱系统，覆盖电控、视觉、后端、前端四大领域',
    '双 STM32 主板 + K230 摄像头 + 云端后端，形成云边端三级协同架构',
    '实现了从\"用户点击\"到\"机械手出衣\"的全自动闭环流程',
    '任务队列机制保障了系统可靠性，断电断网不丢任务',
    '项目累计产出：PCB 设计、固件代码 200+ 文件、API 接口、技术文档',
], top=2.2)

# ============================================================
# SLIDE 20: Thanks
# ============================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
add_bg(slide, C_PRIMARY)
tb = slide.shapes.add_textbox(Inches(2), Inches(2.5), Inches(9), Inches(1.5))
tf = tb.text_frame
p = tf.paragraphs[0]
p.text = '感谢聆听'
p.font.size = Pt(48)
p.font.color.rgb = C_WHITE
p.font.bold = True
p.alignment = PP_ALIGN.CENTER
tb2 = slide.shapes.add_textbox(Inches(2), Inches(4.2), Inches(9), Inches(1))
tf2 = tb2.text_frame
p2 = tf2.paragraphs[0]
p2.text = '请各位老师批评指正'
p2.font.size = Pt(24)
p2.font.color.rgb = RGBColor(0xCC, 0xCC, 0xCC)
p2.alignment = PP_ALIGN.CENTER

# Save
prs.save(OUTPUT)
print(f'PPT saved to: {OUTPUT}')
print(f'Total slides: {len(prs.slides)}')
