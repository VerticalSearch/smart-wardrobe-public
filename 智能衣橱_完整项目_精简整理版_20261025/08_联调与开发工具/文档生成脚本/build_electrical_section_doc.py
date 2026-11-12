# -*- coding: utf-8 -*-
from pathlib import Path

from docx import Document
from docx.enum.table import WD_ALIGN_VERTICAL, WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt, RGBColor


ROOT = Path(__file__).resolve().parent
OUT_DIR = ROOT / "01_智能衣橱方案"
DOCX_PATH = OUT_DIR / "智能衣橱申报书_电控部分内容.docx"
MD_PATH = OUT_DIR / "智能衣橱申报书_电控部分内容.md"

FONT_BODY = "宋体"
FONT_HEAD = "微软雅黑"
BLUE = RGBColor(37, 99, 145)
DARK = RGBColor(31, 45, 61)
MUTED = RGBColor(92, 106, 126)


CONTENT = [
    ("title", "智能衣橱申报书电控部分内容"),
    ("subtitle", "可直接用于申报书“技术方案 / 电控系统 / 硬件控制”栏目"),

    ("h1", "一、电控系统总体任务与分层架构"),
    ("p", "本项目电控系统负责将上层取衣、放衣、识别、清理和护理任务转化为安全、连续、可反馈的机械动作。系统采用“上层任务输入 + STM32F407主板1调度 + STM32F407主板2执行 + 机构状态反馈”的分层控制架构，使人机交互、任务调度和底层运动控制相互解耦。"),
    ("p", "在该架构下，树莓派触摸屏网页、App/Web或云端后端只表达任务意图，不直接输出电机脉冲；主板1负责接收任务、汇聚传感器状态、判断执行条件并下发动作命令；主板2负责多轴步进电机、舵机、夹爪、伸缩轴、B区毛刷、C区热风、负离子和通风风扇等执行机构控制。"),
    ("table", [
        ("层级", "组成", "作用"),
        ("任务输入层", "触摸屏网页、App/Web、云端后端", "产生取衣、放衣、识别、清理、护理和急停等任务意图。"),
        ("调度控制层", "STM32F407主板1", "负责指令接收、状态判定、任务调度、异常拦截和板间命令转发。"),
        ("运动执行层", "STM32F407主板2", "负责多轴运动、夹爪舵机、伸缩机构、毛刷和护理组件执行。"),
        ("状态反馈层", "限位、温湿度、重力、按键、到位反馈", "提供执行前校验、动作完成判断、异常保护和任务状态回写依据。"),
    ]),
    ("note", "【插图位置1：电控系统任务闭环图】建议画“任务输入 -> 主板1调度 -> 主板2执行 -> 执行机构 -> 状态反馈 -> 任务完成”。图注建议：图1 智能衣橱电控系统任务闭环。"),

    ("h1", "二、任务指令流转与调度机制"),
    ("p", "电控系统不是直接根据按钮让单个电机动作，而是先将上层任务转化为可执行的电控流程。主板1接收到任务后，先判断目标挂位、机构状态、环境状态和急停状态是否满足执行条件，再将任务拆解为主板2可以执行的动作命令。"),
    ("p", "这种任务流转机制使上层页面、云端任务和底层机构之间形成清晰边界：上层只负责“要做什么”，主板1负责“能不能做、按什么顺序做”，主板2负责“具体怎么动、是否到位”。"),
    ("table", [
        ("任务类型", "主板1调度重点", "主板2执行重点"),
        ("放衣/入库", "确认流程状态，选择A区取衣点与目标C区存放点。", "驱动夹爪、伸缩轴和X/Y/Z轴完成转运。"),
        ("取衣/输出", "确认目标挂位状态，组织从C区到A区的取衣路径。", "经过前置点和中转点完成夹取、转运和释放。"),
        ("拍照/识别", "在到达B0识别点后触发拍照或识别任务。", "将夹爪移动到识别位置并保持稳定姿态。"),
        ("清理/护理", "根据任务类型选择B区毛刷或C区护理组件。", "控制毛刷、热风、负离子和风扇按设定流程执行。"),
        ("急停/异常", "最高优先级拦截当前流程并下发停止指令。", "停止运动输出，避免机构继续执行危险动作。"),
    ]),
    ("note", "【插图位置2：任务指令流转图】建议画“用户任务 -> 主板1解析/判定 -> 主板2动作状态机 -> 执行机构动作”。图注建议：图2 电控任务指令流转与调度机制。"),

    ("h1", "三、点位表路径规划与防干涉设计"),
    ("p", "衣橱内部空间紧凑，夹爪、衣物、挂位、毛刷和护理组件之间存在潜在干涉风险。因此，系统将A区、B区、B0识别点、C区、回家点和前置点抽象为功能点位表，并根据不同任务自动选择运动路径。"),
    ("p", "路径规划过程中，程序优先抬升Z轴到安全高度，再进行X/Y平面移动，最后下降进入目标点；进入A区和C区等狭窄区域时，系统先经过前置点和中转点，再进入真实目标点，避免低高度横移、直接斜插和未达到安全姿态前的翻转动作。该设计可概括为“电子围栏式防干涉路径控制”。"),
    ("table", [
        ("路径控制策略", "具体含义", "展示价值"),
        ("功能点位表", "将A区、B区、B0、C区等位置统一建模。", "说明机构运动有目标点和任务语义，不是盲目移动。"),
        ("安全高度优先", "先抬Z轴，再进行X/Y移动，最后下降。", "降低夹爪、衣物与柜体结构发生干涉的风险。"),
        ("前置点进入", "进入A区、C区前先经过偏移位置。", "避免直接切入狭窄区域，提高动作可控性。"),
        ("B点中转", "A区与B0、C区与A区之间设置过渡路径。", "减少斜插路线和姿态未调整前的危险运动。"),
    ]),
    ("note", "【插图位置3：A/B/B0/C点位与防干涉路径图】建议用平面示意图标出A区、B区、B0、C区、前置点、安全高度和禁止直达区域。图注建议：图3 基于点位表的防干涉路径规划。"),

    ("h1", "四、动作状态机与执行机构协同控制"),
    ("p", "主板2的执行逻辑采用表驱动动作状态机，将复杂任务拆解为移动、等待、夹取、释放、翻转、伸缩、清理和护理等标准动作单元。每一步动作完成后再进入下一状态，使整套流程具备明确顺序、可等待、可中断、可扩展的特点。"),
    ("p", "在该机制下，夹爪、舵机、伸缩轴、B区毛刷、C区热风机、负离子发生器和通风风扇不再是分散控制，而是被纳入统一动作链路。后续如果增加挂位、调整路径或扩展护理动作，可以优先调整点位参数、等待时间和动作表，而不是重写整套控制逻辑。"),
    ("table", [
        ("动作单元", "控制对象", "作用"),
        ("移动到点位", "X/Y/Z轴、伸缩轴", "完成夹爪或衣物在功能区之间的定位。"),
        ("夹取与释放", "夹爪舵机、Z轴", "完成衣物抓取、放置和输出动作。"),
        ("姿态调整", "舵机、Z轴安全高度", "在允许高度内完成翻转或姿态切换。"),
        ("清理动作", "B区毛刷、步进电机、舵机", "完成衣物表面毛发、浮尘和附着物清理。"),
        ("护理动作", "热风、负离子、通风风扇", "完成烘干、除味、散湿和空气流通辅助。"),
    ]),
    ("note", "【插图位置4：动作状态机与执行机构协同图】建议画状态机节点，如“移动到A点 -> 夹取 -> 移动到B0 -> 识别 -> 移动到C前置点 -> 放置”。图注建议：图4 表驱动动作状态机与多机构协同控制。"),

    ("h1", "五、X/Y同步到点与夹爪轨迹平稳控制"),
    ("p", "X轴和Y轴并不是两个互不相关的独立动作，而是共同决定夹爪在平面内的运动轨迹。如果两轴采用相近速度直接运动，位移较短的轴会提前到位并停止，后续只剩另一轴继续运动，实际轨迹会由平滑斜线变为“先斜向运动、再单轴横向运动”的折线路径。"),
    ("p", "这种折线路径会造成运动方向突变、动作衔接不顺，并在衣橱狭小空间内增加接近柜体、挂位或机构边界的风险。夹持衣物时，单轴提前停止还会使转运过程不够连续，影响夹爪姿态稳定性和定位一致性。"),
    ("p", "因此，系统引入X/Y双轴同步到点与夹爪轨迹平稳控制策略。程序首先计算两轴目标位移差异，再根据预计运行时间对速度、加速度和启动时序进行协调；对于位移较短的轴，可适当降低运行速度或延后启动，使其与位移较长的轴尽量同步到达目标点。执行过程中，系统结合到位反馈与超时判断确认运动完成，从而形成连续、可预测的平面运动轨迹。"),
    ("table", [
        ("对比项", "不同步控制", "同步到点控制"),
        ("轨迹形态", "先斜向运动，再单轴横向运动，形成折线。", "从起点到目标点连续接近，轨迹更稳定。"),
        ("动作衔接", "短轴提前停止，运动方向突然变化。", "两轴协调到达，减少方向突变。"),
        ("空间安全", "折线路径更可能靠近柜体或挂位边界。", "运动轨迹更可预测，便于配合防干涉路径。"),
        ("衣物姿态", "夹持转运过程连续性较差。", "夹爪和衣物姿态更稳定。"),
    ]),
    ("note", "【插图位置5：X/Y双轴同步到点轨迹对比图】建议左侧画不同步轨迹：先斜向运动，再单轴横向运动；右侧画同步轨迹：从起点到终点连续斜向接近。图注建议：图5 X/Y同步到点对夹爪轨迹连续性的改善。"),

    ("h1", "六、典型任务流程、安全保护与预期成果"),
    ("p", "典型任务流程包括放衣、取衣、拍照识别、B区清理和C区护理。放衣流程从A区取衣，经过B/B0识别点后进入C区存放；取衣流程从C区目标挂位出发，经过前置点和B点中转后回到A区输出；清理和护理流程则由主板1根据任务类型调度主板2执行毛刷、热风、负离子和通风风扇动作。"),
    ("p", "安全可靠性设计贯穿任务执行全过程。系统通过挂位限位、温湿度、重力、本地按键、到位反馈和动作超时等状态量进行执行前校验、执行中监测和异常停止。急停命令具有最高优先级，动作超时或状态异常时应停止当前流程并回到安全状态。"),
    ("table", [
        ("保护/成果项", "说明"),
        ("执行前校验", "检查任务类型、目标挂位、机构状态和必要传感器条件。"),
        ("执行中监测", "等待电机到位反馈，结合超时机制判断动作是否可靠完成。"),
        ("异常停止", "急停、动作超时或状态异常时停止执行，避免机构继续动作。"),
        ("可展示成果", "形成电控架构图、路径规划图、动作状态机图、同步轨迹图和测试记录。"),
        ("扩展能力", "后续可增加挂位、护理动作和更多异常反馈，不破坏主体电控架构。"),
    ]),
    ("note", "【插图位置6：典型任务流程与安全保护图】建议将放衣、取衣、清理、护理和急停保护放在同一张流程图中。图注建议：图6 典型任务执行流程与安全保护机制。"),
    ("p", "综上，电控系统并不是简单驱动电机动作，而是通过双主板分层架构，将上层任务转化为带路径规划、防干涉约束、状态机编排、双轴同步控制和安全反馈的完整机械执行流程，为智能衣橱实现自动取放衣、识别、清理和护理提供稳定的底层控制基础。"),
]


