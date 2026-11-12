# -*- coding: utf-8 -*-
from pathlib import Path

from docx import Document
from docx.enum.table import WD_ALIGN_VERTICAL, WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt


ROOT = Path(__file__).resolve().parent
OUT_DIR = ROOT / "申报书_电控"
OUT_PATH = OUT_DIR / "智能衣橱电控系统_优化版申报书.docx"
PROMPT_PATH = OUT_DIR / "智能衣橱电控配图_Image2提示词.md"

IMG_ARCH = Path(r"C:\Users\ECHO\Desktop\1 (2).png")
IMG_STATE = Path(r"C:\Users\ECHO\Desktop\3.png")
FONT = "宋体"
HEAD_FONT = "黑体"
LATIN_FONT = "Times New Roman"
ROMAN = ["I", "II", "III", "IV", "V", "VI"]


def set_run_font(run, size=10.5, bold=False, font=FONT):
    run.font.name = LATIN_FONT
    run._element.rPr.rFonts.set(qn("w:ascii"), LATIN_FONT)
    run._element.rPr.rFonts.set(qn("w:hAnsi"), LATIN_FONT)
    run._element.rPr.rFonts.set(qn("w:eastAsia"), font)
    run.font.size = Pt(size)
    run.bold = bold


def set_para(p, first_line=True, before=0, after=0, line=1.55):
    p.paragraph_format.space_before = Pt(before)
    p.paragraph_format.space_after = Pt(after)
    p.paragraph_format.line_spacing = line
    if first_line:
        p.paragraph_format.first_line_indent = Cm(0.74)


def add_p(doc, text, first_line=True):
    p = doc.add_paragraph()
    set_para(p, first_line=first_line)
    r = p.add_run(text)
    set_run_font(r, 10.5)
    return p


def add_heading(doc, text, level):
    p = doc.add_paragraph()
    set_para(p, first_line=False, before=7 if level == 1 else 4, after=2, line=1.35)
    r = p.add_run(text)
    if level == 1:
        set_run_font(r, 14, True, HEAD_FONT)
    elif level == 2:
        set_run_font(r, 12, True, HEAD_FONT)
    else:
        set_run_font(r, 10.5, True, FONT)
    return p


def add_caption(doc, text):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(2)
    p.paragraph_format.space_after = Pt(4)
    p.paragraph_format.line_spacing = 1.15
    r = p.add_run(text)
    set_run_font(r, 9)
    return p


def add_picture(doc, path, caption, width_cm=13.0):
    if path.exists():
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.paragraph_format.space_before = Pt(4)
        p.paragraph_format.space_after = Pt(2)
        p.add_run().add_picture(str(path), width=Cm(width_cm))
        add_caption(doc, caption)
    else:
        add_caption(doc, caption + "（图片待补）")


def add_image_placeholder(doc, caption, note, height_cm=5.0):
    table = doc.add_table(rows=1, cols=1)
    table.style = "Table Grid"
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = False
    cell = table.rows[0].cells[0]
    cell.width = Cm(14.0)
    cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    set_cell_margins(cell, top=240, bottom=240, start=160, end=160)
    tr_pr = table.rows[0]._tr.get_or_add_trPr()
    tr_height = OxmlElement("w:trHeight")
    tr_height.set(qn("w:val"), str(int(height_cm * 567)))
    tr_height.set(qn("w:hRule"), "atLeast")
    tr_pr.append(tr_height)
    p = cell.paragraphs[0]
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.line_spacing = 1.25
    r = p.add_run(note)
    set_run_font(r, 10)
    add_caption(doc, caption)


def set_cell_margins(cell, top=80, bottom=80, start=100, end=100):
    tc_pr = cell._tc.get_or_add_tcPr()
    tc_mar = tc_pr.find(qn("w:tcMar"))
    if tc_mar is None:
        tc_mar = OxmlElement("w:tcMar")
        tc_pr.append(tc_mar)
    for tag, val in (("top", top), ("bottom", bottom), ("start", start), ("end", end)):
        node = tc_mar.find(qn(f"w:{tag}"))
        if node is None:
            node = OxmlElement(f"w:{tag}")
            tc_mar.append(node)
        node.set(qn("w:w"), str(val))
        node.set(qn("w:type"), "dxa")


