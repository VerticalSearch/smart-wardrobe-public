from docx import Document
from docx.shared import Pt, Inches, Cm, Emu, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import os

doc = Document()

# ── Page Setup (A4) ──
section = doc.sections[0]
section.page_width  = Cm(21.0)
section.page_height = Cm(29.7)
section.left_margin   = Cm(2.1)
section.right_margin  = Cm(2.1)
section.top_margin    = Cm(2.0)
section.bottom_margin = Cm(1.9)

# ── Default Style ──
style = doc.styles['Normal']
style.font.name = '宋体'
style.font.size = Pt(10.5)  # 五号
style.font.color.rgb = RGBColor(0,0,0)
style.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
style.paragraph_format.line_spacing = 1.5
style.paragraph_format.space_after = Pt(0)
style.paragraph_format.first_line_indent = Cm(0.74)  # 两字符

# ── Heading Styles ──
for lvl, (name, font_name, size, color) in enumerate([
    ('Heading 1', '黑体', 16, RGBColor(0,0,0)),
    ('Heading 2', '黑体', 14, RGBColor(0,0,0)),
    ('Heading 3', '黑体', 12, RGBColor(0,0,0)),
], 1):
    hs = doc.styles[name]
    hs.font.name = font_name
    hs.font.size = Pt(size)
    hs.font.bold = True
    hs.font.color.rgb = color
    hs.element.rPr.rFonts.set(qn('w:eastAsia'), font_name)
    hs.paragraph_format.first_line_indent = Cm(0)
    hs.paragraph_format.space_before = Pt(12 if lvl==1 else 8)
    hs.paragraph_format.space_after = Pt(6)

# ── Helpers ──
def h(text, level=1):
    return doc.add_heading(text, level=level)

def p(text, bold=False, indent=True):
    par = doc.add_paragraph()
    par.paragraph_format.line_spacing = 1.5
    if indent:
        par.paragraph_format.first_line_indent = Cm(0.74)
    else:
        par.paragraph_format.first_line_indent = Cm(0)
    run = par.add_run(text)
    run.font.name = '宋体'
    run.font.size = Pt(10.5)
    run.bold = bold
    run.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    return par

def p_center(text, size=10.5, bold=False):
    par = doc.add_paragraph()
    par.alignment = WD_ALIGN_PARAGRAPH.CENTER
    par.paragraph_format.first_line_indent = Cm(0)
    run = par.add_run(text)
    run.font.name = '宋体'
    run.font.size = Pt(size)
    run.bold = bold
    run.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    return par

def image_placeholder(caption):
    """插图占位符"""
    par = doc.add_paragraph()
    par.alignment = WD_ALIGN_PARAGRAPH.CENTER
    par.paragraph_format.first_line_indent = Cm(0)
    par.paragraph_format.space_before = Pt(6)
    run = par.add_run(f'【{caption}】')
    run.font.name = '宋体'
    run.font.size = Pt(9)
    run.font.color.rgb = RGBColor(0x80,0x80,0x80)
    run.italic = True
    run.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    return par

def set_cell_shading(cell, color):
    shading = OxmlElement('w:shd')
    shading.set(qn('w:fill'), color)
    shading.set(qn('w:val'), 'clear')
    cell._tc.get_or_add_tcPr().append(shading)

def table(headers, rows, col_widths=None):
    tbl = doc.add_table(rows=1+len(rows), cols=len(headers))
    tbl.style = 'Table Grid'
    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    # Header row
    for i, hdr in enumerate(headers):
        cell = tbl.rows[0].cells[i]
        cell.text = ''
        p_cell = cell.paragraphs[0]
        p_cell.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p_cell.paragraph_format.first_line_indent = Cm(0)
        p_cell.paragraph_format.space_before = Pt(2)
        p_cell.paragraph_format.space_after = Pt(2)
        run = p_cell.add_run(hdr)
        run.font.name = '黑体'
        run.font.size = Pt(9)
        run.bold = True
        run.font.color.rgb = RGBColor(0xFF,0xFF,0xFF)
        run.element.rPr.rFonts.set(qn('w:eastAsia'), '黑体')
        set_cell_shading(cell, '1A237E')
    # Data rows
    for r, row in enumerate(rows):
        for c, val in enumerate(row):
            cell = tbl.rows[r+1].cells[c]
            cell.text = ''
            p_cell = cell.paragraphs[0]
            p_cell.paragraph_format.first_line_indent = Cm(0)
            p_cell.paragraph_format.space_before = Pt(1)
            p_cell.paragraph_format.space_after = Pt(1)
            run = p_cell.add_run(val)
            run.font.name = '宋体'
            run.font.size = Pt(9)
            run.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    # Col widths
    if col_widths:
        for row in tbl.rows:
            for i, w in enumerate(col_widths):
                row.cells[i].width = Cm(w)
    doc.add_paragraph()
    return tbl


