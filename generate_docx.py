from docx import Document
from docx.shared import Pt, Inches, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import os

doc = Document()
style = doc.styles['Normal']
font = style.font
font.name = '宋体'
font.size = Pt(12)
style.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')

def heading(text, level=1):
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        run.font.name = '黑体'
        run.element.rPr.rFonts.set(qn('w:eastAsia'), '黑体')
    return h

def para(text, bold=False, size=12, align=None):
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.name = '宋体'
    run.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    run.font.size = Pt(size)
    run.bold = bold
    if align:
        p.alignment = align
    return p

def set_cell_shading(cell, color):
    shading = OxmlElement('w:shd')
    shading.set(qn('w:fill'), color)
    shading.set(qn('w:val'), 'clear')
    cell._tc.get_or_add_tcPr().append(shading)

def make_table(headers, rows, col_widths=None):
    table = doc.add_table(rows=1 + len(rows), cols=len(headers))
    table.style = 'Table Grid'
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    # Header
    for i, h in enumerate(headers):
        cell = table.rows[0].cells[i]
        cell.text = h
        for p in cell.paragraphs:
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in p.runs:
                run.bold = True
                run.font.size = Pt(10)
                run.font.name = '宋体'
                run.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
        set_cell_shading(cell, '1A237E')
        for p in cell.paragraphs:
            for run in p.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
    # Body
    for r, row in enumerate(rows):
        for c, val in enumerate(row):
            cell = table.rows[r+1].cells[c]
            cell.text = val
            for p in cell.paragraphs:
                for run in p.runs:
                    run.font.size = Pt(10)
                    run.font.name = '宋体'
                    run.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    doc.add_paragraph()
    return table


# ─── TITLE ───
title_p = doc.add_paragraph()
title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
run = title_p.add_run('智能衣橱申报书 — 电控系统技术方案')
run.font.size = Pt(22)
run.font.name = '黑体'
run.element.rPr.rFonts.set(qn('w:eastAsia'), '黑体')
run.bold = True

doc.add_paragraph()

# ─── 一、总体架构与分层设计 ───
heading('一、总体架构与分层设计', 1)
para('本项目电控系统采用"上层意图输入 + 主板1调度 + 主板2执行 + 传感器反馈"四层架构，将人机交互、任务调度、底层运动三者解耦。')

make_table(
    ['层级', '组成', '职责'],
    [
        ['意图输入层', '树莓派触摸屏 / App / Web / 云端后端', '产生取衣、放衣、拍照、护理等任务意图，不直接控制硬件'],
        ['调度控制层', 'STM32F407 主板1', '接收任务 → 校验状态 → 调用传感器 → 编排流程 → 下发动作命令'],
        ['运动执行层', 'STM32F407 主板2', '驱动5轴步进电机、夹爪舵机、伸缩机构、毛刷和护理组件'],
        ['状态反馈层', '激光测距 / 限位开关 / 按键 / 到位信号', '执行前校验、动作完成判断、异常保护依据'],
    ]
)

para('注：图1 智能衣橱电控系统四层架构图 见附件。')

# ─── 二、硬件设计与接口分配 ───
heading('二、硬件设计与接口分配', 1)
heading('2.1 双板选型与分工', 2)

make_table(
    ['', '主板1（协调调度板）', '主板2（运动执行板）'],
    [
        ['主控芯片', 'STM32F407', 'STM32F407'],
        ['核心职责', '流程编排、外设通信、任务解析', '电机驱动、传感器采集、动作执行'],
        ['调试手段', 'OLED显示 + USMART命令行', 'OLED显示 + 4键本地调试'],
    ]
)

para('两板物理独立，通过UART1 DMA高速通信，职责边界清晰：主板1不直接操作电机，主板2不处理网络和云端逻辑。')

heading('2.2 主板1 外设接口分配', 2)
para('主板1承载全部对外通信，6路UART各司其职：')

make_table(
    ['串口', '连接对象', '功能', '波特率'],
    [
        ['UART1', '主板2', '板间命令帧（DMA模式）', '115200'],
        ['UART2', 'ESP32-S3', '预留（初始化+基础发送）', '115200'],
        ['UART3', 'K230摄像头', '拍照指令 / 配网参数 / 识别结果', '115200'],
        ['UART4', '激光测距模块', '衣物位置距离采集', '115200'],
        ['UART5', '步进电机', '单路电机控制', '115200'],
        ['UART6', '串口屏', '备用显示 / 调试配网', '115200'],
    ]
)

