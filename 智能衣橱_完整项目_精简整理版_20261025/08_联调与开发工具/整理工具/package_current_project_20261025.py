"""Build an auditable, current-project-only copy. Never move/delete source files."""
from __future__ import annotations

import argparse
import ast
import csv
import hashlib
import html
import json
import os
from pathlib import Path
import re
import shutil
import sys
import xml.etree.ElementTree as ET
import zipfile
from collections import defaultdict
from datetime import datetime, timezone, timedelta

ROOT = Path(__file__).resolve().parents[1]
NAME = '智能衣橱_完整项目_精简整理版_20261025'
OUT = ROOT / NAME
AUDIT = OUT / '09_整理校验'
HANDOFF = '云端任务与硬件执行_后端排查交付包_20261023'
TZ = timezone(timedelta(hours=8))
SECTIONS = {
    '00_项目说明': '先了解项目组成、入口、完整性边界和原始改动记录。',
    '01_系统方案': '智能衣橱方案、系统流程、架构图与功能说明。',
    '02_树莓派网关与网页': '现场触摸屏网页、Python 网关、摄像头、串口桥、库存管理和测试。',
    '03_STM32双主板': '主板1负责协调与传感，主板2负责步进电机、伸缩杆和舵机动作。',
    '04_PCB与接线': '两份立创 EDA 原生工程、原工程随附的机构草图与网表入口。',
    '05_护理与器件资料': '护理方案、器件清单、HX711 官方资料及模块原理图。',
    '06_演示申报与专利': '本项目答辩 PPT、申报书、专利说明、海报与绘图素材。',
    '07_安卓客户端': '本地现有安装包及从该安装包读取的内嵌网页；非原生完整构建工程。',
    '08_联调与开发工具': '接口样例、近期排查说明、只读采集、离线测试及文档生成脚本。',
    '09_整理校验': '文件来源、SHA-256、未纳入内容、依赖检查和本轮验证结果。',
}


def sha(p: Path) -> str:
    with p.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def write(rel: str, text: str) -> None:
    p = OUT / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text.strip() + '\n', encoding='utf-8')


