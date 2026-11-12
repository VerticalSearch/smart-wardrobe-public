# -*- coding: utf-8 -*-
from pathlib import Path

from docx import Document
from docx.enum.section import WD_ORIENT
from docx.enum.table import WD_ALIGN_VERTICAL, WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_BREAK
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Inches, Pt, RGBColor


ROOT = Path(__file__).resolve().parent
OUT_DIR = ROOT / "01_智能衣橱方案"
OUT_PATH = OUT_DIR / "智能衣橱项目申报书_8页版.docx"
ARCH_IMAGE = ROOT / "artifacts" / "电控配图_树莓派双板架构.png"
ARCH_IMAGE_JPEG = ROOT / "outputs" / "doc_assets" / "control_arch_for_doc.jpg"

FONT_BODY = "宋体"
FONT_HEAD = "微软雅黑"
BLUE = RGBColor(37, 99, 145)
DARK = RGBColor(31, 45, 61)
MUTED = RGBColor(90, 104, 124)
LIGHT_FILL = "F4F6F9"
HEADER_FILL = "E8EEF5"


def set_cell_shading(cell, fill):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = tc_pr.find(qn("w:shd"))
    if shd is None:
        shd = OxmlElement("w:shd")
        tc_pr.append(shd)
    shd.set(qn("w:fill"), fill)


def set_cell_width(cell, width_dxa):
    tc_pr = cell._tc.get_or_add_tcPr()
    tc_w = tc_pr.find(qn("w:tcW"))
    if tc_w is None:
        tc_w = OxmlElement("w:tcW")
        tc_pr.append(tc_w)
    tc_w.set(qn("w:w"), str(width_dxa))
    tc_w.set(qn("w:type"), "dxa")


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


def set_run_font(run, name=FONT_BODY, size=None, bold=False, color=None):
    run.font.name = name
    run._element.rPr.rFonts.set(qn("w:eastAsia"), name)
    if size:
        run.font.size = Pt(size)
    run.bold = bold
    if color:
        run.font.color.rgb = color


def add_para(doc, text="", style=None, align=None, size=10.5, bold=False, color=None, after=5):
    p = doc.add_paragraph(style=style)
    if align is not None:
        p.alignment = align
    p.paragraph_format.space_after = Pt(after)
    p.paragraph_format.line_spacing = 1.25
    if text:
        run = p.add_run(text)
        set_run_font(run, size=size, bold=bold, color=color)
    return p


def add_heading(doc, text, level=1):
    style = f"Heading {level}"
    p = doc.add_paragraph(style=style)
    p.paragraph_format.keep_with_next = False
    p.paragraph_format.space_before = Pt(8 if level == 1 else 6)
    p.paragraph_format.space_after = Pt(5)
    run = p.add_run(text)
    set_run_font(run, FONT_HEAD, 16 if level == 1 else 12.5, True, BLUE if level == 1 else DARK)
    return p


def add_list(doc, items):
    for item in items:
        p = doc.add_paragraph(style="List Bullet")
        p.paragraph_format.space_after = Pt(3)
        p.paragraph_format.line_spacing = 1.2
        run = p.add_run(item)
        set_run_font(run, size=10.5)


def add_numbered(doc, items):
    for item in items:
        p = doc.add_paragraph(style="List Number")
        p.paragraph_format.space_after = Pt(3)
        p.paragraph_format.line_spacing = 1.2
        run = p.add_run(item)
        set_run_font(run, size=10.5)