# ═══════════════════════════════════════════
# TITLE
# ═══════════════════════════════════════════
p_center('智能衣橱电控系统技术方案', size=16, bold=True)
p_center('（申报书技术方案 / 电控系统 / 硬件控制栏目）', size=10.5)
doc.add_paragraph()

# ═══════════════════════════════════════════
# 一、总体架构与分层设计
# ═══════════════════════════════════════════
h('一、总体架构与分层设计', 1)

p('本项目电控系统负责将上层取衣、放衣、识别、清理和护理任务转化为安全、连续、可反馈的机械动作。系统采用"意图输入层 + 调度控制层 + 运动执行层 + 状态反馈层"四层架构，将人机交互、任务调度和底层运动三者解耦。')

p('在该架构下，树莓派触摸屏网页、App或云端后端只表达任务意图，不直接输出电机脉冲；主板1（STM32F407）负责接收任务、汇聚传感器状态、判断执行条件并下发动作命令；主板2（STM32F407）负责5轴步进电机、夹爪舵机、伸缩轴、B区毛刷、C区热风和通风风扇等执行机构控制；激光测距、限位开关和到位信号组成反馈层，提供执行前校验和动作完成判断依据。')

table(
    ['层级', '组成', '职责'],
    [
        ['意图输入层', '触摸屏网页 / App / Web / 云端后端', '产生取衣、放衣、拍照、护理等任务意图，不直接控制硬件'],
        ['调度控制层', 'STM32F407主板1', '任务调度、状态判定、流程编排、异常拦截、板间命令转发'],
        ['运动执行层', 'STM32F407主板2', '5轴步进电机、夹爪舵机、伸缩机构、毛刷和护理组件执行'],
        ['状态反馈层', '激光测距 / 限位开关 / 按键 / 到位信号', '执行前校验、动作完成判断、异常保护依据'],
    ]
)

image_placeholder('图1  智能衣橱电控系统四层架构图')

# ═══════════════════════════════════════════
# 二、硬件设计与接口分配
# ═══════════════════════════════════════════
h('二、硬件设计与接口分配', 1)

h('2.1 双板选型与分工', 2)
p('两板均采用STM32F407主控芯片，物理独立，通过UART1 DMA高速通信。主板1为协调调度板，承载全部对外通信；主板2为运动执行板，专注电机驱动与传感器采集。两板职责边界清晰：主板1不直接操作电机，主板2不处理网络和云端逻辑。')

table(
    ['对比项', '主板1（协调调度板）', '主板2（运动执行板）'],
    [
        ['主控芯片', 'STM32F407', 'STM32F407'],
        ['核心职责', '流程编排、外设通信、任务解析', '电机驱动、传感器采集、动作执行'],
        ['调试手段', 'OLED显示 + USMART命令行', 'OLED显示 + 4键本地调试'],
        ['定时调度', 'TIM7 1ms中断，10ms控制周期', 'TIM7 1ms中断，10ms控制周期'],
    ]
)

h('2.2 主板1 外设接口分配', 2)
p('主板1承载全部对外通信，6路UART各司其职，全部配置为115200 bps：')

table(
    ['串口', '连接对象', '功能说明', '通信方式'],
    [
        ['UART1', '主板2', '板间命令帧传输', 'DMA模式，自定义帧协议'],
        ['UART2', 'ESP32-S3', '预留扩展接口', '初始化+基础发送'],
        ['UART3', 'K230摄像头', '拍照指令 / 配网参数 / 识别结果回传', '二进制帧 + ASCII文本'],
        ['UART4', '激光测距模块', '衣物位置距离采集', '中断解析 + 20ms软任务'],
        ['UART5', '步进电机（单路）', '辅助电机控制', 'TX=PC12, RX=PD2'],
        ['UART6', '串口屏（备用）', '调试显示 / 备用配网', 'TJC 800x480'],
    ]
)

h('2.3 主板2 执行机构分配', 2)
p('主板2专注运动控制，5路步进电机各占独立串口，实现多轴协同：')