def set_run_font(run, font=FONT_BODY, size=10.5, bold=False, color=None):
    run.font.name = font
    run._element.rPr.rFonts.set(qn("w:eastAsia"), font)
    run.font.size = Pt(size)
    run.bold = bold
    if color:
        run.font.color.rgb = color


def set_cell_shading(cell, fill):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = tc_pr.find(qn("w:shd"))
    if shd is None:
        shd = OxmlElement("w:shd")
        tc_pr.append(shd)
    shd.set(qn("w:fill"), fill)


def set_cell_margins(cell, top=80, bottom=80, start=120, end=120):
    tc_pr = cell._tc.get_or_add_tcPr()
    tc_mar = tc_pr.find(qn("w:tcMar"))
    if tc_mar is None:
        tc_mar = OxmlElement("w:tcMar")
        tc_pr.append(tc_mar)
    for tag, value in (("top", top), ("bottom", bottom), ("start", start), ("end", end)):
        node = tc_mar.find(qn(f"w:{tag}"))
        if node is None:
            node = OxmlElement(f"w:{tag}")
            tc_mar.append(node)
        node.set(qn("w:w"), str(value))
        node.set(qn("w:type"), "dxa")


def add_paragraph(doc, text, size=10.5, color=None, bold=False, align=None, after=5):
    p = doc.add_paragraph()
    p.paragraph_format.line_spacing = 1.25
    p.paragraph_format.space_after = Pt(after)
    if align is not None:
        p.alignment = align
    run = p.add_run(text)
    set_run_font(run, FONT_BODY, size, bold, color)