def add_table(doc, headers, rows, widths=None, caption=None):
    table = doc.add_table(rows=1, cols=len(headers))
    table.style = "Table Grid"
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = False
    for i, h in enumerate(headers):
        cell = table.rows[0].cells[i]
        set_cell_margins(cell)
        cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        set_run_font(r, 9.5, True)
        if widths:
            cell.width = Cm(widths[i])
    for row_data in rows:
        row = table.add_row()
        for i, text in enumerate(row_data):
            cell = row.cells[i]
            set_cell_margins(cell)
            cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
            p = cell.paragraphs[0]
            p.paragraph_format.line_spacing = 1.15
            p.paragraph_format.space_after = Pt(0)
            r = p.add_run(text)
            set_run_font(r, 9.2)
            if widths:
                cell.width = Cm(widths[i])
    if caption:
        add_caption(doc, caption)
    else:
        doc.add_paragraph()
    return table


def add_roman_block(doc, index, title, text):
    p = doc.add_paragraph()
    set_para(p, first_line=True)
    r = p.add_run(f"{ROMAN[index]}．{title}：")
    set_run_font(r, 10.5, True)
    r2 = p.add_run(text)
    set_run_font(r2, 10.5)


def configure_document(doc):
    sec = doc.sections[0]
    sec.page_width = Cm(21)
    sec.page_height = Cm(29.7)
    sec.top_margin = Cm(2.54)
    sec.bottom_margin = Cm(2.2)
    sec.left_margin = Cm(2.65)
    sec.right_margin = Cm(2.35)
    sec.header_distance = Cm(1.3)
    sec.footer_distance = Cm(1.2)

    normal = doc.styles["Normal"]
    normal.font.name = LATIN_FONT
    normal._element.rPr.rFonts.set(qn("w:ascii"), LATIN_FONT)
    normal._element.rPr.rFonts.set(qn("w:hAnsi"), LATIN_FONT)
    normal._element.rPr.rFonts.set(qn("w:eastAsia"), FONT)
    normal.font.size = Pt(10.5)
    normal.paragraph_format.line_spacing = 1.55
    normal.paragraph_format.space_after = Pt(0)

    footer = sec.footer.paragraphs[0]
    footer.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = footer.add_run()
    fld_begin = OxmlElement("w:fldChar")
    fld_begin.set(qn("w:fldCharType"), "begin")
    instr = OxmlElement("w:instrText")
    instr.set(qn("xml:space"), "preserve")
    instr.text = "PAGE"
    fld_end = OxmlElement("w:fldChar")
    fld_end.set(qn("w:fldCharType"), "end")
    run._r.append(fld_begin)
    run._r.append(instr)
    run._r.append(fld_end)