def write_json(p: Path, value) -> None:
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def write_csv(p: Path, rows: list[dict], fields: list[str] | None = None) -> None:
    with p.open('w', encoding='utf-8-sig', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fields or list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def selection() -> list[dict]:
    items = []

    def add(src, dst, purpose, transform='none', member=''):
        src = str(src).replace('\\', '/')
        if not (ROOT / src).is_file():
            raise FileNotFoundError(src)
        items.append(dict(source=src, package_path=str(dst).replace('\\', '/'),
                          purpose=purpose, transformation=transform, archive_member=member))

    def tree(src, dst, purpose, predicate=lambda p: True):
        base = ROOT / src
        for f in sorted(base.rglob('*')):
            if f.is_file() and predicate(f.relative_to(base)):
                add(f.relative_to(ROOT), Path(dst) / f.relative_to(base), purpose)

    add('00_项目导航/改动记录.md', '00_项目说明/原始改动记录.md', '原始项目历史与本轮整理记录')
    add('AGENTS.md', '00_项目说明/原项目协作规则.md', '原目录协作规则备查；整理包使用根目录的新规则')
    for n in ('README_项目总览.md', '家居文件位置索引.md', '调试快速定位.md'):
        add('00_项目导航/' + n, '00_项目说明/原导航_仅作历史参考/' + n, '历史导航，可能包含旧路径和K230口径')

    tree('01_智能衣橱方案', '01_系统方案/01_智能衣橱方案', '现有系统方案与插图',
         lambda p: p.parts[0] not in {'交付压缩包', '智能衣橱申报材料_WPS资料包'}
         and p.name != '云端后端到K230技术链路.md')
    add('智能衣橱技术文档.md', '01_系统方案/智能衣橱技术文档.md', '项目技术说明')

    tree('raspberry_pi_gateway', '02_树莓派网关与网页/raspberry_pi_gateway', '当前网关完整源码、测试和部署文件',
         lambda p: not any(x in {'.pytest_cache', '__pycache__', '.venv', 'data', 'captures'} for x in p.parts))
    add('02_界面原型/cloud_display.html', '02_树莓派网关与网页/02_界面原型/cloud_display.html',
        '网关默认路径及原测试依赖的同级网页副本，不作去重')
    for src in ('artifacts/pi_deploy/start_wardrobe_gateway.sh',
                'artifacts/pi_deploy/wardrobe-one-click-start.desktop',
                'outputs/pi_autostart/labwc_autostart'):
        add(src, '02_树莓派网关与网页/现有桌面启动模板/' + Path(src).name,
            '现有桌面启动配套文件，现场路径与依赖需部署前核对；本次不执行')

    source_exts = {'.c', '.h', '.s', '.uvprojx', '.uvoptx', '.ini', '.sct', '.dbgconf',
                   '.scvd', '.md', '.txt', '.json', '.tel', '.lib', '.a'}
    for board in ('主板1', '主板2'):
        base = ROOT / '代码/4.11' / board
        for f in sorted(base.rglob('*')):
            if not f.is_file():
                continue
            rel = f.relative_to(base)
            if f.suffix.lower() in source_exts:
                add(f.relative_to(ROOT), Path('03_STM32双主板') / board / rel, 'Keil源工程、厂商库、驱动、参数和网表')
            elif rel.as_posix() in {'OBJ/Radar_2.hex', 'OBJ/Radar_2.map', 'OBJ/Radar_2.4G'}:
                add(f.relative_to(ROOT), Path('03_STM32双主板/既有构建产物_未重新编译') / board / f.name,
                    '仅版本对照，未重编译且未确认现场烧录')

    for name in ('家居.eprj2', '家居串口拓展板.eprj2'):
        add('PCB/' + name, '04_PCB与接线/立创EDA工程/' + name, '当前本地原生EDA工程')
    sketch_names = {
        '1368226197E7FB2CFFE1F3E7E85CBF6C.jpg': '01_主板1原始功能草图.jpg',
        '88BCE125F11D2D21F0766414BF99235B.jpg': '02_主板2原始动作草图.jpg',
        'D7E01A677FEFAEBA87EFAE04671A962F.jpg': '03_原始XYZ机构与串口分配草图.jpg',
    }
    for old, new in sketch_names.items():
        add('代码/4.11/' + old, '04_PCB与接线/原始机构草图_设计参考/' + new, '源工程随附早期手绘设计，不作为当前接线依据')

    for f in sorted((ROOT / '护衣系统').iterdir()):
        if f.is_file() and f.stat().st_size and f.suffix.lower() in {'.md', '.jpg'}:
            add(f.relative_to(ROOT), '05_护理与器件资料/护理方案/' + f.name, '护理与称重设计资料')
    tree('器件', '05_护理与器件资料/原器件文件', '原有器件表与素材')
    for name in ('DHT22温湿度传感器使用指南.md', 'HC-SR505人体感应传感器使用指南.md',
                 'STM32标准库开发说明.md', '天气数据获取方案.md', '智能衣柜器件选型清单.md'):
        add('文书/参考资料/' + name, '05_护理与器件资料/器件与接口说明/' + name, '本项目相关器件参考；是否使用以现行源码为准')
    vendor = ROOT / '护衣系统/称重传感器 大陆电子'
    for name in ('hx711官方资料(海芯).pdf', 'HX711模块原理图.pdf', '5Kg电子秤开发必读教程.pdf'):
        found = list(vendor.rglob(name))
        assert len(found) == 1, name
        add(found[0].relative_to(ROOT), '05_护理与器件资料/HX711资料/' + name, '称重芯片资料、模块电路及标定开发参考')

    tree('申报书_电控', '06_演示申报与专利/申报书_电控', '本项目电控申报书与配图')
    tree('ppt_diagrams', '06_演示申报与专利/ppt_diagrams', '答辩架构与流程图素材')
    for f in sorted(ROOT.glob('智能衣橱答辩*.pptx')):
        add(f.name, '06_演示申报与专利/答辩PPT/' + f.name, '本项目答辩演示稿')
    for f in sorted((ROOT / 'outputs/019e48b5-78f3-7881-8d87-87f4884ab0c8').rglob('*.pptx')):
        add(f.relative_to(ROOT), '06_演示申报与专利/答辩PPT/' + f.name, '散落在输出目录的本项目答辩演示稿')
    for f in sorted(ROOT.glob('基于AI Agent*.docx')):
        add(f.name, '06_演示申报与专利/专利说明/' + f.name, '本项目专利核心思想文档')
    for f in sorted(ROOT.glob('豆包_*')):
        if f.is_file():
            add(f.name, '06_演示申报与专利/文案与生成提示词/' + f.name, '本项目演示文案与生成提示词')
    for name in ('smart_wardrobe_showcase_poster.png', 'smart_wardrobe_showcase_poster_v2.png'):
        add('outputs/' + name, '06_演示申报与专利/展示海报/' + name, '本项目已有展示海报')
    add('artifacts/电控配图_树莓派双板架构.png', '06_演示申报与专利/系统配图/电控配图_树莓派双板架构.png', '现有树莓派双板架构图')
    add('outputs/lo_direct/application_book.pdf', '06_演示申报与专利/申报书PDF/既有申报书导出.pdf', '已有申报书PDF，未重新渲染')

    apk = 'artifacts/apk_build/H594B5CF3_20260607_care_schedule_installable_v2v3.apk'
    add(apk, '07_安卓客户端/既有安装包/H594B5CF3_20260607_care_schedule_installable_v2v3.apk',
        '本地现有2026-06-07安装包，不代表现场版本')
    for name in ('index.html', 'manifest.json'):
        add(apk, '07_安卓客户端/安装包内网页/www/' + name, '从同一APK原样读取的HBuilder网页资源',
            'extract_zip_member', 'assets/apps/H594B5CF3/www/' + name)

    for name in ('01_接口与执行链路.md', '02_伸缩杆伸出后停止_排查手册.md', '04_关键代码索引.md'):
        add(HANDOFF + '/' + name, '08_联调与开发工具/后端排查说明_20261023/' + name,
            '近期排查结论；文内旧包相对路径需按本包导航对照')
    tree(HANDOFF + '/接口样例', '08_联调与开发工具/接口样例', '仅作格式参考的接口样例和时间线表')
    add(HANDOFF + '/排查工具/collect_readonly.py', '08_联调与开发工具/collect_readonly.py', '只读HTTP采集工具；本次不运行')
    add(HANDOFF + '/排查工具/run_offline_suite.py', '08_联调与开发工具/run_offline_suite.py',
        '适配整理包目录的离线测试入口', 'adapt_offline_root')
    for f in sorted(ROOT.glob('*.py')):
        add(f.name, '08_联调与开发工具/文档生成脚本/' + f.name, '原有文档生成脚本；保留原文，使用前核对旧绝对路径和素材路径')
    add('deck.json', '08_联调与开发工具/文档生成脚本/deck.json', '原有演示稿生成数据')
    add('tools/package_current_project_20261025.py', '08_联调与开发工具/整理工具/package_current_project_20261025.py',
        '本轮整理脚本归档；须在原目录tools下运行')

    seen = set()
    for item in items:
        key = item['package_path'].casefold()
        assert key not in seen, key
        seen.add(key)
    return items


def source_inventory(items: list[dict]) -> dict:
    included = {x['source'] for x in items}
    grouped = defaultdict(lambda: {'files': 0, 'bytes': 0, 'included_sources': 0})
    total_files = total_bytes = 0
    for d, dirs, files in os.walk(ROOT, followlinks=False):
        dirs[:] = [n for n in dirs if not (Path(d) / n).is_junction()
                   and not (Path(d) / n).is_symlink() and n != NAME]
        for name in files:
            p = Path(d) / name
            rel = p.relative_to(ROOT).as_posix()
            if rel.startswith(NAME) or p.is_symlink():
                continue
            size = p.stat().st_size
            top = rel.split('/')[0]
            grouped[top]['files'] += 1
            grouped[top]['bytes'] += size
            grouped[top]['included_sources'] += int(rel in included)
            total_files += 1
            total_bytes += size
    return dict(source_files=total_files, source_bytes=total_bytes, groups=dict(grouped))


def copy_one(item: dict) -> dict:
    src = ROOT / item['source']
    dst = OUT / item['package_path']
    assert dst.resolve().is_relative_to(OUT.resolve())
    dst.parent.mkdir(parents=True, exist_ok=True)
    original_hash = sha(src)
    if item['transformation'] == 'extract_zip_member':
        with zipfile.ZipFile(src) as z:
            data = z.read(item['archive_member'])
        dst.write_bytes(data)
    elif item['transformation'] == 'adapt_offline_root':
        data = src.read_text(encoding='utf-8')
        old = 'ROOT=Path(__file__).resolve().parents[1]'
        assert data.count(old) == 1
        data = data.replace(old, old + "/'02_树莓派网关与网页'")
        dst.write_text(data, encoding='utf-8')
    else:
        shutil.copy2(src, dst)
    package_hash = sha(dst)
    assert original_hash == sha(src), 'Source changed while copying: ' + str(src)
    if item['transformation'] == 'none':
        assert package_hash == original_hash
    return dict(**item, source_bytes=src.stat().st_size, package_bytes=dst.stat().st_size,
                source_sha256=original_hash, package_sha256=package_hash)


def write_guides(inventory: dict):
    rows = '\n'.join(f'| [{s}]({s}/README_本部分说明.md) | {desc} |' for s, desc in SECTIONS.items())
    write('README_先看这里.md', f'''# 智能衣橱完整项目 · 精简整理版

整理日期：2026-10-25（北京时间）。来源：本机 `D:\\Desktop\\家居` 的当前文件。

本包集中保留当前智能衣橱的本地工程、原生PCB、方案、演示和联调资料。用户选择“只放当前项目，旧资料留在原目录”；原文件没有移动或删除。

双击 `打开项目导航.html` 可按模块查找文件。要继续开发，优先使用下表的网关和双主板目录。

| 部分 | 这是什么 |
|---|---|
{rows}

## 最常用入口

- 网关主程序：`02_树莓派网关与网页/raspberry_pi_gateway/src/wardrobe_gateway/app.py`
- 触摸屏页面：`02_树莓派网关与网页/raspberry_pi_gateway/static/cloud_display.html`
- 主板1 Keil工程：`03_STM32双主板/主板1/USER/MPU9250.uvprojx`
- 主板2 Keil工程：`03_STM32双主板/主板2/USER/MPU9250.uvprojx`
- PCB：`04_PCB与接线/立创EDA工程/家居.eprj2` 和 `家居串口拓展板.eprj2`
- 交付检查：`09_整理校验/本轮检查结果.json`；逐文件来源见同目录 `文件来源清单.csv`。

## 项目组成与完整性边界

网页或安卓客户端 → 树莓派网关 / 云端接口 → STM32主板1 → STM32主板2 → 执行机构；状态沿相关链路返回。具体HTTP/MQTT入口见08中的接口说明。

“完整”指本目录中已具备的当前项目材料已按模块集中。**本地没有找到独立云服务器的完整源码、数据库迁移和部署工程，也没有找到完整的安卓原生构建工程**。07含现有APK与其内嵌网页，02含云接口代理；这些不能代替缺失的云服务或安卓原生工程。未发现可明确列为现行交付件的完整机械CAD工程。

本次只整理本地文件并做离线检查；未重新编译Keil、未部署树莓派、未连接DAP/串口、未烧录、未复位、未触发机械动作。现有HEX/MAP和APK的状态已在各目录标注。

护理页HX711仍是去皮后的counts，不是kg。旧文档与草图保留其原文，现行硬件定义以当前源码与现场核对为准。近期排查中的长命令ID等问题没有因本次整理而修复。
''')
    notes = {
        '00_项目说明': '''`项目组成与入口.md` 是当前整理版导航。`原始改动记录.md` 保留项目过程和本轮记录。

`原导航_仅作历史参考` 与 `原项目协作规则.md` 是原文件原样副本，包含旧C盘路径、K230路线或旧目录，不能直接当作本整理包的实时目录与设备状态。''',
        '01_系统方案': '''`01_智能衣橱方案/智能衣橱系统总览与执行流程.md` 是详细方案入口；`智能衣橱技术文档.md` 是技术说明。

保留现有方案、摘要、申报材料和images。旧压缩交付包、WPS重复资料及K230专用链路说明留在原目录。部分方案形成于2026年5月，与当前源码不一致的内容需要按源码和现场校核。''',
        '02_树莓派网关与网页': '''`raspberry_pi_gateway` 是完整本地网关工程。`src/wardrobe_gateway` 是Python后端，`static` 是页面，`tests` 是原测试，`scripts` 和 `systemd` 是安装/服务模板。

`02_界面原型/cloud_display.html` 是网关默认配置和测试依赖的同级副本。这两份网页必须同步，不能为了去重删掉其中一份。网关目录内部结构保持不变。

树莓派部署入口仍为网关README和 `scripts/install_pi.sh`，其默认用户与路径是 `pi` 和 `/home/pi/raspberry_pi_gateway`。部署时核对现场用户、路径、串口和环境变量。当前运行页面使用 `/cloud`；直接双击HTML只能查看静态文件，不能证明设备链路已连接。

`现有桌面启动模板` 保存本机已有的启动shell、桌面快捷方式和labwc自启动条目。shell对应 `/home/pi/raspberry_pi_gateway`，快捷方式和自启动条目调用 `/home/pi/start_wardrobe_gateway.sh`；涉及Chromium、ffmpeg与v4l2-ctl。它们是部署参考，未与本次现场设备核对、未执行。

本包未包含实时库存data、相机captures、虚拟环境或凭据；它不是现场状态备份。独立远端云服务需后端人员另行提供。''',
        '03_STM32双主板': '''- `主板1`：协调板。串口桥接、板间协议、HX711/温湿度/限位等传感输入及护理控制；入口为 `USER/MPU9250.uvprojx`。
- `主板2`：执行板。步进电机、XYZ/伸缩杆、舵机、动作队列和目标C点；入口同样为 `USER/MPU9250.uvprojx`。
- `CORE` 为启动代码，`FWLIB` 为STM32标准库，`SYSTEM` 为基础系统，`HARDWARE` 为业务驱动，`USER` 为主程序及Keil配置。
- 各板根目录 `.tel` 为源工程自带网表；现行EDA另见04。

工程内部文件层级保留。删除范围仅发生在整理副本的选择阶段：不复制 `.o/.crf/.d`、列表、IDE窗口状态和重复构建日志。没有删除原目录文件。

`既有构建产物_未重新编译` 保留原OBJ中的HEX/MAP/ELF用于对照，不能据此判断现场烧录版本。Keil项目原输出名 `Radar_2.4G` 沿用旧名称，不能仅凭名字把这些工程当成另一个雷达项目。

本次验证源文件引用和目录移动后的相对路径，未调用Keil重新构建、未烧录、未复位。''',
        '04_PCB与接线': '''`立创EDA工程/家居.eprj2` 与 `家居串口拓展板.eprj2` 是当前本地两个原生EDA工程，均原样复制。

主板网表位于 `../03_STM32双主板/主板1/主板1.tel`、`../03_STM32双主板/主板2/主板2.tel`。`原始机构草图_设计参考` 是原4.11目录随附的三张手写设计图，已改成能识别用途的文件名；它们不是已核对的当前接线图。

树莓派CH340控制链路接线说明见02网关README。PCB备份和旧打板包留在原目录，本轮没有执行ERC/DRC或生成新的制造文件。''',
        '05_护理与器件资料': '''`护理方案`：护理与称重设计文档及示意图；`HX711资料`：芯片官方资料、模块原理图和开发教程；`器件与接口说明`：传感器、天气数据、STM32标准库及选型参考；`原器件文件`：现有表格与素材。

现行HX711驱动在 `../03_STM32双主板/主板1/HARDWARE/HX711`，显示值仍是去皮counts，需已知重量标定后才能使用kg。资料中提到的器件不等于全部已装机。旧51单片机电子秤示例、重复压缩包与空白草案留在原目录。''',
        '06_演示申报与专利': '''`答辩PPT` 汇总根目录和outputs中本项目的9份可编辑演示稿，保留版本名称供选择；`申报书_电控` 是申报正文、配图及提示词；`专利说明` 是两份核心思想文档。

`展示海报`、`系统配图`、`ppt_diagrams` 为本项目已有图片素材；`申报书PDF` 是已有PDF导出；`文案与生成提示词` 为现有文案。文件均按原始字节保留，本轮未重排版、未重新渲染。

其他团队的比赛范例、视频、参考软著等不放入本包。生成脚本集中在08，相关旧路径使用前需调整。''',
        '07_安卓客户端': '''`既有安装包/H594B5CF3_20260607_care_schedule_installable_v2v3.apk` 是本地现有的2026-06-07构建，未重新打包或安装，不能据此认定现场版本。

`安装包内网页/www/index.html` 和 `manifest.json` 从上述同一APK的 `assets/apps/H594B5CF3/www/` 原样读取。APK内页面与树莓派8月网页是不同时间的版本，修改02不会自动更新此APK。

没有找到完整Android原生工程；没有附带签名私钥。需要发布新APK时，应从原开发者获取原生/HBuilder工程和受控签名流程。''',
        '08_联调与开发工具': '''`接口样例`：格式示例及现场时间线模板；`后端排查说明_20261023`：最近一次排查的接口、症状分流和代码索引。

这些排查文档保留原文：其中 `raspberry_pi_gateway/...` 对应本包 `02_树莓派网关与网页/raspberry_pi_gateway/...`，`代码/4.11/主板1或2/...` 对应 `03_STM32双主板/主板1或2/...`。旧包证据路径不是本轮结果；本轮结果看09。

`collect_readonly.py` 仅供以后现场采集，运行前查看 `--help`。`run_offline_suite.py` 是本轮适配目录的离线测试入口，在安装网关依赖和pytest的Python环境下运行：

```text
python run_offline_suite.py
```

该入口禁用真实串口、外部HTTP与业务网络，使用临时库存和拍照目录。Windows asyncio内部回环可用。

`verify_package.py` 用Python标准库核对整理包中全部文件的SHA-256，可在任何工作目录运行。

`文档生成脚本` 保留原始生成脚本；其中一些仍写有原C盘绝对路径或原项目目录布局，不能直接按新目录运行。它们不参与网关/固件运行。`整理工具` 是本次打包脚本的归档，须在原项目tools目录运行。''',
        '09_整理校验': '''`文件来源清单.csv` / `.json` 记录每个复制文件的原路径、目标路径、哈希及转换说明；`未纳入内容.csv` 按原顶层目录汇总未收录数量和体积。

`本轮检查结果.json` 记录当前源码引用、页面一致性、AST检查及离线测试结果；`本轮网关测试.txt` 为本次运行日志；`文件校验清单.json` 用于包内校验，清单自身不在自身哈希列表中。

ZIP的最终SHA-256与CRC/内容核验结果写在压缩包旁边的 `.zip.sha256.txt` 和 `.zip.验收.json`，避免循环哈希。原项目约10.29 GiB包含Git历史、多个旧包和无关参考，压缩前后大小并非单纯压缩率。
''',
    }
    for section, desc in SECTIONS.items():
        write(section + '/README_本部分说明.md', '# ' + section + '\n\n' + desc + '\n\n' + notes[section])
    write('00_项目说明/项目组成与入口.md', '''# 项目组成与入口

| 部分 | 可编辑工程入口 | 职责 |
|---|---|---|
| 网页 | `../02_树莓派网关与网页/raspberry_pi_gateway/static/cloud_display.html` | 衣物管理、存取衣、护理与状态展示 |
| 树莓派 | `../02_树莓派网关与网页/raspberry_pi_gateway/run.py` | HTTP服务、相机、串口桥、库存与MQTT |
| 主板1 | `../03_STM32双主板/主板1/USER/MPU9250.uvprojx` | 协调、传感与护理控制 |
| 主板2 | `../03_STM32双主板/主板2/USER/MPU9250.uvprojx` | 电机、舵机和动作序列 |
| PCB | `../04_PCB与接线/立创EDA工程` | 两份本地原生硬件工程 |
| 安卓 | `../07_安卓客户端` | 既有安装包及其网页资源 |
| 云服务 | 未找到完整源码 | 需补充任务API、worker、MQTT消费、数据库及部署配置 |

先读各目录的README。本文为2026-10-25文件整理导航，不是现场联网、烧录或机械验收报告。
''')
    write('AGENTS.md', '''# 整理包协作入口

本文件适用于此精简整理包。先读根目录README和00中的项目组成、原始改动记录。实际源码入口以新目录为准，原导航仅作历史参考。

修改后在 `00_项目说明/后续改动记录.md` 追加时间、文件、变更、验证结果，以及是否烧录、复位、只读。不要把整理副本当作原工作目录自动部署。

树莓派和双主板源码保持原始业务逻辑。设备状态须现场核对；HX711是去皮counts，未经标定不是kg。缺失云端服务见README。
''')
    write('00_项目说明/后续改动记录.md', '# 整理包后续改动记录\n\n2026-10-25：建立按功能分类的副本；本次详情见原始改动记录与09中的校验材料。后续在此追加。')
    write('08_联调与开发工具/verify_package.py', '''"""Read-only verification, Python standard library only."""
from pathlib import Path
import hashlib, json, sys
root = Path(__file__).resolve().parents[1]
manifest = root / '09_整理校验/文件校验清单.json'
rows = json.loads(manifest.read_text(encoding='utf-8'))
errors = []
for row in rows:
    p = root / row['path']
    if not p.is_file():
        errors.append({'path': row['path'], 'error': 'missing'})
        continue
    with p.open('rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    if p.stat().st_size != row['bytes'] or digest != row['sha256']:
        errors.append({'path': row['path'], 'error': 'changed'})
expected = {x['path'] for x in rows} | {'09_整理校验/文件校验清单.json'}
actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
extra = sorted(actual - expected)
print(json.dumps({'checked': len(rows), 'errors': errors, 'extra_files': extra, 'ok': not errors and not extra}, ensure_ascii=False))
sys.exit(1 if errors or extra else 0)
''')
    cards = '\n'.join(f'<a class="card" href="{html.escape(s)}/"><strong>{html.escape(s)}</strong><p>{html.escape(d)}</p><span>打开文件夹 →</span></a>' for s, d in SECTIONS.items())
    write('打开项目导航.html', f'''<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>智能衣橱 · 项目导航</title>
<style>body{{margin:0;background:#f5f7fa;color:#182c36;font:16px/1.65 "Microsoft YaHei",sans-serif}}main{{max-width:1100px;margin:auto;padding:44px 28px}}header{{margin-bottom:24px}}.tag{{color:#086963;font-size:14px}}h1{{font-size:36px;margin:6px 0}}.intro{{max-width:900px;color:#52636c}}.grid{{display:grid;grid-template-columns:repeat(auto-fit,minmax(290px,1fr));gap:14px}}.card{{display:block;background:#fff;border:1px solid #dce5e9;border-radius:12px;padding:22px;color:inherit;text-decoration:none}}.card:hover{{border-color:#098577}}.card strong{{font-size:18px}}.card p{{font-size:14px;color:#52636c;min-height:48px}}.card span{{color:#08736c;font-size:14px}}.note{{background:#e8f2f1;border-radius:10px;padding:18px 22px;margin-top:24px}}footer{{font-size:13px;color:#61737c;margin-top:20px}}</style></head>
<body><main><header><div class="tag">本地工程整理 · 2026-10-25</div><h1>智能衣橱 · 完整项目</h1><p class="intro">按功能查找源码、硬件和文档。每个模块内都有“README_本部分说明.md”。当前工作源码来自本机原目录；比赛范例、其他项目与旧备份仍留在原处。</p></header>
<div class="grid">{cards}</div><div class="note"><strong>交付范围</strong><br>集中保存本地已具备的当前项目。云服务器完整源码、安卓原生构建工程尚缺；07提供现有APK及内嵌网页。此次未部署、烧录或复位设备。</div>
<footer>此页只用于离线导航，不连接设备或云端。若浏览器不支持文件夹链接，请在资源管理器中打开同名目录。</footer></main></body></html>''')


def build():
    items = selection()
    inv = source_inventory(items)
    OUT.mkdir(exist_ok=False)
    AUDIT.mkdir()
    records = [copy_one(item) for item in items]
    write_json(AUDIT / '文件来源清单.json', records)
    write_csv(AUDIT / '文件来源清单.csv', records)
    write_json(AUDIT / '原目录统计.json', inv)
    rows = []
    included_bytes = defaultdict(int)
    for item in records:
        if not item['archive_member']:
            included_bytes[item['source'].split('/')[0]] += item['source_bytes']
    reasons = {
        '.git': 'Git历史与对象库，留在原目录',
        '99_项目资料_纯净版': '旧整理副本，当前源码从现行工作目录读取',
        '项目资料.zip': '旧整包备份，留在原目录',
        '文书': '只取本项目相关器件说明；其他团队比赛视频、软著范例等不纳入',
        '代码': '仅保留4.11双板源工程及对照产物；农装、串口屏、旧备份、编译中间件留原处',
        'artifacts': '仅取已有APK、系统配图；设备读回和历史调试备份留原处，私钥不纳入',
        'outputs': '仅取本项目演示PPT、海报、PDF；缓存、环境、提取文件及历史部署临时件留原处',
        'PCB': '仅取当前两份eprj2；旧备份留原处',
        '02_界面原型': '仅取当前cloud_display；旧K230页面、串口屏PPT和版本备份留原处',
        '护衣系统': '仅取设计说明及HX711关键资料；旧演示源码、空草案和重复ZIP留原处',
        '01_智能衣橱方案': '保留现有方案与图片；旧交付ZIP、WPS重复副本、K230旧链路留原处',
        'raspberry_pi_gateway': '仅排除缓存、虚拟环境、实时库存和相机照片目录',
        HANDOFF: '仅选近期排查说明、样例和工具；重复源码与历史验收包留原处',
    }
    for top, st in inv['groups'].items():
        if st['included_sources'] == st['files']:
            continue
        rows.append(dict(原目录项=top, 原文件数=st['files'], 收录源文件数=st['included_sources'],
                         未纳入文件数=st['files']-st['included_sources'],
                         未纳入字节数=max(0, st['bytes']-included_bytes[top]),
                         说明=reasons.get(top, '历史参考、其他项目、个人工具配置或临时环境；保留原目录')))
    write_csv(AUDIT / '未纳入内容.csv', rows)
    write_guides(inv)
    print(json.dumps({'folder': str(OUT), 'copied_or_extracted_files': len(records),
                      'copied_bytes': sum(x['package_bytes'] for x in records)}, ensure_ascii=False))


def inspect_package() -> dict:
    records = json.loads((AUDIT / '文件来源清单.json').read_text(encoding='utf-8'))
    bad_copy = [x['package_path'] for x in records if sha(OUT / x['package_path']) != x['package_sha256']]
    projects = []
    for proj in sorted((OUT / '03_STM32双主板').glob('主板*/USER/*.uvprojx')):
        tree = ET.parse(proj)
        refs = [p.text for p in tree.findall('.//FilePath') if p.text]
        missing = [s for s in refs if not (proj.parent / s.replace('\\', '/')).is_file()]
        incs = [x.strip() for el in tree.findall('.//IncludePath') if el.text for x in el.text.split(';') if x.strip()]
        missing_inc = [s for s in incs if not (proj.parent / s.replace('\\', '/')).is_dir()]
        projects.append(dict(project=proj.relative_to(OUT).as_posix(), file_references=len(refs),
                             missing_files=missing, missing_include_dirs=missing_inc))
    syntax = []
    for p in sorted((OUT / '02_树莓派网关与网页/raspberry_pi_gateway').rglob('*.py')):
        ast.parse(p.read_text(encoding='utf-8-sig'), filename=str(p))
        syntax.append(p.relative_to(OUT).as_posix())
    for p in (OUT / '08_联调与开发工具').glob('*.py'):
        ast.parse(p.read_text(encoding='utf-8-sig'), filename=str(p))
    a = OUT / '02_树莓派网关与网页/raspberry_pi_gateway/static/cloud_display.html'
    b = OUT / '02_树莓派网关与网页/02_界面原型/cloud_display.html'
    testlog = AUDIT / '本轮网关测试.txt'
    log = testlog.read_text(encoding='utf-8-sig') if testlog.exists() else ''
    passed = re.findall(r'(\d+) passed', log)
    page_report = AUDIT / '页面语法检查.json'
    page_check = json.loads(page_report.read_text(encoding='utf-8')) if page_report.exists() else {}
    nav_links = re.findall(r'href="([^"]+)"', (OUT / '打开项目导航.html').read_text(encoding='utf-8'))
    missing_links = [s for s in nav_links if not (OUT / s).exists()]
    forbidden = [p.relative_to(OUT).as_posix() for p in OUT.rglob('*') if p.is_file()
                 and (p.suffix.lower() in {'.pem', '.key', '.pyc', '.o', '.crf', '.d'}
                      or p.name == '.env' or '__pycache__' in p.parts or '.pytest_cache' in p.parts)]
    long_paths = [p.relative_to(OUT).as_posix() for p in OUT.rglob('*') if p.is_file() and len(str(p)) >= 250]
    result = dict(time=datetime.now(TZ).isoformat(), copy_count=len(records), copy_hash_errors=bad_copy,
                  keil_projects=projects, gateway_python_ast_count=len(syntax), pages_identical=sha(a)==sha(b),
                  offline_tests={'passed': int(passed[-1]) if passed else None, 'log': '本轮网关测试.txt'},
                  javascript_syntax=page_check, missing_navigation_targets=missing_links,
                  forbidden_cache_or_key_files=forbidden,
                  long_windows_paths=long_paths,
                  not_performed=['Keil重新编译', '现场部署', '真实云端请求', '串口或DAP连接', '烧录', '复位', '机械动作'])
    result['ok'] = (not bad_copy and all(not p['missing_files'] and not p['missing_include_dirs'] for p in projects)
                    and result['pages_identical'] and bool(passed) and not long_paths and not forbidden
                    and not missing_links and bool(page_check)
                    and all(x['status'] == 'passed' for x in page_check['results']))
    write_json(AUDIT / '本轮检查结果.json', result)
    print(json.dumps(result, ensure_ascii=False))
    return result


def seal():
    records = json.loads((AUDIT / '文件来源清单.json').read_text(encoding='utf-8'))
    # Synchronize the authorized appended changelog and the final packaging script.
    for i, item in enumerate(records):
        if item['source'] in {'00_项目导航/改动记录.md', 'tools/package_current_project_20261025.py'}:
            spec = {k: item[k] for k in ('source', 'package_path', 'purpose', 'transformation', 'archive_member')}
            records[i] = copy_one(spec)
        else:
            assert sha(ROOT / item['source']) == item['source_sha256'], 'Source changed: ' + item['source']
    write_json(AUDIT / '文件来源清单.json', records)
    write_csv(AUDIT / '文件来源清单.csv', records)
    result = inspect_package()
    assert result['ok'], 'Validation failed'
    # Enumerate assets for human navigation; full checksums follow.
    files = sorted(p.relative_to(OUT).as_posix() for p in OUT.rglob('*') if p.is_file())
    write('文件目录.txt', '\n'.join([NAME + '/', ''] + files + ['文件目录.txt', '09_整理校验/文件校验清单.json']))
    checks = []
    for p in sorted(OUT.rglob('*')):
        if p.is_file() and p != AUDIT / '文件校验清单.json':
            checks.append(dict(path=p.relative_to(OUT).as_posix(), bytes=p.stat().st_size, sha256=sha(p)))
    write_json(AUDIT / '文件校验清单.json', checks)
    archive = ROOT / (NAME + '.zip')
    assert not archive.exists(), 'Refuse to overwrite existing ZIP'
    actual = sorted(p for p in OUT.rglob('*') if p.is_file())
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as z:
        for p in actual:
            z.write(p, NAME + '/' + p.relative_to(OUT).as_posix())
    archive_check = []
    with zipfile.ZipFile(archive) as z:
        names = z.namelist()
        assert len(names) == len(set(n.casefold() for n in names)) == len(actual)
        assert z.testzip() is None
        for p in actual:
            n = NAME + '/' + p.relative_to(OUT).as_posix()
            with z.open(n) as f:
                digest = hashlib.file_digest(f, 'sha256').hexdigest()
            assert digest == sha(p), n
            archive_check.append(n)
    digest = sha(archive)
    receipt = dict(time=datetime.now(TZ).isoformat(), folder=str(OUT), archive=str(archive),
                   files=len(actual), folder_bytes=sum(p.stat().st_size for p in actual),
                   zip_bytes=archive.stat().st_size, zip_sha256=digest, zip_crc='passed',
                   zip_unique_paths='passed', zip_content_sha256='passed',
                   source_files_preserved=True, validation=result, ok=True)
    (ROOT / (NAME + '.zip.sha256.txt')).write_text(digest + '  ' + archive.name + '\n', encoding='utf-8')
    write_json(ROOT / (NAME + '.zip.验收.json'), receipt)
    print(json.dumps(receipt, ensure_ascii=False))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('phase', choices=['plan', 'build', 'inspect', 'seal'])
    args = parser.parse_args()
    if args.phase == 'plan':
        items = selection()
        stats = defaultdict(lambda: [0, 0])
        for x in items:
            key = x['package_path'].split('/')[0]
            stats[key][0] += 1
            stats[key][1] += (ROOT / x['source']).stat().st_size if not x['archive_member'] else 0
        print(json.dumps(dict(files=len(items), sections=dict(stats)), ensure_ascii=False, indent=2))
    elif args.phase == 'build':
        build()
    elif args.phase == 'inspect':
        inspect_package()
    elif args.phase == 'seal':
        seal()
