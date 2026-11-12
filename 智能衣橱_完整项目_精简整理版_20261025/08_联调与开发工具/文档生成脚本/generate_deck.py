#!/usr/bin/env python3
"""Generate deck.json for 智能衣橱系统 graduation defense PPT (Excalidraw Slides format)."""

import json
from pathlib import Path

OUTPUT = Path(r"C:\Users\ECHO\Desktop\家居\deck.json")
DIAGRAM_DIR = Path(r"C:\Users\ECHO\Desktop\家居\ppt_diagrams")

STYLE = {
    "bg": "#fffdf7",
    "text": "#1a1a2e",
    "text_muted": "#495057",
    "roughness": 1.5,
    "stroke_width": 2,
}

# ACCENT_COLORS order matches render_slides.py: Blue=0, Green=1, Orange=2, Purple=3, Red=4, Cyan=5
# The user's color-to-index mapping: Blue=0, Green=1, Purple→3, Orange→2, Red=4, Cyan=5
BLUE, GREEN, ORANGE, PURPLE, RED, CYAN = 0, 1, 2, 3, 4, 5

COLOR_HEX = {
    0: ("#1971c2", "#d0ebff", "#1864ab"),
    1: ("#2f9e44", "#d3f9d8", "#2b8a3e"),
    2: ("#e8590c", "#ffe8cc", "#d9480f"),
    3: ("#7048e8", "#e5dbff", "#6741d9"),
    4: ("#c92a2a", "#ffe3e3", "#b02525"),
    5: ("#0c8599", "#c3fae8", "#0b7285"),
}


def ac(idx):
    return COLOR_HEX[idx][0]

def acl(idx):
    return COLOR_HEX[idx][1]

def acd(idx):
    return COLOR_HEX[idx][2]


# --- Element helpers ---

def textbox(x, y, w, h, paragraphs, valign="top"):
    return {"type": "textbox", "x": x, "y": y, "w": w, "h": h, "paragraphs": paragraphs, "valign": valign}


def image(x, y, w, h, src=None, label="", bg_color=None, border_color=None):
    elem = {"type": "image", "x": x, "y": y, "w": w, "h": h, "label": label}
    if src:
        elem["src"] = src
    if bg_color:
        elem["bg_color"] = bg_color
    if border_color:
        elem["border_color"] = border_color
    return elem


def p(text, font="body", size=32, color=None, bold=False, align="left"):
    r = {"text": text, "font": font, "size": size, "align": align}
    if color:
        r["color"] = color
    if bold:
        r["bold"] = True
    return r


def ph(text, font="heading", size=56, color=None, bold=True, align="center"):
    r = {"text": text, "font": font, "size": size, "align": align, "bold": bold}
    if color:
        r["color"] = color
    return r


def pa(text, size=28, color=None, align="center"):
    r = {"text": text, "font": "annotation", "size": size, "align": align}
    if color:
        r["color"] = color
    return r


# --- CSS fragments ---

CSS_RESET = "#content{padding:0!important;width:1920px;height:1080px;}"

CARD_CSS = """
.card{position:absolute;display:flex;flex-direction:column;align-items:center;justify-content:center;
text-align:center;padding:40px;border-radius:20px;}
.card h3{font-size:48px;margin-bottom:24px;color:var(--accent);}
.card p{font-size:32px;line-height:1.5;color:var(--text);}
.card ul{list-style:none;padding:0;}
.card ul li{font-size:30px;margin-bottom:12px;color:var(--text);padding-left:0;}
.card ul li::before{content:none;}
"""

GRID_CSS = CSS_RESET + CARD_CSS

SPLIT_CSS = CSS_RESET + """
.split-left{position:absolute;top:200px;left:80px;width:820px;height:700px;
display:flex;align-items:center;justify-content:center;border-radius:20px;
border:2px dashed var(--text-muted);}
.split-left img{width:100%;height:100%;object-fit:contain;border-radius:18px;}
.split-right{position:absolute;top:200px;left:960px;width:880px;height:700px;
display:flex;flex-direction:column;justify-content:center;padding:20px 40px;}
.split-right h3{font-size:44px;margin-bottom:30px;color:var(--accent);}
.split-right ul{list-style:none;padding:0;}
.split-right ul li{font-size:34px;margin-bottom:20px;padding-left:32px;position:relative;}
.split-right ul li::before{content:'';position:absolute;left:0;top:14px;width:14px;height:14px;
border-radius:50%;background:var(--accent);}
.split-right p{font-size:32px;line-height:1.6;color:var(--text);}
"""

COMPARE_CSS = CSS_RESET + CARD_CSS + """
.compare-left{position:absolute;top:200px;left:80px;width:700px;height:700px;
display:flex;flex-direction:column;align-items:center;justify-content:center;
text-align:center;padding:40px;border-radius:20px;}
.compare-right{position:absolute;top:200px;left:1140px;width:700px;height:700px;
display:flex;flex-direction:column;align-items:center;justify-content:center;
text-align:center;padding:40px;border-radius:20px;}
.compare-arrow{position:absolute;top:500px;left:820px;width:280px;height:100px;
display:flex;align-items:center;justify-content:center;}
.compare-arrow span{font-size:60px;color:var(--accent);}
.compare-left h3,.compare-right h3{font-size:48px;margin-bottom:30px;color:var(--accent);}
.compare-left ul,.compare-right ul{list-style:none;padding:0;text-align:left;}
.compare-left ul li,.compare-right ul li{font-size:30px;margin-bottom:16px;
color:var(--text);padding-left:28px;position:relative;}
.compare-left ul li::before,.compare-right ul li::before{content:'';position:absolute;
left:0;top:10px;width:12px;height:12px;border-radius:50%;background:var(--accent);}
"""

FLOW_CSS = CSS_RESET + """
.flow-row{position:absolute;top:340px;left:80px;width:1760px;display:flex;
justify-content:space-around;align-items:center;}
.flow-step{width:280px;height:280px;display:flex;flex-direction:column;
align-items:center;justify-content:center;text-align:center;padding:20px;border-radius:20px;}
.flow-step .step-num{font-size:48px;color:var(--accent);font-family:var(--font-heading);
font-weight:700;margin-bottom:12px;}
.flow-step .step-text{font-size:30px;color:var(--text);line-height:1.4;}
"""