def add_heading(doc, text, level=1):
    p = doc.add_paragraph(style=f"Heading {level}")
    p.paragraph_format.space_before = Pt(8 if level == 1 else 5)
    p.paragraph_format.space_after = Pt(5)
    run = p.add_run(text)
    set_run_font(run, FONT_HEAD, 15 if level == 1 else 12, True, BLUE if level == 1 else DARK)


def add_note(doc, text):
    table = doc.add_table(rows=1, cols=1)
    table.style = "Table Grid"
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.rows[0].cells[0]
    set_cell_shading(cell, "F8FAFC")
    set_cell_margins(cell, top=120, bottom=120, start=160, end=160)
    p = cell.paragraphs[0]
    p.paragraph_format.line_spacing = 1.2
    run = p.add_run(text)
    set_run_font(run, FONT_BODY, 10, False, DARK)
    add_paragraph(doc, "", after=2)


def add_table(doc, rows):
    cols = len(rows[0])
    table = doc.add_table(rows=1, cols=cols)
    table.style = "Table Grid"
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = True
    for i, h in enumerate(rows[0]):
        cell = table.rows[0].cells[i]
        set_cell_shading(cell, "E8EEF5")
        set_cell_margins(cell)
        cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
        run = cell.paragraphs[0].add_run(h)
        set_run_font(run, FONT_HEAD, 10, True, DARK)
    for row_data in rows[1:]:
        row = table.add_row()
        for i, text in enumerate(row_data):
            cell = row.cells[i]
            set_cell_margins(cell)
            cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(0)
            p.paragraph_format.line_spacing = 1.15
            run = p.add_run(text)
            set_run_font(run, FONT_BODY, 9.8, False, DARK)
    add_paragraph(doc, "", after=2)