heading('2.3 主板2 执行机构分配', 2)
para('主板2专注运动控制，5路步进电机各占独立串口：')

make_table(
    ['串口', '控制对象', '说明'],
    [
        ['UART1', '—', '与主板1通信'],
        ['UART2', '步进电机 ID1 + ID2', '双电机反向联动，实现夹爪X轴开合'],
        ['UART3', '步进电机 M2', 'Y轴移动'],
        ['UART4', '步进电机 M3', 'Z轴升降'],
        ['UART5', '步进电机 M4', '伸缩机构'],
        ['UART6', '步进电机 M5', '辅助轴'],
    ]
)

para('注：图2 硬件接口拓扑图 见附件。')

# ─── 三、通信协议与数据流 ───
heading('三、通信协议与数据流', 1)

heading('3.1 板间通信协议', 2)
para('主板1与主板2之间采用自定义帧协议，UART1 DMA高速传输。帧格式：帧头0xFF + 命令码1B + 参数1(2B) + 参数2(2B) + 校验和1B。支持命令类型：')

make_table(
    ['命令', '功能'],
    [
        ['CMD_STEPPER_ENABLE', '电机使能/失能'],
        ['CMD_STEPPER_MOVE_ANGLE', '指定角度运动'],
        ['CMD_GET_SENSOR', '查询传感器状态'],
        ['CMD_EMERGENCY', '紧急停止'],
    ]
)

heading('3.2 主板1与K230交互协议', 2)
para('K230通过UART3连接主板1，协议分两种模式：')

make_table(
    ['模式', '用途', '格式'],
    [
        ['二进制帧', '拍照触发 / 状态查询', '帧头 + 命令 + 校验'],
        ['ASCII文本', 'Wi-Fi配网参数传输', 'SSID + 密码明文转发'],
    ]
)

para('K230拍到照片后自行通过WiFi上传云端，识别结果通过UART3回传style_id给主板1，主板1据此匹配取衣任务。')

heading('3.3 激光测距数据流', 2)
para('UART4接收激光模块原始数据帧，在TIM7中断中解析。每20ms进行一次数据处理，输出距离值(mm)，供control层判定衣物所在区间(C1~C4)。数据流向：激光模块 → UART4 → 中断解析 → 20ms软任务处理 → 区间判定(C1~C4) → 运动决策。')

para('注：图3 电控系统数据流图 见附件。')

# ─── 四、控制策略 ───
heading('四、控制策略', 1)

heading('4.1 任务调度机制', 2)
para('主板1作为调度核心，通过HTTP轮询云端任务队列（GET /api/tasks/pending，间隔1~3s），获取待执行任务。核心特点：上层只负责"要做什么"，主板1负责"能不能做、按什么顺序做"，主板2负责"具体怎么动"。')

make_table(
    ['步骤', '操作', '说明'],
    [
        ['1', '轮询获取任务', 'HTTP GET，FIFO顺序'],
        ['2', '解析任务类型', 'fetch / store-new / store-return'],
        ['3', '校验执行条件', '目标挂位状态、机构空闲、传感器正常'],
        ['4', '编排执行流程', '调度K230拍照 → 规划路径 → 下发动作序列'],
        ['5', '主板2执行反馈', '逐步执行并回传状态'],
        ['6', '完成回写', 'POST /api/tasks/{id}/complete → 同步衣物状态'],
    ]
)

heading('4.2 表驱动动作状态机', 2)
para('主板2采用表驱动状态机，将复杂任务拆解为标准动作单元串联：')

make_table(
    ['动作单元', '控制对象', '触发条件', '完成条件'],
    [
        ['移动到目标点', 'X/Y/Z轴、伸缩轴', '上一步完成', '到位信号'],
        ['夹取', '夹爪舵机', '到达目标位置', '动作延时完成'],
        ['释放', '夹爪舵机', '到达释放位置', '动作延时完成'],
        ['姿态调整', 'Z轴+舵机', '移动到安全高度后', '到位信号'],
        ['清理动作', 'B区毛刷电机', '夹持衣物到位', '毛刷完成循环'],
        ['护理动作', '热风/负离子/风扇', '衣物挂入C区', '定时完成'],
    ]
)
para('新增动作只需扩展状态表，无需改动底层驱动。')