THREE_COL_CSS = CSS_RESET + """
.col-row{position:absolute;top:220px;left:80px;width:1760px;display:flex;
justify-content:space-between;}
.col-card{width:520px;height:660px;display:flex;flex-direction:column;
align-items:center;justify-content:center;text-align:center;padding:30px;border-radius:20px;}
.col-card h3{font-size:44px;margin-bottom:30px;color:var(--accent);}
.col-card p,.col-card ul li{font-size:30px;line-height:1.6;color:var(--text);}
.col-card ul{list-style:none;padding:0;text-align:center;}
.col-card ul li{margin-bottom:14px;}
.col-card ul li::before{content:none;}
"""

CLOSE_CSS = CSS_RESET + """
.close-center{position:absolute;top:0;left:0;width:1920px;height:1080px;
display:flex;flex-direction:column;align-items:center;justify-content:center;}
"""


# --- shapes_js fragments ---

def page_border(stroke="STYLE.accent", strokeWidth=3):
    return f"""drawRect(80, 80, 1760, 920, {{stroke: {stroke}, strokeWidth: {strokeWidth}}});
drawRect(100, 100, 1720, 880, {{stroke: STYLE.accentLight, strokeWidth: 1, roughness: 1}});"""


def corner_accents(cards, color="STYLE.accent"):
    """Draw corner accents on each card rect. cards = [(x,y,w,h), ...]"""
    lines = []
    for (cx, cy, cw, ch) in cards:
        L = 40
        # top-left
        lines.append(f"drawLine({cx},{cy},{cx+L},{cy},{{stroke:{color},strokeWidth:3}});")
        lines.append(f"drawLine({cx},{cy},{cx},{cy+L},{{stroke:{color},strokeWidth:3}});")
        # top-right
        x2 = cx + cw
        lines.append(f"drawLine({x2},{cy},{x2-L},{cy},{{stroke:{color},strokeWidth:3}});")
        lines.append(f"drawLine({x2},{cy},{x2},{cy+L},{{stroke:{color},strokeWidth:3}});")
        # bottom-left
        y2 = cy + ch
        lines.append(f"drawLine({cx},{y2},{cx+L},{y2},{{stroke:{color},strokeWidth:3}});")
        lines.append(f"drawLine({cx},{y2},{cx},{y2-L},{{stroke:{color},strokeWidth:3}});")
        # bottom-right
        lines.append(f"drawLine({x2},{y2},{x2-L},{y2},{{stroke:{color},strokeWidth:3}});")
        lines.append(f"drawLine({x2},{y2},{x2},{y2-L},{{stroke:{color},strokeWidth:3}});")
    return "\n".join(lines)


def card_border(x, y, w, h, stroke="STYLE.accent"):
    return f"drawRect({x},{y},{w},{h},{{stroke:{stroke},strokeWidth:2.5,roughness:1.8}});"


def arrow_between(x1, y, x2, stroke="STYLE.accent"):
    """Horizontal arrow from x1 to x2 at y"""
    return f"""drawLine({x1},{y},{x2},{y},{{stroke:{stroke},strokeWidth:3}});
drawLine({x2},{y},{x2-16},{y-12},{{stroke:{stroke},strokeWidth:3}});
drawLine({x2},{y},{x2-16},{y+12},{{stroke:{stroke},strokeWidth:3}});"""


def arrow_right(x1, y1, x2, y2, stroke="STYLE.accent"):
    return f"drawLine({x1},{y1},{x2},{y2},{{stroke:{stroke},strokeWidth:3,roughness:1.5}});"


def underline_deco(x, y, w, stroke="STYLE.accent"):
    return f"drawLine({x},{y},{x+w},{y},{{stroke:{stroke},strokeWidth:3,roughness:2}});"


def rough_circle(cx, cy, d, stroke="STYLE.accent"):
    return f"drawCircle({cx},{cy},{d},{{stroke:{stroke},strokeWidth:2.5,roughness:2}});"


# --- HTML fragments ---

def slide_title(title, subtitle=None):
    sub = f'<p class="annotation" style="font-size:28px;margin-top:8px;">{subtitle}</p>' if subtitle else ""
    return f"""<div style="position:absolute;top:30px;left:80px;width:1760px;text-align:center;">
<h2>{title}</h2>{sub}</div>"""


def img_tag(path, w="100%", h="100%", fit="contain"):
    """Generate img tag with file:// URL for local images."""
    url = path.as_uri()
    return f'<img src="{url}" style="width:{w};height:{h};object-fit:{fit};border-radius:18px;">'


# --- Slide builders ---

def slide_1_cover():
    html = f"""<div style="display:flex;flex-direction:column;align-items:center;justify-content:center;height:1080px;text-align:center;">
<h1 style="font-size:100px;color:var(--accent);margin-bottom:30px;letter-spacing:12px;">智能衣橱系统</h1>
<p style="font-size:42px;color:var(--text-muted);margin-bottom:200px;">基于云端协同的智能衣物存取与管理系统</p>
<p class="annotation" style="font-size:30px;color:var(--text);">毕业设计答辩</p>
</div>"""
    return {
        "id": 1, "render_mode": "dual", "accent_index": BLUE,
        "html": html, "css": CSS_RESET,
        "shapes_js": page_border(),
        "elements": [
            textbox(80, 180, 1760, 160, [ph("智能衣橱系统", size=100, color=ac(BLUE), align="center")], "middle"),
            textbox(80, 380, 1760, 80, [p("基于云端协同的智能衣物存取与管理系统", size=40, color="#495057", align="center")], "middle"),
            textbox(80, 860, 1760, 60, [pa("毕业设计答辩", size=30, color="#1a1a2e", align="center")], "middle"),
        ],
    }