def build_doc():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    doc = Document()
    configure_document(doc)

    title = doc.add_paragraph()
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title.paragraph_format.space_after = Pt(10)
    tr = title.add_run("智能衣橱电控系统申报书")
    set_run_font(tr, 16, True, HEAD_FONT)

    add_heading(doc, "2.2 电控系统设计", 1)
    add_p(
        doc,
        "本项目电控系统的核心任务，是将用户侧取衣、放衣、识别、清理和护理等意图，转化为可执行、可反馈、可保护的机械动作。系统围绕树莓派边缘接入、双STM32F407分层控制、点位路径规划、状态机执行和安全反馈建立完整控制链路，使上层交互、任务调度和底层机构动作能够协同工作。"
    )

    add_heading(doc, "2.2.1 总体控制架构", 2)
    add_p(
        doc,
        "智能衣橱电控系统采用分层协同架构，如图1所示。树莓派负责触摸屏网页、摄像头接入和云端通信，是上层任务与硬件控制之间的边缘接入节点；STM32F407主板1作为系统协调中心，负责接收树莓派串口任务、汇聚传感器状态、判断执行条件并下发动作命令；STM32F407主板2作为运动执行板，负责步进电机、舵机、继电器、热风机等执行机构控制。"
    )
    add_picture(doc, IMG_ARCH, "图1 智能衣橱电控系统总体控制架构", 14.2)
    add_p(
        doc,
        "该架构的关键优势在于职责边界清晰：树莓派不直接输出电机控制脉冲，主板1不直接承担大规模运动执行，主板2不参与云端通信和视觉识别。系统由此形成“上层表达任务、主板1组织流程、主板2执行动作、传感器反馈状态”的控制闭环。"
    )

    add_heading(doc, "2.2.2 主控与执行单元设计", 2)
    add_roman_block(
        doc, 0, "树莓派边缘接入单元",
        "树莓派连接外接触摸屏并运行本地网页，承担现场人机交互入口。用户在触摸屏网页、App或Web端发起取衣、放衣、清理、护理等操作后，树莓派负责与云端后端通信，并通过串口将需要执行的任务信息传递给STM32F407主板1。同时，树莓派连接外接摄像头，用于衣物拍照、图片上传和识别结果回显。"
    )
    add_roman_block(
        doc, 1, "STM32F407主板1调度单元",
        "主板1是系统协调板，其功能类似电控系统的调度中心。它接收树莓派串口下发的任务，读取温湿度、重力、限位、按键和到位等状态量，判断当前任务是否满足执行条件；在条件满足后，主板1将任务拆解为主板2可执行的动作序列，并负责异常拦截、状态回传和流程阶段管理。"
    )
    add_roman_block(
        doc, 2, "STM32F407主板2执行单元",
        "主板2是运动执行板，主要控制步进电机、舵机、继电器、热风机、毛刷和护理组件等执行机构。主板2不处理云端接口和衣物识别，而是接收主板1下发的动作命令，按照动作状态机逐步执行移动、夹取、释放、清理和护理动作。"
    )
    add_roman_block(
        doc, 3, "板间UART通信链路",
        "树莓派与主板1之间通过UART串口完成任务下发和状态回传，主板1与主板2之间通过板间UART通信完成流程协调、动作命令下发和执行状态反馈。该链路使任务流和动作流分离，既便于调试，也便于后续扩展更多功能模块。"
    )
    add_image_placeholder(
        doc,
        "图2 智能衣橱任务指令流转示意图",
        "此处插入任务指令流转图：用户触摸屏/App/Web -> 云端后端任务队列 -> 树莓派边缘服务 -> STM32F407主板1任务调度 -> STM32F407主板2动作执行 -> 夹爪、导轨、毛刷、热风护理组件。",
        5.0,
    )
    add_p(
        doc,
        "从控制关系看，树莓派负责“任务进入系统”，主板1负责“判断任务能否执行并组织流程”，主板2负责“把流程转换为实际动作”。传感器、限位和到位反馈贯穿其中，用于确认机构状态并触发必要的保护。这样的分工能够避免单一控制板同时承担联网、识别、调度和运动执行等多类任务。"
    )

    add_heading(doc, "2.2.3 传感器与状态反馈设计", 2)
    add_p(
        doc,
        "为保证取放衣和护理过程的可靠性，系统设置了多类状态反馈信号。与普通传感器罗列不同，本项目中的传感器直接参与执行前校验、执行中监测和异常处理，形成面向任务闭环的状态反馈体系。"
    )
    add_table(
        doc,
        ["反馈量", "作用位置", "参与逻辑"],
        [
            ["温湿度传感器", "衣橱内部或护理区域", "为热风、通风和衣物护理过程提供环境依据。"],
            ["除湿袋重力传感器", "除湿袋或承载检测位置", "辅助判断除湿耗材、局部承载或状态变化。"],
            ["限位开关", "挂位、回零点和机构边界", "判断挂位占用、机构是否到位以及是否触发边界保护。"],
            ["按键/急停", "本地调试与人工干预位置", "用于紧急停止、调试和人工安全保护。"],
            ["到位/超时反馈", "主板2运动执行链路", "判断动作是否完成，超时则进入异常停止流程。"],
            ["摄像头图像反馈", "树莓派外接摄像头", "用于衣物拍照、识别、建档和执行结果确认。"],
        ],
        widths=[3.4, 4.6, 7.8],
        caption="表1 智能衣橱传感器与状态反馈功能",
    )
    add_p(
        doc,
        "主板1在执行任务前，会综合目标挂位、机构姿态、传感器状态和急停状态进行判断；执行过程中，主板2持续反馈动作阶段和到位状态；若出现限位异常、动作超时或急停触发，系统立即停止当前动作，并进入安全恢复或等待人工确认状态。"
    )

    add_heading(doc, "2.2.4 点位路径规划与防干涉控制", 2)
    add_p(
        doc,
        "智能衣橱内部空间紧凑，夹爪、挂位、衣物、毛刷和护理组件之间存在潜在干涉风险。因此，系统将A区取放衣口、B区清理区、B0识别拍照点、C区存储护理区、前置点和回家点抽象为功能点位表。不同任务不直接驱动电机盲目移动，而是先由主板1选择目标点和安全路径，再由主板2按点位序列执行。"
    )
    add_image_placeholder(
        doc,
        "图3 智能衣橱点位路径规划与防干涉示意图",
        "此处插入点位路径规划图：标出A区取放衣口、B区清理区、B0识别拍照点、C区存储护理区、前置点、中转点、回家点，并用路径箭头标出推荐路径和禁止直达路径。",
        5.4,
    )
    add_p(
        doc,
        "路径规划遵循“安全高度优先、前置点进入、中转点过渡、禁止低高度横移”的原则。夹爪在进入A区、C区等狭窄区域前，先抬升到安全高度，再进行X/Y平面移动，最后下降进入目标点，从而降低衣物、夹爪与柜体结构发生碰撞的风险。"
    )
    add_p(
        doc,
        "在平面运动控制方面，X轴和Y轴共同决定夹爪轨迹。如果某一轴提前到位，夹爪轨迹会形成折线，造成方向突变和衣物晃动。因此系统引入X/Y双轴同步到点策略，根据两轴位移差异协调速度或启动时序，使夹爪尽量以连续轨迹到达目标点。"
    )
    add_image_placeholder(
        doc,
        "图4 X/Y同步到点轨迹对比",
        "此处插入X/Y同步轨迹对比图：左侧为不同步控制形成折线路径，右侧为同步到点控制形成连续轨迹。",
        4.6,
    )
    add_p(
        doc,
        "点位表、安全高度、前置点和同步轨迹共同构成电控层的防干涉控制策略。主板1在任务开始前确定目标点位和路径顺序，主板2按照点位序列执行具体动作；如果任一阶段出现限位异常、到位超时或急停信号，则立即中断当前流程并等待重新调度。"
    )

    add_heading(doc, "2.2.5 表驱动动作状态机与典型流程", 2)
    add_p(
        doc,
        "主板2采用动作状态机控制方式，将复杂任务拆解为移动、等待、夹取、释放、伸缩、清理、护理等基础动作单元。每个动作执行结束后，系统会根据到位反馈、传感器状态和异常标志判断下一步流程：条件满足时进入后续动作，条件不满足时进入等待、重试或安全停止状态。该方式既保证了正常流程的顺序性，也提高了异常情况下的可控性和安全性。"
    )
    add_picture(doc, IMG_STATE, "图5 智能衣橱动作状态机示意图", 11.2)
    add_p(
        doc,
        "典型流程包括：放衣/入库任务按照“A区夹取、B/B0识别、C区存放”的顺序完成；取衣/输出任务按照“C区夹取、前置点过渡、B点中转、A区释放”的顺序完成；B区清理任务在衣物转运至清理区后执行毛刷动作循环；C区护理任务则调用热风、负离子和通风组件完成烘干、除味和空气流通辅助。"
    )

    add_heading(doc, "2.2.6 安全保护与异常处理", 2)
    add_p(
        doc,
        "安全保护贯穿任务执行全过程。系统将安全机制划分为执行前校验、执行中监测、异常停止、状态回写和安全恢复五个阶段。急停命令具有最高优先级，动作超时、限位异常或状态不一致时，系统应立即停止当前动作，避免机构继续运行造成碰撞、夹持失败或衣物损伤。"
    )
    add_image_placeholder(
        doc,
        "图6 电控系统安全保护与异常处理机制",
        "此处插入安全保护机制图：执行前校验 -> 执行中监测 -> 异常处理 -> 状态回写 -> 安全恢复，标出急停、超时、限位异常和安全停止分支。",
        5.0,
    )
    add_p(
        doc,
        "安全保护流程不单独依赖某一个传感器，而是由任务条件、机构位置、反馈状态和人工急停共同构成。执行前，系统确认目标挂位、机构状态和传感器条件；执行中，系统根据到位反馈和超时判断确认动作是否可靠完成；异常发生后，系统停止输出并将失败或异常状态反馈给树莓派和上层页面。"
    )
    add_p(
        doc,
        "综上，本项目电控系统的创新点不只是使用了两块STM32F407主控板，而是通过清晰的分层架构，将任务调度、路径规划、动作状态机和安全反馈组织成完整闭环。该方案能够支撑智能衣橱实现自动取放衣、衣物识别、B区清理和C区护理，并为后续增加挂位、护理动作和更多状态反馈保留扩展空间。"
    )

    doc.save(OUT_PATH)
    return OUT_PATH