def build_docx():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    doc = Document()
    section = doc.sections[0]
    section.page_width = Cm(21)
    section.page_height = Cm(29.7)
    section.top_margin = Cm(2.0)
    section.bottom_margin = Cm(2.0)
    section.left_margin = Cm(2.2)
    section.right_margin = Cm(2.2)

    styles = doc.styles
    normal = styles["Normal"]
    normal.font.name = FONT_BODY
    normal._element.rPr.rFonts.set(qn("w:eastAsia"), FONT_BODY)
    normal.font.size = Pt(10.5)
    normal.paragraph_format.line_spacing = 1.25
    normal.paragraph_format.space_after = Pt(5)

    for name in ("Heading 1", "Heading 2"):
        style = styles[name]
        style.font.name = FONT_HEAD
        style._element.rPr.rFonts.set(qn("w:eastAsia"), FONT_HEAD)
        style.font.bold = True
        style.font.color.rgb = BLUE if name == "Heading 1" else DARK

    footer = section.footer.paragraphs[0]
    footer.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = footer.add_run("智能衣橱申报书电控部分内容")
    set_run_font(r, FONT_BODY, 9, False, MUTED)

    for kind, value in CONTENT:
        if kind == "title":
            p = doc.add_paragraph()
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            p.paragraph_format.space_after = Pt(8)
            r = p.add_run(value)
            set_run_font(r, FONT_HEAD, 20, True, DARK)
        elif kind == "subtitle":
            add_paragraph(doc, value, size=10.5, color=MUTED, align=WD_ALIGN_PARAGRAPH.CENTER, after=12)
        elif kind == "h1":
            add_heading(doc, value, 1)
        elif kind == "p":
            add_paragraph(doc, value)
        elif kind == "note":
            add_note(doc, value)
        elif kind == "table":
            add_table(doc, value)

    doc.core_properties.title = "智能衣橱申报书电控部分内容"
    doc.core_properties.subject = "电控部分申报书正文"
    doc.save(DOCX_PATH)
    return DOCX_PATH


def build_markdown():
    lines = ["# 智能衣橱申报书电控部分内容", "", "可直接用于申报书“技术方案 / 电控系统 / 硬件控制”栏目。", ""]
    for kind, value in CONTENT[2:]:
        if kind == "h1":
            lines.extend([f"## {value}", ""])
        elif kind == "p":
            lines.extend([value, ""])
        elif kind == "note":
            lines.extend([f"> {value}", ""])
        elif kind == "table":
            headers = value[0]
            lines.append("| " + " | ".join(headers) + " |")
            lines.append("| " + " | ".join(["---"] * len(headers)) + " |")
            for row in value[1:]:
                lines.append("| " + " | ".join(row) + " |")
            lines.append("")
    MD_PATH.write_text("\n".join(lines), encoding="utf-8")
    return MD_PATH


if __name__ == "__main__":
    print(build_docx())
    print(build_markdown())