def slide_2_background():
    cards = [
        ("🔍", "查找困难", "传统衣柜衣物堆积，翻找耗时\n经常找不到想穿的衣服"),
        ("😵", "管理混乱", "换季衣物混放，重复购买\n无法掌握衣柜存量状况"),
        ("📱", "缺乏智能", "无远程控制与自动分类\n每次翻找全靠人工记忆"),
        ("🏠", "市场空白", "现有产品价格高昂\n缺乏面向大众的解决方案"),
    ]
    html_parts = [slide_title("项目背景与痛点分析")]
    positions = [(130, 220, 780, 370), (1010, 220, 780, 370), (130, 630, 780, 370), (1010, 630, 780, 370)]
    for i, ((cx, cy, cw, ch), (icon, title, desc)) in enumerate(zip(positions, cards)):
        desc_html = desc.replace("\n", "<br>")
        html_parts.append(f"""<div class="card" style="left:{cx}px;top:{cy}px;width:{cw}px;height:{ch}px;">
<div style="font-size:64px;margin-bottom:16px;">{icon}</div>
<h3>{title}</h3>
<p>{desc_html}</p></div>""")

    return {
        "id": 2, "render_mode": "dual", "accent_index": BLUE,
        "html": "\n".join(html_parts),
        "css": GRID_CSS,
        "shapes_js": corner_accents(positions) + "\n" + underline_deco(580, 110, 700),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("项目背景与痛点分析", size=56, color="#1a1a2e", align="center")], "middle"),
            textbox(170, 280, 700, 200, [ph("查找困难", size=48, color=ac(BLUE), align="center"), p("传统衣柜衣物堆积，翻找耗时", size=28, align="center"), p("经常找不到想穿的衣服", size=28, align="center")], "middle"),
            textbox(1050, 280, 700, 200, [ph("管理混乱", size=48, color=ac(BLUE), align="center"), p("换季衣物混放，重复购买", size=28, align="center"), p("无法掌握衣柜存量状况", size=28, align="center")], "middle"),
            textbox(170, 690, 700, 200, [ph("缺乏智能", size=48, color=ac(BLUE), align="center"), p("无远程控制与自动分类", size=28, align="center"), p("每次翻找全靠人工记忆", size=28, align="center")], "middle"),
            textbox(1050, 690, 700, 200, [ph("市场空白", size=48, color=ac(BLUE), align="center"), p("现有产品价格高昂", size=28, align="center"), p("缺乏面向大众的解决方案", size=28, align="center")], "middle"),
        ],
    }


def slide_3_goals():
    cards = [
        ("📱", "多端操控", "手机App / 触摸屏\n随时远程控制衣橱"),
        ("👁️", "视觉识别", "K230摄像头拍照\nAI云端识别衣物"),
        ("🤖", "自动存取", "双STM32主板驱动\n5轴机械手精准取衣"),
        ("☁️", "云端管理", "FastAPI后端服务\n任务队列持久化"),
    ]
    html_parts = [slide_title("系统设计目标")]
    positions = [(130, 220, 780, 370), (1010, 220, 780, 370), (130, 630, 780, 370), (1010, 630, 780, 370)]
    for i, ((cx, cy, cw, ch), (icon, title, desc)) in enumerate(zip(positions, cards)):
        desc_html = desc.replace("\n", "<br>")
        html_parts.append(f"""<div class="card" style="left:{cx}px;top:{cy}px;width:{cw}px;height:{ch}px;">
<div style="font-size:64px;margin-bottom:16px;">{icon}</div>
<h3>{title}</h3>
<p>{desc_html}</p></div>""")

    return {
        "id": 3, "render_mode": "dual", "accent_index": GREEN,
        "html": "\n".join(html_parts),
        "css": GRID_CSS,
        "shapes_js": corner_accents(positions) + "\n" + underline_deco(580, 110, 700),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("系统设计目标", size=56, color="#1a1a2e", align="center")], "middle"),
            textbox(170, 280, 700, 200, [ph("多端操控", size=48, color=ac(GREEN), align="center"), p("手机App / 触摸屏", size=28, align="center"), p("随时远程控制衣橱", size=28, align="center")], "middle"),
            textbox(1050, 280, 700, 200, [ph("视觉识别", size=48, color=ac(GREEN), align="center"), p("K230摄像头拍照", size=28, align="center"), p("AI云端识别衣物", size=28, align="center")], "middle"),
            textbox(170, 690, 700, 200, [ph("自动存取", size=48, color=ac(GREEN), align="center"), p("双STM32主板驱动", size=28, align="center"), p("5轴机械手精准取衣", size=28, align="center")], "middle"),
            textbox(1050, 690, 700, 200, [ph("云端管理", size=48, color=ac(GREEN), align="center"), p("FastAPI后端服务", size=28, align="center"), p("任务队列持久化", size=28, align="center")], "middle"),
        ],
    }


def slide_4_architecture():
    arch_img = DIAGRAM_DIR / "01_系统架构.png"
    html = f"""{slide_title("系统整体架构")}
<div class="split-left">{img_tag(arch_img)}</div>
<div class="split-right">
<h3>系统拓扑</h3>
<p style="margin-bottom:20px;">端—云—边 三层协同架构</p>
<ul>
<li>手机 / 触控屏幕</li>
<li>云端 FastAPI 服务器</li>
<li>K230 CanMV 摄像头</li>
<li>双 STM32F407 主板</li>
<li>5轴步进电机机械手</li>
<li>激光测距精准定位</li>
</ul>
</div>"""
    return {
        "id": 4, "render_mode": "dual", "accent_index": BLUE,
        "html": html, "css": SPLIT_CSS,
        "shapes_js": page_border(),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("系统整体架构", size=56, color="#1a1a2e", align="center")], "middle"),
            image(80, 200, 820, 700, src=str(arch_img), label="系统架构图", border_color=ac(BLUE)),
            textbox(960, 220, 880, 650, [
                ph("系统拓扑", size=44, color=ac(BLUE), align="left"),
                p("端—云—边 三层协同架构", font="body", size=30, color="#495057", align="left"),
                p(""), p("手机 / 触控屏幕", size=34, align="left"),
                p("云端 FastAPI 服务器", size=34, align="left"),
                p("K230 CanMV 摄像头", size=34, align="left"),
                p("双 STM32F407 主板", size=34, align="left"),
                p("5轴步进电机机械手", size=34, align="left"),
                p("激光测距精准定位", size=34, align="left"),
            ], "top"),
        ],
    }