def build_prompts():
    prompts = """# 智能衣橱电控配图 Image2 提示词

使用说明：每次只复制一个提示词到 Image2。所有图都要求白底、工程说明书风格、中文标签清晰、不要卡通、不要炫光、不要复杂背景。

## 图2 任务指令流转图

请生成一张“智能衣橱电控任务指令流转示意图”。项目背景：这是一个智能衣橱系统，不是普通衣架。系统由树莓派、云端后端、两块STM32F407主板和衣橱机械执行机构组成。树莓派连接触摸屏网页和外接摄像头，负责用户操作、拍照上传、云端通信和串口桥接；STM32F407主板1是系统协调中心，负责任务解析、传感器状态判断、流程调度和异常拦截；STM32F407主板2是运动执行板，负责步进电机、舵机、继电器、热风机、毛刷和护理组件动作。

画面要求：白色背景，横向流程图，从左到右依次画出六个模块：1 用户触摸屏/App/Web，2 云端后端任务队列，3 树莓派边缘服务，4 STM32F407主板1任务调度，5 STM32F407主板2动作执行，6 夹爪/导轨/毛刷/热风护理组件。模块之间用橙色或蓝色箭头连接，箭头文字分别写“用户任务”“任务生成”“UART串口桥接”“动作命令”“执行反馈”。底部再加一条回传箭头，从执行机构回到云端，标注“状态回写/完成反馈”。中文标签要准确、清晰、适合申报书插图，工程图风格，不要卡通，不要人物，不要复杂背景。

## 图3 点位路径规划与防干涉图

请生成一张“智能衣橱内部点位路径规划与防干涉控制示意图”。项目背景：智能衣橱内部有A区取放衣口、B区清理区、B0识别拍照点、C区存储护理区，夹爪需要在这些区域之间移动衣物。为了避免夹爪、衣物、柜体、毛刷和护理组件发生干涉，系统采用点位表路径规划、安全高度优先、前置点进入和中转点过渡策略。

画面要求：白底二维俯视工程图，画一个简化衣橱内部矩形空间，清楚标出“A区取放衣口”“B区清理区”“B0识别拍照点”“C区存储护理区”“前置点”“中转点”“回家点”。用绿色实线箭头表示推荐路径：A区 -> B0识别点 -> C区，C区 -> 前置点 -> B点中转 -> A区。用红色虚线表示禁止直达或干涉风险路径，并标注“禁止低高度直达”“干涉风险”。在图角落加入小图例：绿色=安全路径，红色虚线=禁止路径，蓝色点=功能点位。中文标签清晰，线条简洁，专业机械电控方案图风格，不要真实照片，不要3D炫光，不要复杂背景。

## 图4 表驱动动作状态机图

请生成一张“智能衣橱表驱动动作状态机示意图”。项目背景：智能衣橱使用两块STM32F407控制板，主板1负责调度，主板2负责执行。主板2不是直接按按钮驱动单个电机，而是按照动作状态机执行移动、等待、夹取、释放、清理、护理等标准动作单元。

画面要求：白底流程图，主流程从左到右或从上到下：待命 -> 任务解析 -> 执行前校验 -> 抬升到安全高度 -> 移动到目标点 -> 夹取/释放 -> 清理/护理动作 -> 到位反馈 -> 状态回写 -> 完成。旁边画异常分支：急停、动作超时、限位异常，三者都指向“安全停止”，再指向“等待人工确认/重新调度”。每个节点使用简洁矩形或圆角矩形，箭头清晰，中文标签准确。风格为技术说明书流程图，蓝灰色线条，少量橙色强调异常分支，不要卡通，不要人物，不要复杂背景。

## 图5 安全保护机制图

请生成一张“智能衣橱电控安全保护机制示意图”。项目背景：智能衣橱通过树莓派、STM32F407主板1、STM32F407主板2和多类传感器控制夹爪、导轨、毛刷、热风机等机构。安全保护分为执行前校验、执行中监测、异常处理、状态回写和安全恢复五个阶段。

画面要求：白底分层流程图，横向五段结构：第一段“执行前校验”，包含任务类型、目标挂位、机构状态、传感器状态；第二段“执行中监测”，包含到位反馈、动作超时、限位检测；第三段“异常处理”，包含急停、停止输出、保护机构；第四段“状态回写”，包含完成、失败、异常；第五段“安全恢复”，包含待机、安全位置、人工确认。用蓝色表示正常流程，用红色表示异常触发，用绿色表示恢复完成。中文标签清晰，正式工程图风格，适合放入大学生创新创业项目申报书，不要卡通，不要照片背景。
"""
    PROMPT_PATH.write_text(prompts, encoding="utf-8")
    return PROMPT_PATH


if __name__ == "__main__":
    print(build_doc())
    print(build_prompts())