table(
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

image_placeholder('图2  硬件接口拓扑图')

# ═══════════════════════════════════════════
# 三、通信协议与数据流
# ═══════════════════════════════════════════
h('三、通信协议与数据流', 1)

h('3.1 板间通信协议', 2)
p('主板1与主板2之间采用自定义帧协议，在UART1 DMA模式下高速传输。帧结构为：帧头（0xFF）+ 命令码（1字节）+ 参数1（2字节）+ 参数2（2字节）+ 校验和（1字节）。校验和不通过则丢弃该帧，保证指令可靠性。')

table(
    ['命令码', '功能', '参数说明'],
    [
        ['CMD_STEPPER_ENABLE', '电机使能/失能', '参数1=通道(1~5), 参数2=使能(0/1)'],
        ['CMD_STEPPER_MOVE_ANGLE', '指定角度运动', '参数1=通道(1~5), 参数2=角度(度)'],
        ['CMD_GET_SENSOR', '查询传感器状态', '返回当前传感器数据'],
        ['CMD_EMERGENCY', '紧急停止', '最高优先级，立即停止所有电机'],
    ]
)

h('3.2 主板1与K230视觉模块交互协议', 2)
p('K230通过UART3连接主板1，协议分两种模式：二进制帧用于拍照触发和状态查询；ASCII文本用于Wi-Fi配网参数（SSID和密码）的明文转发。K230拍摄照片后自行通过WiFi上传云端进行AI识别，识别结果（style_id）通过UART3回传给主板1，主板1据此匹配取衣任务。')

h('3.3 激光测距数据流', 2)
p('UART4接收激光模块原始数据帧，在TIM7中断服务程序中解析帧头和校验，置位帧就绪标志。每20ms软任务周期内执行数据处理，输出距离值（mm），供控制层判定衣物所在区间（C1~C4），为路径规划提供定位依据。')

image_placeholder('图3  电控系统数据流示意图')

# ═══════════════════════════════════════════
# 四、控制策略
# ═══════════════════════════════════════════
h('四、控制策略', 1)

h('4.1 任务调度机制', 2)
p('主板1作为调度核心，通过HTTP轮询云端任务队列（GET /api/tasks/pending，间隔1~3s），获取待执行任务。任务处理遵循FIFO顺序：先解析任务类型（取衣/放衣/护理），再校验执行条件（目标挂位状态、机构空闲、传感器正常），条件满足后编排执行流程并下发动作序列。执行完成后回写云端（POST /api/tasks/{id}/complete），同步衣物状态。该机制使上层只负责"要做什么"，主板1负责"能不能做、按什么顺序做"，主板2负责"具体怎么动"。')

h('4.2 表驱动动作状态机', 2)
p('主板2采用表驱动状态机，将复杂任务拆解为标准动作单元串联执行。每一步动作完成（到位反馈或延时完成）后再进入下一状态，使流程具备明确顺序、可等待、可中断、可扩展的特点。夹爪、舵机、伸缩轴、毛刷、热风机和通风风扇均被纳入统一动作链路，新增动作只需扩展状态表，无需改动底层驱动。')

table(
    ['动作单元', '控制对象', '触发条件', '完成条件'],
    [
        ['移动到目标点', 'X/Y/Z轴、伸缩轴', '上一步完成', '到位信号'],
        ['夹取', '夹爪舵机', '到达目标位置', '动作延时完成'],
        ['释放', '夹爪舵机', '到达释放位置', '动作延时完成'],
        ['姿态调整', 'Z轴 + 舵机', '移动到安全高度后', '到位信号'],
        ['清理动作', 'B区毛刷电机', '夹持衣物到位', '毛刷完成循环'],
        ['护理动作', '热风/负离子/风扇', '衣物挂入C区', '定时完成'],
    ]
)

h('4.3 点位表路径规划与防干涉设计', 2)
p('衣橱内部空间紧凑，夹爪、衣物、挂位和机构组件之间存在潜在干涉风险。系统将A区（取衣口）、B区（清理区）、B0（识别拍照点）、C区（存储护理区）抽象为功能点位表，根据任务类型自动选择运动路径。')

p('路径规划遵循"安全高度优先"原则：先抬升Z轴到安全高度，再进行X/Y平面移动，最后下降进入目标点。进入A区和C区等狭窄区域前，先经过前置偏移点和中转点，避免低高度横移、直接斜插和未达到安全姿态前的翻转动作。该设计可概括为"电子围栏式防干涉路径控制"。')

table(
    ['路径控制策略', '含义', '作用'],
    [
        ['功能点位表', '将A区、B区、B0、C区统一建模', '使运动有目标点和任务语义'],
        ['安全高度优先', '先抬Z轴，再X/Y移动，最后下降', '降低夹爪与柜体的干涉风险'],
        ['前置点进入', '进入狭窄区域前先经过偏移位置', '避免直接切入，提高可控性'],
        ['中转点过渡', '关键区域间设置过渡路径', '减少斜插和姿态未调整的危险运动'],
    ]
)

h('4.4 X/Y双轴同步到点控制', 2)
p('X轴和Y轴共同决定夹爪在衣橱内的平面运动轨迹。若两轴以相近速度独立运动，位移较短的轴会提前到位停止，实际轨迹由平滑斜线变为折线路径，容易造成运动方向突变、衣物姿态晃动，并在狭小空间内增加贴近柜体边界的风险。')

p('系统引入X/Y双轴同步到点策略：先计算两轴目标位移差异，对短轴降低运行速度或延后启动时序，使两轴尽量同步到达目标点。结合到位反馈和超时判断确认运动完成，形成连续、可预测的平面运动轨迹，提升夹持转运过程中衣物的姿态稳定性。')

table(
    ['对比项', '不同步控制', '同步到点控制'],
    [
        ['轨迹形态', '折线，方向突变', '平滑连续'],
        ['空间安全', '折线路径靠近柜体边界', '轨迹可预测，利于配合防干涉'],
        ['衣物姿态', '夹持转运连续性较差', '夹爪和衣物姿态更稳定'],
    ]
)

h('4.5 安全保护机制', 2)
p('安全保护贯穿任务执行全过程，分三个层面：')

table(
    ['保护层面', '措施', '触发条件与响应'],
    [
        ['执行前校验', '任务合法性检查、目标挂位状态确认、机构安全状态验证、传感器在线检测', '条件不满足则暂缓执行'],
        ['执行中监测', '电机到位反馈与超时判断、限位开关作软件安全边界、激光测距实时距离监控', '超时视为故障，进入异常处理'],
        ['异常处理', '急停（最高优先级）、动作超时→回零待命、限位触发→反向脱离、通信超时→待机', '立即停止运动输出，避免机构继续危险动作'],
    ]
)

p('主板2硬件层面支持加速度限幅宏BOARD2_ACCEL_U8(x)，将加速度参数限制在0~255范围内，确保协议字段不溢出导致异常运动。')

image_placeholder('图4  控制策略综合示意图（含安全保护层级）')

# ═══════════════════════════════════════════
# 五、典型任务闭环
# ═══════════════════════════════════════════
h('五、典型任务闭环', 1)

p('电控系统围绕放衣、取衣、拍照识别、清理和护理等典型任务建立闭环流程。每类任务均遵循"主板1解析判定 → 主板2按动作表执行 → 完成/异常状态反馈"的统一范式，将用户点击转化为可验证、可反馈的机械执行结果。')

h('5.1 取衣闭环', 2)
p('用户通过App或触摸屏点击取衣 → 云端生成fetch任务（含item_ids） → 主板1轮询获取任务 → 调度K230拍照确认衣物位置 → 云端AI识别返回style_id → 主板1判定路径（C区目标挂位→前置点→B中转→A区出口） → 主板2按序执行（抬Z安全高度→X/Y同步移动→降Z→夹取→抬Z→转运→A区释放，每步到位反馈） → 取衣完成 → 回写云端 → 更新衣物状态为"已取出"。')

h('5.2 放衣/入库闭环', 2)
p('用户放入衣物 → K230拍照入库 → 云端生成store-new任务 → 主板1调度（A区夹取→B0拍照识别→C区存放） → 主板2执行完整点位序列 → 完成回写 → 衣物进入目标挂位，状态可查询。')

h('5.3 清理与护理闭环', 2)
p('由云端care任务触发，主板1根据任务类型调度主板2执行对应动作：B区清理（转运至B区→毛刷接近→清理动作循环→完成回写）；C区护理（转运至C区→热风/负离子/通风组件执行→定时完成→状态反馈）。')

image_placeholder('图5  智能衣橱典型任务闭环流程图')

# ═══════════════════════════════════════════
h('六、工程规范与可展示成果', 1)

p('本项目电控部分的工程化程度较高：源代码规模200+文件，经Keil编译验证为0 Error、0 Warning；全量源码统一为UTF-8编码，便于跨工具协作；累计48轮编译日志，完整追踪修改历史。可展示资料包括PCB设计文件、电控架构图、路径规划图、动作状态机图、同步轨迹对比图和安全保护层级图。新增挂位、护理动作或传感器仅需修改参数宏和动作表，不破坏主体电控架构。')

# ── Save ──
out_path = r'C:\Users\ECHO\Desktop\家居\申报书_电控\电控系统_申报书正式版.docx'
doc.save(out_path)
print(f'Saved: {out_path}')