def slide_5_pcb():
    html = f"""{slide_title("硬件实物展示")}
<div class="split-left" style="left:960px;">
{img_tag(DIAMAGR_DIR / "01_系统架构.png" if False else None) if False else '<p class="annotation" style="font-size:24px;">自研PCB电路板<br>双STM32F407主板<br>自行设计、打样、焊接</p>'}
</div>
<div class="split-right" style="left:80px;width:820px;">
<h3>自研PCB电路板</h3>
<p style="margin-bottom:16px;">双 STM32F407 主板</p>
<p style="margin-bottom:16px;">兼容6路UART通信接口</p>
<p style="margin-bottom:16px;">集成步进电机驱动模块</p>
<p style="margin-bottom:16px;">激光测距传感器接口</p>
<p>OLED显示屏状态监控</p>
</div>"""
    # Use two divs - left text, right placeholder
    html = f"""{slide_title("硬件实物展示")}
<div class="split-left" style="left:80px;width:820px;">
<h3>自研PCB电路板</h3>
<p style="margin-bottom:16px;">双 STM32F407 主板</p>
<p style="margin-bottom:16px;">兼容6路UART通信接口</p>
<p style="margin-bottom:16px;">集成步进电机驱动模块</p>
<p style="margin-bottom:16px;">激光测距传感器接口</p>
<p>OLED显示屏状态监控</p>
</div>
<div class="split-left" style="left:960px;width:820px;border:2px dashed var(--text-muted);display:flex;align-items:center;justify-content:center;">
<p class="annotation" style="font-size:28px;color:var(--text-muted);">[ 自研PCB电路板实拍照片 ]</p>
</div>"""
    return {
        "id": 5, "render_mode": "dual", "accent_index": GREEN,
        "html": html, "css": SPLIT_CSS.replace("border:2px dashed var(--text-muted);", ""),
        "shapes_js": page_border(stroke="STYLE.accent", strokeWidth=2.5),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("硬件实物展示", size=56, color="#1a1a2e", align="center")], "middle"),
            textbox(80, 250, 820, 550, [
                ph("自研PCB电路板", size=44, color=ac(GREEN), align="left"),
                p(""), p("双 STM32F407 主板", size=34, align="left"),
                p("兼容6路UART通信接口", size=34, align="left"),
                p("集成步进电机驱动模块", size=34, align="left"),
                p("激光测距传感器接口", size=34, align="left"),
                p("OLED显示屏状态监控", size=34, align="left"),
            ], "top"),
            image(960, 200, 820, 700, label="自研PCB电路板实拍照片", bg_color=acl(GREEN), border_color=ac(GREEN)),
        ],
    }


def slide_6_dual_board():
    html = f"""{slide_title("双板协同架构")}
<div class="compare-left" style="background:var(--accent-light);border:2px dashed var(--accent);">
<h3 style="color:var(--accent-dark);">主板1 — 协调调度</h3>
<ul>
<li>任务流程编排</li>
<li>6路UART通信</li>
<li>激光测距定位</li>
<li>OLED状态显示</li>
<li>WiFi云端轮询</li>
</ul>
</div>
<div class="compare-arrow"><span>→</span></div>
<div class="compare-right" style="background:var(--accent-light);border:2px dashed var(--accent);">
<h3 style="color:var(--accent-dark);">主板2 — 运动执行</h3>
<ul>
<li>5轴步进电机</li>
<li>夹爪联动控制</li>
<li>动作序列解析</li>
<li>回零校准机制</li>
<li>限位安全保护</li>
</ul>
</div>"""
    return {
        "id": 6, "render_mode": "dual", "accent_index": GREEN,
        "html": html, "css": COMPARE_CSS,
        "shapes_js": card_border(80, 200, 700, 700, "STYLE.accent") + "\n" +
                      card_border(1140, 200, 700, 700, "STYLE.accent") + "\n" +
                      arrow_between(800, 550, 1120, "STYLE.accent"),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("双板协同架构", size=56, color="#1a1a2e", align="center")], "middle"),
            textbox(120, 240, 620, 600, [
                ph("主板1 — 协调调度", size=42, color=acd(GREEN), align="center"),
                p(""), p("任务流程编排", size=32, align="center"),
                p("6路UART通信", size=32, align="center"),
                p("激光测距定位", size=32, align="center"),
                p("OLED状态显示", size=32, align="center"),
                p("WiFi云端轮询", size=32, align="center"),
            ], "middle"),
            textbox(1180, 240, 620, 600, [
                ph("主板2 — 运动执行", size=42, color=acd(GREEN), align="center"),
                p(""), p("5轴步进电机", size=32, align="center"),
                p("夹爪联动控制", size=32, align="center"),
                p("动作序列解析", size=32, align="center"),
                p("回零校准机制", size=32, align="center"),
                p("限位安全保护", size=32, align="center"),
            ], "middle"),
        ],
    }


def slide_7_uart_topology():
    img = DIAGRAM_DIR / "02_硬件连接拓扑.png"
    html = f"""<div style="position:absolute;top:80px;left:80px;width:1760px;height:920px;">
{img_tag(img, "100%", "100%", "contain")}
</div>"""
    return {
        "id": 7, "render_mode": "image_only", "accent_index": GREEN,
        "html": html, "css": CSS_RESET,
        "shapes_js": page_border(stroke="STYLE.accent", strokeWidth=2),
        "elements": [],
    }