def add_label_table(doc, rows, widths=(2500, 6500), header=None):
    table = doc.add_table(rows=0, cols=2)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.style = "Table Grid"
    table.autofit = False
    if header:
        row = table.add_row()
        row.cells[0].merge(row.cells[1])
        cell = row.cells[0]
        set_cell_shading(cell, HEADER_FILL)
        set_cell_margins(cell)
        run = cell.paragraphs[0].add_run(header)
        set_run_font(run, FONT_HEAD, 10.5, True, DARK)
    for label, value in rows:
        row = table.add_row()
        for i, (cell, width) in enumerate(zip(row.cells, widths)):
            set_cell_width(cell, width)
            set_cell_margins(cell)
            cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(0)
            run = p.add_run(label if i == 0 else value)
            set_run_font(run, FONT_HEAD if i == 0 else FONT_BODY, 10.2, i == 0, DARK)
            if i == 0:
                set_cell_shading(cell, LIGHT_FILL)
    add_para(doc, "", after=3)
    return table


def add_matrix_table(doc, headers, rows, widths):
    table = doc.add_table(rows=1, cols=len(headers))
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.style = "Table Grid"
    table.autofit = False
    for idx, h in enumerate(headers):
        cell = table.rows[0].cells[idx]
        set_cell_width(cell, widths[idx])
        set_cell_margins(cell)
        set_cell_shading(cell, HEADER_FILL)
        cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
        run = cell.paragraphs[0].add_run(h)
        set_run_font(run, FONT_HEAD, 9.8, True, DARK)
    for row_data in rows:
        row = table.add_row()
        for idx, text in enumerate(row_data):
            cell = row.cells[idx]
            set_cell_width(cell, widths[idx])
            set_cell_margins(cell)
            cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(0)
            p.paragraph_format.line_spacing = 1.15
            run = p.add_run(text)
            set_run_font(run, size=9.5)
    add_para(doc, "", after=3)
    return table


def add_callout(doc, title, body):
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.style = "Table Grid"
    table.autofit = False
    cell = table.rows[0].cells[0]
    set_cell_width(cell, 9360)
    set_cell_margins(cell, top=120, bottom=120, start=160, end=160)
    set_cell_shading(cell, "F8FAFC")
    p = cell.paragraphs[0]
    p.paragraph_format.space_after = Pt(3)
    r1 = p.add_run(title + "：")
    set_run_font(r1, FONT_HEAD, 10.3, True, BLUE)
    r2 = p.add_run(body)
    set_run_font(r2, FONT_BODY, 10.3, False, DARK)
    add_para(doc, "", after=2)


def add_page_break(doc):
    p = doc.add_paragraph()
    run = p.add_run()
    run.add_break(WD_BREAK.PAGE)


def get_render_safe_arch_image():
    if not ARCH_IMAGE.exists():
        return None
    try:
        from PIL import Image

        ARCH_IMAGE_JPEG.parent.mkdir(parents=True, exist_ok=True)
        with Image.open(ARCH_IMAGE) as im:
            rgb = im.convert("RGB")
            rgb.save(ARCH_IMAGE_JPEG, "JPEG", quality=92, optimize=True)
        return ARCH_IMAGE_JPEG
    except Exception:
        return ARCH_IMAGE


def setup_document():
    doc = Document()
    section = doc.sections[0]
    section.orientation = WD_ORIENT.PORTRAIT
    section.page_width = Cm(21)
    section.page_height = Cm(29.7)
    section.top_margin = Cm(2.0)
    section.bottom_margin = Cm(1.9)
    section.left_margin = Cm(2.1)
    section.right_margin = Cm(2.1)
    section.header_distance = Cm(1.0)
    section.footer_distance = Cm(1.0)

    styles = doc.styles
    normal = styles["Normal"]
    normal.font.name = FONT_BODY
    normal._element.rPr.rFonts.set(qn("w:eastAsia"), FONT_BODY)
    normal.font.size = Pt(10.5)
    normal.paragraph_format.line_spacing = 1.25
    normal.paragraph_format.space_after = Pt(5)

    for name, size, color in (("Heading 1", 16, BLUE), ("Heading 2", 12.5, DARK), ("Heading 3", 11.5, DARK)):
        style = styles[name]
        style.font.name = FONT_HEAD
        style._element.rPr.rFonts.set(qn("w:eastAsia"), FONT_HEAD)
        style.font.size = Pt(size)
        style.font.bold = True
        style.font.color.rgb = color
        style.paragraph_format.space_before = Pt(8)
        style.paragraph_format.space_after = Pt(5)
        style.paragraph_format.line_spacing = 1.15

    for name in ("List Bullet", "List Number"):
        style = styles[name]
        style.font.name = FONT_BODY
        style._element.rPr.rFonts.set(qn("w:eastAsia"), FONT_BODY)
        style.font.size = Pt(10.5)
        style.paragraph_format.left_indent = Cm(0.75)
        style.paragraph_format.first_line_indent = Cm(-0.35)
        style.paragraph_format.space_after = Pt(3)
        style.paragraph_format.line_spacing = 1.2

    footer = section.footer.paragraphs[0]
    footer.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = footer.add_run("智能衣橱项目申报书（正文版）")
    set_run_font(r, FONT_BODY, 9, False, MUTED)
    return doc