heading('4.3 点位表路径规划', 2)
para('衣橱内部空间紧凑，采用功能点位表抽象各工作区（A区取衣口、B区清理区、B0识别拍照点、C区存储护理区）。路径规划遵循"安全高度优先"原则：先抬Z轴到安全高度 → X/Y平面移动 → 降Z进入目标点。进入狭窄区域前先经过前置偏移点，避免直接斜插。')

heading('4.4 X/Y双轴同步到点控制', 2)
para('X/Y两轴共同决定夹爪平面运动轨迹。若不协调，位移短的轴会提前到位停止，实际轨迹由平滑斜线变为折线，增加贴近柜体边界的风险。本系统引入同步到点策略：计算两轴目标位移差，对短轴降低速度或延后启动，使两轴同步到达，形成连续平滑轨迹。')

make_table(
    ['对比项', '不同步', '同步'],
    [
        ['轨迹形态', '折线，方向突变', '平滑连续'],
        ['空间安全', '易贴近边界', '轨迹可预测'],
        ['衣物姿态', '转运不连续', '姿态稳定'],
    ]
)

para('注：图4 控制策略综合示意图 见附件。')

# ─── 五、安全保护机制 ───
heading('五、安全保护机制', 1)
para('安全可靠性贯穿任务执行全过程，分三个层面：')

make_table(
    ['层面', '措施', '说明'],
    [
        ['执行前校验', '任务合法性、目标挂位状态、机构安全状态、传感器在线', '不满足条件则暂缓执行'],
        ['执行中监测', '电机到位反馈+超时机制、限位开关作软件安全边界、激光实时避障', '超时视为故障'],
        ['异常处理', '急停（最高优先级）、动作超时→回零、限位触发→反向脱离、通信超时→待机', '确保机构不继续危险动作'],
    ]
)

para('主板2硬件支持加速度限幅宏BOARD2_ACCEL_U8(x)，确保协议字段不溢出导致异常运动。')

para('注：图5 安全保护层级示意图 见附件。')

# ─── 六、典型任务闭环 ───
heading('六、典型任务闭环', 1)

heading('6.1 取衣流程', 2)
para('用户点击取衣(App/屏幕) → 云端生成fetch任务(含item_ids) → 主板1轮询到任务 → 调度K230拍照确认位置 → 云端识别返回style_id → 主板1判定路径：C区目标挂位→前置点→B中转→A区出口 → 主板2执行：抬Z安全高度→X/Y同步移动→降Z→夹取→抬Z→转运→A区释放（每步到位反馈） → 取衣完成 → 回写云端 → 更新衣物状态为"已取出"。')

heading('6.2 放衣/入库流程', 2)
para('用户放入衣物 → K230拍照入库 → 云端生成store-new任务 → 主板1调度：A区夹取→B0拍照→C区存放 → 主板2执行完整点位序列 → 完成回写。')

heading('6.3 护理流程', 2)
para('由B区毛刷清理和C区热风/负离子护理两类动作组成，分别由云端care任务触发，主板1根据任务类型调度主板2执行对应动作单元。')

para('注：图6 典型任务执行流程 见附件。')

# ─── 七、工程规范与可展示成果 ───
heading('七、工程规范与可展示成果', 1)

make_table(
    ['项目', '内容'],
    [
        ['源代码规模', '200+源文件，0 Error / 0 Warning编译通过'],
        ['编码规范', '全量UTF-8统一编码，模块化目录结构'],
        ['迭代记录', '48轮编译日志，完整追踪修改历史'],
        ['可展示资料', 'PCB设计文件、电控架构图、路径规划图、动作状态机图、同步轨迹对比图'],
        ['扩展能力', '新增挂位、护理动作、传感器仅需修改参数宏和动作表，不破坏主体架构'],
    ]
)

# Save
out_path = r'C:\Users\ECHO\Desktop\家居\申报书_电控\电控系统_申报内容.docx'
doc.save(out_path)
print(f'Saved: {out_path}')