def slide_8_action_loop():
    steps = ["激光测距", "区间判定", "任务匹配", "电机驱动", "取衣完成"]
    step_html = ""
    for i, step in enumerate(steps):
        step_html += f"""<div class="flow-step" style="background:var(--accent-light);border:2px dashed var(--accent);">
<div class="step-num">0{i+1}</div>
<div class="step-text">{step}</div></div>"""

    html = f"""{slide_title("机械手动作流程")}
<div class="flow-row">{step_html}</div>"""

    # Arrows between steps: 5 steps, 4 arrows
    # Steps positioned at x = 80 + i*440, width=280 centered
    step_centers = [(80 + i * 440 + 140, 480) for i in range(5)]
    arrows_js = ""
    for i in range(4):
        x1 = step_centers[i][0] + 140
        x2 = step_centers[i+1][0] - 140
        y = 480
        arrows_js += arrow_between(x1, y, x2) + "\n"

    # Also add downward arrow label
    html += """<div style="position:absolute;bottom:220px;left:80px;width:1760px;text-align:center;">
<p class="annotation" style="font-size:26px;color:var(--text-muted);">驱动 5 轴步进电机 + 夹爪机构，精准取放衣物</p></div>"""

    return {
        "id": 8, "render_mode": "dual", "accent_index": GREEN,
        "html": html, "css": FLOW_CSS,
        "shapes_js": arrows_js + underline_deco(500, 110, 900),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("机械手动作流程", size=56, color="#1a1a2e", align="center")], "middle"),
            textbox(120, 400, 280, 160, [ph("激光测距", size=40, color=ac(GREEN), align="center")], "middle"),
            textbox(560, 400, 280, 160, [ph("区间判定", size=40, color=ac(GREEN), align="center")], "middle"),
            textbox(1000, 400, 280, 160, [ph("任务匹配", size=40, color=ac(GREEN), align="center")], "middle"),
            textbox(1440, 400, 280, 160, [ph("电机驱动", size=40, color=ac(GREEN), align="center")], "middle"),
            textbox(120, 640, 1640, 160, [ph("取衣完成", size=40, color=acd(GREEN), align="center")], "middle"),
            textbox(80, 810, 1760, 50, [pa("驱动 5 轴步进电机 + 夹爪机构，精准取放衣物", size=26, color="#495057", align="center")], "middle"),
        ],
    }


def slide_9_k230_camera():
    html = f"""{slide_title("K230 CanMV 视觉模块")}
<div class="split-left" style="border:2px dashed var(--text-muted);display:flex;align-items:center;justify-content:center;">
<p class="annotation" style="font-size:28px;color:var(--text-muted);">[ K230 CanMV 摄像头模块实拍照片 ]</p>
</div>
<div class="split-right">
<h3>硬件规格</h3>
<ul>
<li>RISC-V 双核处理器</li>
<li>KPU AI 神经网络加速</li>
<li>320×240 图像采集</li>
<li>WiFi 直连云服务器</li>
<li>舵机云台角度控制</li>
<li>MicroPython 开发</li>
</ul>
</div>"""
    return {
        "id": 9, "render_mode": "dual", "accent_index": PURPLE,
        "html": html, "css": SPLIT_CSS,
        "shapes_js": page_border(stroke="STYLE.accent", strokeWidth=2.5),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("K230 CanMV 视觉模块", size=56, color="#1a1a2e", align="center")], "middle"),
            image(80, 200, 820, 700, label="K230 CanMV 摄像头模块实拍照片", bg_color=acl(PURPLE), border_color=ac(PURPLE)),
            textbox(960, 250, 880, 600, [
                ph("硬件规格", size=44, color=ac(PURPLE), align="left"),
                p(""), p("RISC-V 双核处理器", size=34, align="left"),
                p("KPU AI 神经网络加速", size=34, align="left"),
                p("320×240 图像采集", size=34, align="left"),
                p("WiFi 直连云服务器", size=34, align="left"),
                p("舵机云台角度控制", size=34, align="left"),
                p("MicroPython 开发", size=34, align="left"),
            ], "top"),
        ],
    }


def slide_10_recognition():
    html = f"""{slide_title("衣物视觉识别")}
<div class="compare-left" style="background:var(--accent-light);border:2px dashed var(--accent);display:flex;align-items:center;justify-content:center;">
<p class="annotation" style="font-size:28px;color:var(--text-muted);">[ K230 拍摄衣物照片 ]</p>
</div>
<div class="compare-arrow"><span>→</span></div>
<div class="compare-right" style="background:var(--accent-light);border:2px dashed var(--accent);display:flex;align-items:center;justify-content:center;">
<p class="annotation" style="font-size:28px;color:var(--text-muted);">[ AI 云端识别结果 ]</p>
</div>"""
    return {
        "id": 10, "render_mode": "dual", "accent_index": PURPLE,
        "html": html, "css": COMPARE_CSS,
        "shapes_js": card_border(80, 200, 700, 700, "STYLE.accent") + "\n" +
                      card_border(1140, 200, 700, 700, "STYLE.accent") + "\n" +
                      arrow_between(800, 550, 1120, "STYLE.accent"),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("衣物视觉识别", size=56, color="#1a1a2e", align="center")], "middle"),
            image(80, 200, 700, 700, label="K230 拍摄衣物照片", bg_color=acl(PURPLE), border_color=ac(PURPLE)),
            image(1140, 200, 700, 700, label="AI 云端识别结果", bg_color=acl(PURPLE), border_color=ac(PURPLE)),
            textbox(820, 520, 280, 60, [p("→", size=48, color=ac(PURPLE), align="center")], "middle"),
        ],
    }


def slide_11_vision_flow():
    steps = ["K230拍照", "WiFi上传", "云端识别", "存入数据库"]
    step_html = ""
    for i, step in enumerate(steps):
        step_html += f"""<div class="flow-step" style="background:var(--accent-light);border:2px dashed var(--accent);">
<div class="step-num">0{i+1}</div>
<div class="step-text">{step}</div></div>"""

    html = f"""{slide_title("视觉识别链路")}
<div class="flow-row" style="top:360px;">{step_html}</div>
<div style="position:absolute;bottom:260px;left:80px;width:1760px;text-align:center;">
<p class="annotation" style="font-size:26px;color:var(--text-muted);">K230捕获图像 → 实时上传 → AI模型推理 → 结构化存储</p></div>"""

    # 4 steps, 3 arrows; step width 360, gap
    # x positions: 80, 540, 1000, 1460 (each 460 apart)
    arrow_js = ""
    for i in range(3):
        x1 = 80 + i * 460 + 360 + 10
        x2 = 80 + (i + 1) * 460 - 10
        arrow_js += arrow_between(x1, 500, x2) + "\n"

    return {
        "id": 11, "render_mode": "dual", "accent_index": PURPLE,
        "html": html, "css": FLOW_CSS,
        "shapes_js": arrow_js + underline_deco(500, 110, 900),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("视觉识别链路", size=56, color="#1a1a2e", align="center")], "middle"),
            textbox(120, 420, 360, 160, [ph("K230拍照", size=40, color=ac(PURPLE), align="center")], "middle"),
            textbox(580, 420, 360, 160, [ph("WiFi上传", size=40, color=ac(PURPLE), align="center")], "middle"),
            textbox(1040, 420, 360, 160, [ph("云端识别", size=40, color=ac(PURPLE), align="center")], "middle"),
            textbox(1500, 420, 360, 160, [ph("存入数据库", size=40, color=ac(PURPLE), align="center")], "middle"),
            textbox(80, 780, 1760, 50, [pa("K230捕获图像 → 实时上传 → AI模型推理 → 结构化存储", size=26, color="#495057", align="center")], "middle"),
        ],
    }