def build():
    doc = setup_document()

    # Page 1
    add_para(doc, "项目申报书（正文版）", align=WD_ALIGN_PARAGRAPH.CENTER, size=15, bold=True, color=BLUE, after=10)
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_after = Pt(10)
    r = p.add_run("基于云端控制、视觉识别与上身效果预览的智能衣橱系统")
    set_run_font(r, FONT_HEAD, 22, True, DARK)
    add_para(doc, "面向家庭衣物收纳、选衣、取衣、清理与护理场景", align=WD_ALIGN_PARAGRAPH.CENTER, size=11.5, color=MUTED, after=12)
    add_label_table(doc, [
        ("项目类型", "实物类 / 智能家居与机电控制方向"),
        ("申报阶段", "原型系统完善与应用验证"),
        ("核心架构", "树莓派触摸屏网页 + 云端后端 + 树莓派外接摄像头 + 双 STM32F407 + 多执行机构"),
        ("申报日期", "2026 年 5 月"),
    ])
    add_heading(doc, "一、项目概述", 1)
    add_para(doc, "本项目面向家庭衣物管理中“找衣难、搭配判断慢、取放依赖人工、衣物护理不连续”等实际问题，设计一套集衣物识别建档、云端数据管理、现场触摸交互、自动取衣执行、毛刷清理、热风护理与上身效果预览于一体的智能衣橱系统。系统不只完成单一机械动作，而是围绕衣物全流程管理建立“识别、记录、选择、预览、执行、护理、回写”的闭环。")
    add_para(doc, "当前正式方案以树莓派触摸屏网页作为现场人机交互入口，树莓派外接摄像头负责衣物拍照与上传；云端后端负责衣物数据、图片、状态和任务管理；树莓派通过串口把控制意图送入 STM32F407 主板1，主板1完成传感器汇聚、任务判定和流程调度，主板2负责步进电机、舵机、夹爪、伸缩轴、毛刷、热风机、负离子发生器和通风风扇等执行机构落地。")
    add_callout(doc, "申报定位", "本项目可作为智能家居、机电一体化、云边协同控制和生活服务机器人方向的实物类作品申报。")
    add_page_break(doc)

    # Page 2
    add_heading(doc, "二、立项背景与需求分析", 1)
    add_para(doc, "衣橱是家庭高频使用场景，但传统衣柜仍主要依赖人工记忆和人工整理。随着衣物数量增加，用户常遇到衣物位置不清、已取出和在柜状态混乱、季节性衣物护理不足、出门前搭配效率低等问题。现有智能衣柜产品多集中在照明、除湿、杀菌或简单传感器监测，缺少把衣物视觉识别、云端数据、现场交互、机械取放和护理流程统一起来的系统方案。")
    add_para(doc, "本项目的立项价值在于把衣物管理从“存放空间”提升为“可感知、可查询、可执行、可维护”的服务系统。用户不需要在柜内反复翻找，也不需要先取出衣物再判断搭配效果；系统可以先在前端显示衣物信息和上身效果预览，用户确认后再发起取衣或护理任务。这样既提升使用体验，也能减少衣物被频繁翻动造成的褶皱、污染和错放。")
    add_matrix_table(doc, ["痛点", "现有表现", "本项目解决思路"], [
        ("衣物查找效率低", "用户依赖记忆和人工翻找，衣物位置变化后难以追踪。", "通过拍照建档、状态记录和挂位检测建立衣物数字档案。"),
        ("搭配判断成本高", "常需先取出试穿，满意后再整理，不满意还要归还。", "引入个人全身照上身效果预览，先判断再取衣。"),
        ("取放护理割裂", "取衣、清理、通风、除味通常由用户分散完成。", "将取衣、B 区毛刷清理、C 区热风/负离子/通风护理纳入统一流程。"),
        ("单板控制压力大", "视觉、云端、执行混在一处会增加调试和稳定性风险。", "采用树莓派、云端、主板1、主板2分层架构，各单元职责清晰。"),
    ], [2100, 3300, 3960])
    add_para(doc, "因此，本项目不仅是一个电机驱动衣架的装置，更是一个面向真实家庭衣物管理流程的综合型系统。它兼顾展示效果、工程可实现性和后续扩展空间，适合作为学生科研、创新创业和智能家居类竞赛作品继续完善。")
    add_page_break(doc)

    # Page 3
    add_heading(doc, "三、项目目标与建设内容", 1)
    add_para(doc, "项目总体目标是完成一套可展示、可联调、可扩展的智能衣橱原型系统，实现衣物从入库识别到取衣执行、从搭配预览到清理护理的闭环管理。系统建设遵循“上层业务清晰、底层执行可靠、软硬件边界明确”的原则，避免把网页、云端、视觉、实时控制和机械动作混在同一模块中。")
    add_heading(doc, "3.1 建设目标", 2)
    add_list(doc, [
        "实现衣物图片采集、上传、识别建档和衣物列表查询，形成基础衣物数字化管理能力。",
        "实现树莓派触摸屏网页现场交互，支持衣物浏览、取衣、清理、护理和状态显示。",
        "实现用户全身照上传与目标衣物上身效果预览，使选衣流程从“先取后试”改为“先看后取”。",
        "实现主板1与主板2的分层控制，主板1负责任务判定与传感器汇聚，主板2负责动作执行。",
        "实现处理区 B 毛刷清理、存储区 C 热风/负离子/通风护理和挂位限位检测的原型能力。",
    ])
    add_heading(doc, "3.2 建设内容", 2)
    add_matrix_table(doc, ["建设模块", "主要内容", "预期输出"], [
        ("交互与云端", "树莓派触摸屏网页、App/Web入口、衣物列表、状态同步、上身预览接口。", "可操作的前端页面和云端接口契约。"),
        ("视觉与边缘接入", "树莓派外接摄像头拍照、图片上传、串口桥接和结果回显。", "可完成入库拍照上传和任务接入的边缘节点。"),
        ("双主板电控", "STM32F407 主板1协调，主板2执行步进电机、舵机、夹爪和护理机构。", "稳定的板间通信与动作执行骨架。"),
        ("机构与传感器", "挂位限位开关、温湿度、除湿袋重力、B区毛刷、C区热风/负离子/风扇。", "支持取衣校验和衣物护理的实物机构。"),
        ("测试与申报材料", "整理流程、接口、测试项、演示脚本和成果材料。", "申报书、说明文档、展示视频/图片和验收记录。"),
    ], [1900, 4700, 2760])
    add_para(doc, "项目完成后，系统应能展示完整流程：用户在触摸屏或 App/Web 中查看衣物，上传全身照并查看上身效果预览，选择目标衣物后发起取衣；系统根据衣物状态和挂位检测结果进入执行流程，必要时完成毛刷清理或热风护理，最后回写状态。")
    add_page_break(doc)

    # Page 4
    add_heading(doc, "四、总体方案与系统架构", 1)
    add_para(doc, "系统采用“云端业务管理 + 树莓派边缘接入 + 双 STM32 分层控制 + 多执行机构”的总体架构。树莓派侧承担触摸屏网页承载、摄像头拍照上传和串口桥接；云端负责衣物数据、图片和任务状态；主板1作为调度层，汇聚除湿袋重力、温湿度、挂位限位等传感器信息，并向主板2下发动作任务；主板2作为执行层，控制步进电机、舵机、继电器和护理组件。")
    arch_image = get_render_safe_arch_image()
    if arch_image and arch_image.exists():
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        run = p.add_run()
        run.add_picture(str(arch_image), width=Inches(6.3))
        add_para(doc, "图 1  树莓派与双 STM32F407 分层电控架构示意", align=WD_ALIGN_PARAGRAPH.CENTER, size=9.5, color=MUTED, after=5)
    add_matrix_table(doc, ["层级", "组成", "职责边界"], [
        ("交互层", "树莓派触摸屏网页、App/Web", "承接用户点击、衣物浏览、全身照上传和任务发起，不直接输出电机脉冲。"),
        ("数据层", "云端后端", "保存衣物图片、状态、任务和预览结果，负责业务数据统一。"),
        ("边缘接入层", "树莓派外接摄像头、本机服务、串口桥接", "完成拍照上传、网页承载和与主板1的串口连接。"),
        ("协调层", "STM32F407 主板1", "接收树莓派串口指令，汇聚传感器，进行状态判定和流程编排。"),
        ("执行层", "STM32F407 主板2、驱动模块", "执行步进电机、舵机、夹爪、毛刷、热风机和开关量动作。"),
        ("机构层", "导轨、伸缩轴、夹爪、B区毛刷、C区护理组件、挂位限位", "完成真实物理动作并反馈挂位占用或执行状态。"),
    ], [1700, 2700, 4960])
    add_para(doc, "K230 与串口屏相关资料在本项目早期调试中具有参考价值，但本申报书按当前正式主线描述，不再把 K230 写作当前视觉与联网核心，也不把串口屏写作唯一现场交互入口。后续如需扩展，可将其作为视觉节点或备用调试链路接入。")

    # Page 5
    add_heading(doc, "五、关键流程与技术路线", 1)
    add_para(doc, "本项目技术路线可以概括为“用户意图上云、边缘节点接入、主板协调判定、执行机构落地、状态回写闭环”。前端和云端处理业务信息，底层主板处理实时控制，二者之间通过树莓派串口桥接和任务状态进行解耦。")
    add_heading(doc, "5.1 主要业务流程", 2)
    add_numbered(doc, [
        "衣物入库：用户放入衣物，树莓派外接摄像头拍照并上传云端，云端保存衣物图片、名称、状态和挂位信息，前端刷新衣物列表。",
        "上身预览：用户上传个人全身照并选择目标衣物，云端生成 try_on_image 预览图，用户根据效果决定是否取衣。",
        "自动取衣：用户点击取衣后，云端生成任务，树莓派通过串口将控制意图送入主板1，主板1结合挂位限位和状态判断后通知主板2执行。",
        "清理护理：系统可将衣物送至处理区 B 进行毛刷清理，或在存储区 C 通过热风机、负离子发生器和通风风扇完成护理。",
        "状态回写：取衣、清理、护理、异常和完成状态回到云端或本地界面，使用户能够看到当前任务进度。"
    ])
    add_heading(doc, "5.2 核心技术路线", 2)
    add_matrix_table(doc, ["方向", "方案", "作用"], [
        ("云端与前端", "采用 HTTP API 管理衣物列表、图片、状态、上身预览和硬件任务。", "让现场触摸屏和远程 App/Web 使用同一套业务数据。"),
        ("树莓派边缘节点", "运行触摸屏网页，控制外接摄像头，负责串口桥接。", "把云端业务和底层主板控制连接起来。"),
        ("双 STM32 控制", "主板1调度，主板2执行，板间命令传输动作意图。", "降低单板耦合度，提高动作控制稳定性和可维护性。"),
        ("多传感器判定", "温湿度、除湿袋重力、挂位限位开关、本地按键等汇聚到主板1。", "在执行前确认环境和挂位条件，减少误动作。"),
        ("护理机构设计", "B区毛刷、C区热风/负离子/通风组件独立于取衣主流程。", "从单纯取衣扩展到衣物维护和展示功能。"),
    ], [1900, 4550, 2910])
    add_callout(doc, "技术边界", "上身效果预览服务于选衣决策，不直接触发电机；网页上传的是业务意图，不是底层脉冲；限位开关证明挂位有衣，不单独证明衣物身份。")

    # Page 6
    add_heading(doc, "六、创新点与项目特色", 1)
    add_para(doc, "项目创新并不只体现在单一算法或单一硬件，而体现在衣物管理流程被重新组织：用户选择衣物前可以先看上身效果，衣物取放前有挂位检测，取出前后可按需清理护理，云端记录与本地执行形成闭环。")
    add_matrix_table(doc, ["创新点", "具体体现", "价值"], [
        ("上身效果预览接入衣橱", "用户上传个人全身照并选择衣物，云端生成预览图。", "把智能衣橱从“存取工具”升级为“穿搭决策辅助工具”。"),
        ("云边端分层控制", "树莓派网页和云端处理业务，STM32 主板处理实时控制。", "避免网页直接控制电机，提高系统边界清晰度。"),
        ("双主板分工", "主板1负责传感器汇聚和流程判定，主板2专注运动执行。", "便于扩展多轴、多舵机和护理机构。"),
        ("衣物清理护理一体化", "B区毛刷、C区热风、负离子和风扇共同服务衣物维护。", "不只解决取衣，还覆盖日常衣物维护需求。"),
        ("挂位占用检测", "每个挂衣点位设置限位开关，取衣前先确认目标点位状态。", "提高取衣定位可靠性，为后续 slot_id 闭环打基础。"),
    ], [2000, 4300, 3060])
    add_heading(doc, "七、实施方案与进度安排", 1)
    add_matrix_table(doc, ["阶段", "时间", "主要任务"], [
        ("第一阶段", "第 1-2 周", "整理现有硬件、接口、页面和固件资料，统一当前正式架构口径。"),
        ("第二阶段", "第 3-5 周", "完善树莓派触摸屏网页、摄像头拍照上传、云端状态接口和串口桥接。"),
        ("第三阶段", "第 6-8 周", "联调主板1、主板2、挂位限位、夹爪和取衣动作流程。"),
        ("第四阶段", "第 9-10 周", "实现 B 区毛刷清理与 C 区护理任务的控制流程和异常保护。"),
        ("第五阶段", "第 11-12 周", "完成整机演示、测试记录、申报材料、说明书和展示视频。"),
    ], [1600, 1700, 6060])
    add_para(doc, "实施中优先保证主链路可跑通，再逐步增加护理、推荐和身份增强功能。这样可以避免在同一阶段同时修改前端、云端、串口、固件和机械结构造成联调风险。")
    add_page_break(doc)

    # Page 7
    add_heading(doc, "八、可行性基础与风险控制", 1)
    add_para(doc, "项目已经具备较完整的工程基础。现有资料中包含智能衣橱方案说明、系统总览与执行流程、树莓派与双主板架构图、主板1和主板2的 STM32F407 工程、引脚配置文档、串口与 DMA 基础代码、UI 原型、答辩 PPT 和申报材料底稿。这些资料能够支撑项目继续向申报书、实物联调和展示验收推进。")
    add_heading(doc, "8.1 可行性基础", 2)
    add_list(doc, [
        "硬件方面：双 STM32F407 主板、电机/舵机控制、OLED、按键、限位开关、温湿度和重力传感器等基础清晰。",
        "软件方面：已形成云端接口口径、树莓派触摸屏网页定位、衣物列表和图片上传流程。",
        "控制方面：主板1具备串口接入和传感器汇聚基础，主板2具备步进电机与舵机执行基础。",
        "资料方面：已有方案说明、流程文档、图示素材和 PPT 材料，可快速转化为申报、答辩和说明书。"
    ])
    add_heading(doc, "8.2 风险与对策", 2)
    add_matrix_table(doc, ["风险", "影响", "控制措施"], [
        ("视觉识别不稳定", "衣物名称、类别或状态可能判断不准。", "保留人工确认入口，先确保图片建档和状态管理，再逐步提高识别准确率。"),
        ("串口链路丢包", "任务可能无法可靠下发或反馈。", "增加帧头、校验、超时、重试和动作完成反馈。"),
        ("机械动作误差", "夹爪定位、伸缩轴行程和挂位位置可能偏差。", "先固定少量挂位进行标定，再逐步扩展挂位数量。"),
        ("护理机构安全", "热风或风扇长时间工作存在风险。", "设置最长工作时间、异常停止、空挂位不执行和人工急停。"),
        ("云端依赖", "断网时无法上传或同步状态。", "保留本地提示和缓存状态，网络恢复后再同步。"),
    ], [2100, 3300, 3960])
    add_para(doc, "项目风险总体可控。关键是把“已实现能力、接口反推能力、后续规划能力”分开验收：先完成可演示主流程，再完善异常处理和库存闭环，最后扩展 RFID/NFC、推荐算法和更多护理模式。")
    add_page_break(doc)

    # Page 8
    add_heading(doc, "九、预期成果与应用价值", 1)
    add_para(doc, "项目预期形成一套可展示的智能衣橱原型系统和完整申报材料。成果既包括实物系统，也包括软件、固件、文档、流程图、测试记录和展示内容，便于参加创新创业、科研立项、智能家居或机电控制类项目申报。")
    add_matrix_table(doc, ["成果类别", "具体成果"], [
        ("实物成果", "智能衣橱原型机、树莓派触摸屏交互端、外接摄像头、双 STM32F407 控制板、B 区清理和 C 区护理机构。"),
        ("软件成果", "触摸屏网页、衣物数据管理接口、上身效果预览流程、树莓派拍照上传和串口桥接程序。"),
        ("控制成果", "主板1协调程序、主板2执行程序、板间通信协议、挂位检测和动作流程。"),
        ("文档成果", "项目申报书、技术说明、流程图、接口说明、测试方案、答辩 PPT 和演示脚本。"),
        ("拓展成果", "后续可申报软件著作权、实用新型专利、竞赛作品或校园科研项目结题材料。"),
    ], [2100, 7260])
    add_heading(doc, "十、推广前景", 1)
    add_para(doc, "本项目具有较强的家庭应用和展示推广价值。对普通家庭用户而言，系统能降低找衣、选衣、取衣和护理成本；对老年人、行动不便人群和衣物较多的用户而言，自动取衣与状态管理能够提升便利性；对学校创新实践而言，该项目综合了云端服务、前端交互、嵌入式控制、机械结构、传感器、执行机构和项目文档表达，适合作为跨学科实践载体。")
    add_para(doc, "后续推广可分为三个方向：一是继续完善实物稳定性，使系统能够长期演示；二是扩展衣物身份识别和库存一致性，例如将 slot_id 与视觉识别、后续 RFID/NFC 或标签机制结合；三是优化上身预览、搭配推荐和护理策略，使系统从“自动衣橱”进一步升级为“衣物管理与穿搭服务平台”。")
    add_callout(doc, "阶段性结论", "本项目具备明确的应用场景、完整的技术链路和可持续完善的工程基础，适合以实物类智能家居项目进行申报。")

    doc.core_properties.title = "智能衣橱项目申报书"
    doc.core_properties.subject = "8页左右正文版"
    doc.core_properties.author = "Codex"
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    doc.save(OUT_PATH)
    return OUT_PATH


if __name__ == "__main__":
    print(build())