def slide_12_api():
    endpoints = [
        ("GET", "/api/clothes", "获取衣物列表"),
        ("GET", "/api/clothes/{id}", "查询单件衣物"),
        ("POST", "/api/tasks", "创建取衣任务"),
        ("GET", "/api/tasks/queue", "查询任务队列"),
        ("POST", "/api/upload", "上传衣物照片"),
        ("PUT", "/api/tasks/{id}", "更新任务状态"),
    ]
    html = f"""{slide_title("云端 API 接口设计")}
<div class="split-left" style="border:2px dashed var(--text-muted);display:flex;align-items:center;justify-content:center;">
<p class="annotation" style="font-size:28px;color:var(--text-muted);">[ FastAPI Swagger 接口文档截图 ]</p>
</div>
<div class="split-right">
<h3>RESTful API 端点</h3>
<ul>"""
    for method, path, desc in endpoints:
        badge_color = {"GET": "#2f9e44", "POST": "#1971c2", "PUT": "#e8590c"}
        html += f'<li><span style="display:inline-block;background:{badge_color.get(method,"#495057")};color:#fff;padding:2px 12px;border-radius:8px;font-size:22px;margin-right:12px;">{method}</span> {path}</li>'
    html += "</ul></div>"

    return {
        "id": 12, "render_mode": "dual", "accent_index": ORANGE,
        "html": html, "css": SPLIT_CSS,
        "shapes_js": page_border(stroke="STYLE.accent", strokeWidth=2.5),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("云端 API 接口设计", size=56, color="#1a1a2e", align="center")], "middle"),
            image(80, 200, 820, 700, label="FastAPI Swagger 接口文档截图", bg_color=acl(ORANGE), border_color=ac(ORANGE)),
            textbox(960, 220, 880, 680, [
                ph("RESTful API 端点", size=44, color=ac(ORANGE), align="left"),
                p(""), p("GET    /api/clothes      获取衣物列表", size=30, align="left"),
                p("GET    /api/clothes/{id}  查询单件衣物", size=30, align="left"),
                p("POST   /api/tasks         创建取衣任务", size=30, align="left"),
                p("GET    /api/tasks/queue   查询任务队列", size=30, align="left"),
                p("POST   /api/upload        上传衣物照片", size=30, align="left"),
                p("PUT    /api/tasks/{id}    更新任务状态", size=30, align="left"),
            ], "top"),
        ],
    }


def slide_13_task_queue():
    img = DIAGRAM_DIR / "07_任务队列.png"
    html = f"""<div style="position:absolute;top:80px;left:80px;width:1760px;height:920px;">
{img_tag(img, "100%", "100%", "contain")}
</div>"""
    return {
        "id": 13, "render_mode": "image_only", "accent_index": ORANGE,
        "html": html, "css": CSS_RESET,
        "shapes_js": page_border(stroke="STYLE.accent", strokeWidth=2),
        "elements": [],
    }


def slide_14_multi_terminal():
    cols = [
        ("📱", "手机 App", "Android 应用\n蓝牙 + WiFi\n远程控制衣橱"),
        ("🖥️", "触控屏幕", "本地触摸屏\n直观操作界面\n无需手机"),
        ("🌐", "Web 管理", "浏览器访问\n云端数据管理\n跨平台兼容"),
    ]
    html = f"""{slide_title("多终端协同操控")}
<div class="col-row" style="top:240px;">"""
    for icon, title, desc in cols:
        desc_html = desc.replace("\n", "<br>")
        html += f"""<div class="col-card" style="background:var(--accent-light);border:2px dashed var(--accent);">
<div style="font-size:64px;margin-bottom:20px;">{icon}</div>
<h3>{title}</h3>
<p>{desc_html}</p></div>"""
    html += "</div>"

    return {
        "id": 14, "render_mode": "dual", "accent_index": ORANGE,
        "html": html, "css": THREE_COL_CSS,
        "shapes_js": (
            card_border(80, 240, 520, 640, "STYLE.accent") + "\n" +
            card_border(700, 240, 520, 640, "STYLE.accent") + "\n" +
            card_border(1320, 240, 520, 640, "STYLE.accent")
        ),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("多终端协同操控", size=56, color="#1a1a2e", align="center")], "middle"),
            image(130, 320, 420, 460, label="手机 App 截图", bg_color=acl(ORANGE), border_color=ac(ORANGE)),
            textbox(130, 800, 420, 60, [ph("手机 App", size=36, color=ac(ORANGE), align="center")], "middle"),
            image(750, 320, 420, 460, label="触摸屏截图", bg_color=acl(ORANGE), border_color=ac(ORANGE)),
            textbox(750, 800, 420, 60, [ph("触控屏幕", size=36, color=ac(ORANGE), align="center")], "middle"),
            image(1370, 320, 420, 460, label="Web 管理截图", bg_color=acl(ORANGE), border_color=ac(ORANGE)),
            textbox(1370, 800, 420, 60, [ph("Web 管理", size=36, color=ac(ORANGE), align="center")], "middle"),
        ],
    }


def slide_15_full_flow():
    img = DIAGRAM_DIR / "03_操作流程.png"
    steps = ["App点击", "云端生成任务", "主板轮询", "K230拍照", "电机取衣", "完成回传"]
    step_html = ""
    for i, step in enumerate(steps):
        step_html += f"""<div class="flow-step" style="background:var(--accent-light);border:2px dashed var(--accent);width:260px;height:200px;">
<div class="step-num" style="font-size:36px;">0{i+1}</div>
<div class="step-text" style="font-size:24px;">{step}</div></div>"""

    html = f"""{slide_title("全流程操作演示")}
<div class="flow-row" style="top:200px;">{step_html}</div>
<div style="position:absolute;top:480px;left:80px;width:1760px;height:420px;">
{img_tag(img, "100%", "100%", "contain")}
</div>
<div style="position:absolute;bottom:40px;left:80px;width:1760px;text-align:center;">
<p style="font-size:36px;color:var(--accent);font-weight:700;font-family:var(--font-heading);">全流程 &lt; 15 秒</p>
<p class="annotation" style="font-size:24px;color:var(--text-muted);">从点击到取衣完成的端到端延迟</p>
</div>"""

    arrow_js = ""
    step_positions = [(160 + i * 280, 300) for i in range(6)]  # approximate centers
    for i in range(5):
        x1 = step_positions[i][0] + 130
        x2 = step_positions[i+1][0] - 10
        arrow_js += arrow_between(x1, 300, x2) + "\n"

    return {
        "id": 15, "render_mode": "dual", "accent_index": RED,
        "html": html, "css": FLOW_CSS,
        "shapes_js": arrow_js + underline_deco(600, 980, 700, "STYLE.accent"),
        "elements": [
            textbox(80, 20, 1760, 60, [ph("全流程操作演示", size=52, color="#1a1a2e", align="center")], "middle"),
            textbox(120, 220, 260, 150, [ph("App点击", size=32, color=ac(RED), align="center")], "middle"),
            textbox(400, 220, 260, 150, [ph("云端生成任务", size=32, color=ac(RED), align="center")], "middle"),
            textbox(680, 220, 260, 150, [ph("主板轮询", size=32, color=ac(RED), align="center")], "middle"),
            textbox(960, 220, 260, 150, [ph("K230拍照", size=32, color=ac(RED), align="center")], "middle"),
            textbox(1240, 220, 260, 150, [ph("电机取衣", size=32, color=ac(RED), align="center")], "middle"),
            textbox(1520, 220, 260, 150, [ph("完成回传", size=32, color=ac(RED), align="center")], "middle"),
            image(80, 490, 1760, 400, src=str(img), label="操作流程图"),
            textbox(80, 940, 1760, 50, [ph("全流程 < 15 秒", size=36, color=ac(RED), align="center")], "middle"),
            textbox(80, 990, 1760, 40, [pa("从点击到取衣完成的端到端延迟", size=24, color="#495057", align="center")], "middle"),
        ],
    }


def slide_16_demo():
    html = f"""{slide_title("系统演示")}
<div style="position:absolute;top:180px;left:160px;width:1600px;height:680px;
border:3px dashed var(--accent);border-radius:20px;display:flex;align-items:center;
justify-content:center;background:var(--accent-light);">
<p class="annotation" style="font-size:40px;color:var(--accent-dark);">🎬 系统演示视频</p>
</div>
<div style="position:absolute;bottom:60px;left:80px;width:1760px;text-align:center;">
<p class="annotation" style="font-size:26px;color:var(--text-muted);">智能衣橱系统完整操作流程演示</p>
</div>"""
    return {
        "id": 16, "render_mode": "dual", "accent_index": RED,
        "html": html, "css": CSS_RESET,
        "shapes_js": page_border(stroke="STYLE.accent", strokeWidth=3) + "\n" +
                      rough_circle(960, 500, 600, "STYLE.accent") + "\n" +
                      rough_circle(960, 500, 620, "STYLE.accentLight"),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("系统演示", size=60, color="#1a1a2e", align="center")], "middle"),
            image(160, 180, 1600, 680, label="系统演示视频", bg_color=acl(RED), border_color=ac(RED)),
            textbox(80, 920, 1760, 50, [pa("智能衣橱系统完整操作流程演示", size=26, color="#495057", align="center")], "middle"),
        ],
    }


def slide_17_innovation():
    cards = [
        ("🔧", "双板解耦设计", "协调板与执行板分离\n6路UART高效通信\n故障隔离、易于维护"),
        ("👁️", "视觉识别闭环", "K230拍照 + 云端AI\n自动分类与检索\n识别准确率持续提升"),
        ("📋", "任务队列容错", "云端持久化任务队列\n断网重试、状态追踪\n保证指令不丢失"),
        ("☁️", "云边端协同", "端侧执行、云端调度\n多终端统一接入\n架构可水平扩展"),
    ]
    html_parts = [slide_title("创新点与亮点")]
    positions = [(130, 220, 780, 370), (1010, 220, 780, 370), (130, 630, 780, 370), (1010, 630, 780, 370)]
    for i, ((cx, cy, cw, ch), (icon, title, desc)) in enumerate(zip(positions, cards)):
        desc_html = desc.replace("\n", "<br>")
        html_parts.append(f"""<div class="card" style="left:{cx}px;top:{cy}px;width:{cw}px;height:{ch}px;">
<div style="font-size:56px;margin-bottom:12px;">{icon}</div>
<h3>{title}</h3>
<p>{desc_html}</p></div>""")

    return {
        "id": 17, "render_mode": "dual", "accent_index": CYAN,
        "html": "\n".join(html_parts),
        "css": GRID_CSS,
        "shapes_js": corner_accents(positions) + "\n" + underline_deco(580, 110, 700),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("创新点与亮点", size=56, color="#1a1a2e", align="center")], "middle"),
            textbox(170, 270, 700, 220, [
                ph("双板解耦设计", size=42, color=ac(CYAN), align="center"),
                p(""), p("协调板与执行板分离", size=28, align="center"),
                p("6路UART高效通信", size=28, align="center"),
                p("故障隔离、易于维护", size=28, align="center"),
            ], "middle"),
            textbox(1050, 270, 700, 220, [
                ph("视觉识别闭环", size=42, color=ac(CYAN), align="center"),
                p(""), p("K230拍照 + 云端AI", size=28, align="center"),
                p("自动分类与检索", size=28, align="center"),
                p("识别准确率持续提升", size=28, align="center"),
            ], "middle"),
            textbox(170, 680, 700, 220, [
                ph("任务队列容错", size=42, color=ac(CYAN), align="center"),
                p(""), p("云端持久化任务队列", size=28, align="center"),
                p("断网重试、状态追踪", size=28, align="center"),
                p("保证指令不丢失", size=28, align="center"),
            ], "middle"),
            textbox(1050, 680, 700, 220, [
                ph("云边端协同", size=42, color=ac(CYAN), align="center"),
                p(""), p("端侧执行、云端调度", size=28, align="center"),
                p("多终端统一接入", size=28, align="center"),
                p("架构可水平扩展", size=28, align="center"),
            ], "middle"),
        ],
    }


def slide_18_summary():
    cols = [
        ("✅", "已完成", "双STM32F407电控系统\nK230视觉识别模块\nFastAPI云端后端\n任务队列管理系统\n多终端应用开发"),
        ("🔮", "可扩展", "接入大语言模型\n智能穿搭推荐\n更多传感器融合\n衣柜集群管理\n社区共享功能"),
        ("📦", "项目产出", "PCB电路板 ×2\nAndroid App\n触控屏固件\n云端API服务\n技术文档与论文"),
    ]
    html = f"""{slide_title("项目总结与展望")}
<div class="col-row" style="top:240px;">"""
    for icon, title, desc in cols:
        desc_html = desc.replace("\n", "<br>")
        html += f"""<div class="col-card" style="background:var(--accent-light);border:2px dashed var(--accent);">
<div style="font-size:56px;margin-bottom:16px;">{icon}</div>
<h3>{title}</h3>
<p style="text-align:left;line-height:1.7;font-size:26px;">{desc_html}</p></div>"""
    html += "</div>"

    return {
        "id": 18, "render_mode": "dual", "accent_index": CYAN,
        "html": html, "css": THREE_COL_CSS,
        "shapes_js": (
            card_border(80, 240, 520, 670, "STYLE.accent") + "\n" +
            card_border(700, 240, 520, 670, "STYLE.accent") + "\n" +
            card_border(1320, 240, 520, 670, "STYLE.accent")
        ),
        "elements": [
            textbox(80, 30, 1760, 80, [ph("项目总结与展望", size=56, color="#1a1a2e", align="center")], "middle"),
            textbox(120, 280, 440, 550, [
                ph("已完成", size=42, color=ac(CYAN), align="center"),
                p(""), p("双STM32F407电控系统", size=26, align="center"),
                p("K230视觉识别模块", size=26, align="center"),
                p("FastAPI云端后端", size=26, align="center"),
                p("任务队列管理系统", size=26, align="center"),
                p("多终端应用开发", size=26, align="center"),
            ], "middle"),
            textbox(740, 280, 440, 550, [
                ph("可扩展", size=42, color=ac(CYAN), align="center"),
                p(""), p("接入大语言模型", size=26, align="center"),
                p("智能穿搭推荐", size=26, align="center"),
                p("更多传感器融合", size=26, align="center"),
                p("衣柜集群管理", size=26, align="center"),
                p("社区共享功能", size=26, align="center"),
            ], "middle"),
            textbox(1360, 280, 440, 550, [
                ph("项目产出", size=42, color=ac(CYAN), align="center"),
                p(""), p("PCB电路板 ×2", size=26, align="center"),
                p("Android App", size=26, align="center"),
                p("触控屏固件", size=26, align="center"),
                p("云端API服务", size=26, align="center"),
                p("技术文档与论文", size=26, align="center"),
            ], "middle"),
        ],
    }


def slide_19_thanks():
    html = f"""<div class="close-center">
<h1 style="font-size:100px;color:var(--accent);margin-bottom:40px;letter-spacing:16px;">感谢聆听</h1>
<p class="annotation" style="font-size:36px;color:var(--text);margin-bottom:80px;">请各位老师批评指正</p>
<p style="font-size:28px;color:var(--text-muted);">智能衣橱系统 — 基于云端协同的智能衣物存取与管理系统</p>
</div>"""
    return {
        "id": 19, "render_mode": "dual", "accent_index": BLUE,
        "html": html, "css": CLOSE_CSS,
        "shapes_js": (
            rough_circle(960, 380, 400, "STYLE.accentLight") + "\n" +
            rough_circle(960, 380, 420, "STYLE.accent") + "\n" +
            rough_circle(960, 380, 440, "STYLE.accentLight") + "\n" +
            page_border(stroke="STYLE.accent", strokeWidth=2)
        ),
        "elements": [
            textbox(80, 300, 1760, 160, [ph("感谢聆听", size=100, color=ac(BLUE), align="center")], "middle"),
            textbox(80, 500, 1760, 80, [pa("请各位老师批评指正", size=36, color="#1a1a2e", align="center")], "middle"),
            textbox(80, 620, 1760, 60, [pa("智能衣橱系统 — 基于云端协同的智能衣物存取与管理系统", size=28, color="#495057", align="center")], "middle"),
        ],
    }


def build_deck():
    slides = [
        slide_1_cover(),
        slide_2_background(),
        slide_3_goals(),
        slide_4_architecture(),
        slide_5_pcb(),
        slide_6_dual_board(),
        slide_7_uart_topology(),
        slide_8_action_loop(),
        slide_9_k230_camera(),
        slide_10_recognition(),
        slide_11_vision_flow(),
        slide_12_api(),
        slide_13_task_queue(),
        slide_14_multi_terminal(),
        slide_15_full_flow(),
        slide_16_demo(),
        slide_17_innovation(),
        slide_18_summary(),
        slide_19_thanks(),
    ]
    return {"style": STYLE, "slides": slides}


if __name__ == "__main__":
    deck = build_deck()
    with open(OUTPUT, "w", encoding="utf-8") as f:
        json.dump(deck, f, ensure_ascii=False, indent=2)
    print(f"Generated: {OUTPUT} ({len(deck['slides'])} slides)")
