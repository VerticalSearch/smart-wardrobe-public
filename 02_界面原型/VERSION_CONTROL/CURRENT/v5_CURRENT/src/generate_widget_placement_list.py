from __future__ import annotations

import csv
import json
import re
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT_DIR = ROOT / "reports"
REF_DIR = ROOT / "reference_tables"
MANIFEST = ROOT / "manifest_v5.json"


NAV_ITEMS = [
    ("home", "首页", "NAV_HOME"),
    ("retrieve", "取衣", "NAV_RETRIEVE"),
    ("return", "归还", "NAV_RETURN"),
    ("treat", "护理", "NAV_TREAT"),
    ("browse", "浏览", "NAV_BROWSE"),
    ("network", "网络", "NAV_NETWORK"),
]


def clean_id(value: str) -> str:
    mapping = {
        "护理": "treat",
        "浏览": "browse",
        "网络": "network",
        "取衣": "retrieve",
        "归还": "return",
        "首页": "home",
        "热风": "hotwind",
        "负离子": "ion",
        "组合": "combo",
        "连接": "connect",
        "取消": "cancel",
    }
    value = mapping.get(value, value)
    value = value.replace("-", "_").replace("/", "_")
    value = re.sub(r"[^A-Za-z0-9_]+", "_", value)
    value = re.sub(r"_+", "_", value).strip("_")
    return value.lower() or "item"


def round_px(value) -> int:
    return int(round(float(value)))


def get_bbox(element: dict) -> tuple[int, int, int, int]:
    x, y, w, h = element["bbox"]
    return round_px(x), round_px(y), round_px(w), round_px(h)


def bbox_from_values(x, y, w, h) -> tuple[int, int, int, int]:
    return round_px(x), round_px(y), round_px(w), round_px(h)


def text_of(element: dict | None) -> str:
    if not element:
        return ""
    return element.get("text") or element.get("textPreview") or ""


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def find_name(layout: dict, name: str) -> dict | None:
    for element in layout["elements"]:
        if element.get("name") == name:
            return element
    return None


def find_text(layout: dict, text: str) -> dict | None:
    for element in layout["elements"]:
        if text_of(element) == text:
            return element
    return None


def previous_shape_for_text(layout: dict, text: str) -> dict | None:
    text_element = find_text(layout, text)
    if not text_element:
        return None
    order = text_element["order"]
    candidates = [
        e
        for e in layout["elements"]
        if e.get("kind") == "shape" and not text_of(e) and e.get("order", 0) < order
    ]
    return candidates[-1] if candidates else None


def add_row(rows: list[dict], page: dict, component_id: str, component_type: str, x: int, y: int, w: int, h: int,
            initial_text: str = "", event: str = "", update_source: str = "", layer: str = "runtime",
            note: str = "", source: str = "") -> None:
    rows.append(
        {
            "page_no": f"{page['no']:02d}",
            "page_name": page["name"],
            "page_title": page["title"],
            "component_id": component_id,
            "component_type": component_type,
            "layer": layer,
            "x": x,
            "y": y,
            "w": w,
            "h": h,
            "initial_text": initial_text,
            "event": event,
            "update_source": update_source,
            "note": note,
            "source": source,
        }
    )


def add_element_row(rows: list[dict], page: dict, layout_file: Path, element: dict) -> None:
    x, y, w, h = get_bbox(element)
    rows.append(
        {
            "page_no": f"{page['no']:02d}",
            "page_name": page["name"],
            "page_title": page["title"],
            "element_name": element.get("name", ""),
            "kind": element.get("kind", ""),
            "x": x,
            "y": y,
            "w": w,
            "h": h,
            "text": text_of(element),
            "fillColor": element.get("fillColor", ""),
            "lineColor": element.get("lineColor", ""),
            "source_file": layout_file.name,
        }
    )


def add_status_and_nav(rows: list[dict], page: dict, layout: dict) -> None:
    page_prefix = f"p{page['no']:02d}"
    status = find_name(layout, "status-line")
    if status:
        add_row(
            rows,
            page,
            f"{page_prefix}_txt_status",
            "text",
            *get_bbox(status),
            initial_text=text_of(status),
            update_source="controller",
            layer="optional_dynamic",
            note="顶部状态文字；如果固定不变可烘焙到底图",
            source="status-line",
        )
    action = find_name(layout, "action-line")
    if action:
        add_row(
            rows,
            page,
            f"{page_prefix}_txt_action",
            "text",
            *get_bbox(action),
            initial_text=text_of(action),
            update_source="controller",
            layer="optional_dynamic",
            note="顶部副提示文字；如果固定不变可烘焙到底图",
            source="action-line",
        )
    for key, label, event in NAV_ITEMS:
        element = find_name(layout, f"nav-{key}")
        if not element:
            continue
        add_row(
            rows,
            page,
            f"{page_prefix}_hot_nav_{key}",
            "touch_hotspot",
            *get_bbox(element),
            initial_text=label,
            event=event,
            update_source="touch",
            note="底部导航透明触摸区；文字和高亮可用背景图",
            source=f"nav-{key}",
        )


def add_home_controls(rows: list[dict], page: dict, layout: dict) -> None:
    mapping = [
        ("btn-retrieve", "retrieve", "取衣", "HOME_RETRIEVE"),
        ("btn-return", "return", "归还", "HOME_RETURN"),
        ("btn-护理", "treat", "护理", "HOME_TREAT"),
        ("btn-浏览", "browse", "浏览", "HOME_BROWSE"),
        ("btn-网络", "network", "网络", "HOME_NETWORK"),
    ]
    for source_name, suffix, label, event in mapping:
        element = find_name(layout, source_name)
        if not element:
            continue
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_btn_{suffix}",
            "button",
            *get_bbox(element),
            initial_text=label,
            event=event,
            update_source="touch",
            note="首页主操作按钮",
            source=source_name,
        )


def add_list_controls(rows: list[dict], page: dict, layout: dict, event_prefix: str) -> None:
    for idx in range(1, 4):
        thumb = find_name(layout, f"thumb-placeholder-{idx}")
        title = find_name(layout, f"item-title-{idx}")
        meta = find_name(layout, f"item-meta-{idx}")
        if not (thumb and title and meta):
            continue
        tx, ty, tw, th = get_bbox(thumb)
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_hot_item_{idx}",
            "touch_hotspot",
            34,
            ty,
            732,
            66,
            initial_text=f"item_{idx}",
            event=f"{event_prefix}_ITEM_{idx}",
            update_source="touch",
            note="列表行透明触摸区",
            source=f"row-from-thumb-placeholder-{idx}",
        )
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_pic_item_{idx}",
            "image",
            tx,
            ty,
            tw,
            th,
            update_source="clothes_data",
            note="衣物缩略图/占位图",
            source=f"thumb-placeholder-{idx}",
        )
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_txt_item_{idx}_title",
            "text",
            *get_bbox(title),
            initial_text=text_of(title),
            update_source="clothes_data",
            note="衣物名称",
            source=f"item-title-{idx}",
        )
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_txt_item_{idx}_meta",
            "text",
            *get_bbox(meta),
            initial_text=text_of(meta),
            update_source="clothes_data",
            note="类别 / 挂位 / 状态",
            source=f"item-meta-{idx}",
        )


def add_progress_controls(rows: list[dict], page: dict, layout: dict) -> None:
    progress_text = find_name(layout, "progress-number")
    if progress_text:
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_txt_progress_percent",
            "text",
            *get_bbox(progress_text),
            initial_text=text_of(progress_text),
            update_source="controller",
            note="进度百分比",
            source="progress-number",
        )
    fill = find_name(layout, "progress-fill-placeholder")
    bar_y = get_bbox(fill)[1] if fill else 296
    add_row(
        rows,
        page,
        f"p{page['no']:02d}_bar_progress",
        "progress_bar",
        34,
        bar_y,
        732,
        20,
        initial_text=(text_of(progress_text).replace("%", "") if progress_text else ""),
        update_source="controller",
        note="使用全宽 0-100 进度条；背景条可烘焙到底图",
        source="progress-fill-placeholder/full-bar",
    )


def add_result_controls(rows: list[dict], page: dict, layout: dict) -> None:
    title_text = "等待确认" if page["name"] == "error" else "已完成"
    sub_text = "查看原因并处理" if page["name"] == "error" else "结果已同步"
    title = find_text(layout, title_text)
    sub = find_text(layout, sub_text)
    if title:
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_txt_result_title",
            "text",
            *get_bbox(title),
            initial_text=title_text,
            update_source="controller",
            note="结果主提示",
            source=f"text:{title_text}",
        )
    if sub:
        sx, sy, sw, sh = get_bbox(sub)
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_txt_result_hint",
            "text",
            sx,
            sy,
            max(sw, 360),
            sh,
            initial_text=sub_text,
            update_source="controller",
            note="结果副提示，宽度按代码 wrap(360) 预留",
            source=f"text:{sub_text}",
        )
    if page["name"] == "error":
        main = find_name(layout, "main-stage")
        if main:
            add_row(
                rows,
                page,
                f"p{page['no']:02d}_hot_error_confirm",
                "touch_hotspot",
                *get_bbox(main),
                initial_text="确认",
                event="ERROR_CONFIRM",
                update_source="touch",
                note="异常提示确认/继续区域",
                source="main-stage",
            )


def add_return_prepare_controls(rows: list[dict], page: dict, layout: dict) -> None:
    prompt = find_name(layout, "return-prompt") or find_name(layout, "main-stage")
    if prompt:
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_hot_return_start",
            "touch_hotspot",
            *get_bbox(prompt),
            initial_text="开始归还",
            event="RETURN_START",
            update_source="touch",
            note="归还准备页整块启动触摸区",
            source=prompt.get("name", "main-stage"),
        )
    for suffix, label in [("prompt_title", "归还杆就绪"), ("prompt_hint", "挂好衣物后点击开始")]:
        element = find_text(layout, label)
        if element:
            add_row(
                rows,
                page,
                f"p{page['no']:02d}_txt_{suffix}",
                "text",
                *get_bbox(element),
                initial_text=label,
                update_source="controller",
                layer="optional_dynamic",
                note="归还准备提示；固定不变可烘焙到底图",
                source=f"text:{label}",
            )


def add_treat_controls(rows: list[dict], page: dict, layout: dict) -> None:
    for label, suffix, event in [
        ("热风", "hotwind", "TREAT_HOTWIND"),
        ("负离子", "ion", "TREAT_ION"),
        ("组合", "combo", "TREAT_COMBO"),
    ]:
        button = previous_shape_for_text(layout, label)
        if not button:
            continue
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_btn_{suffix}",
            "button",
            *get_bbox(button),
            initial_text=label,
            event=event,
            update_source="touch",
            note="护理模式选择按钮",
            source=f"button-before-text:{label}",
        )


def add_network_controls(rows: list[dict], page: dict, layout: dict) -> None:
    labels = [("Wi-Fi 名称", "wifi_1", "WIFI_SELECT_1"), ("信号强度", "wifi_2", "WIFI_SELECT_2"), ("连接状态", "wifi_3", "WIFI_SELECT_3")]
    for label, suffix, event in labels:
        text_element = find_text(layout, label)
        if not text_element:
            continue
        _, text_y, _, _ = get_bbox(text_element)
        row_y = text_y - 18
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_hot_{suffix}",
            "touch_hotspot",
            34,
            row_y,
            732,
            56,
            initial_text=label,
            event=event,
            update_source="touch",
            note="Wi-Fi 列表行触摸区",
            source=f"wifi-row:{label}",
        )
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_txt_{suffix}",
            "text",
            *get_bbox(text_element),
            initial_text=label,
            update_source="wifi_scan",
            note="Wi-Fi 列表显示文字",
            source=f"text:{label}",
        )


def add_password_controls(rows: list[dict], page: dict, layout: dict) -> None:
    for label, suffix, component_type, event, source in [
        ("SSID", "ssid", "text", "", "wifi_scan"),
        ("请输入密码", "password", "input_text", "", "user_input"),
        ("连接", "connect", "button", "WIFI_CONNECT", "touch"),
        ("取消", "cancel", "button", "WIFI_CANCEL", "touch"),
    ]:
        field = previous_shape_for_text(layout, label)
        if not field:
            continue
        add_row(
            rows,
            page,
            f"p{page['no']:02d}_{'btn' if component_type == 'button' else 'txt'}_{suffix}",
            component_type,
            *get_bbox(field),
            initial_text=label,
            event=event,
            update_source=source,
            note="Wi-Fi 密码页控件",
            source=f"shape-before-text:{label}",
        )


def build_rows() -> tuple[list[dict], list[dict], list[dict]]:
    manifest = load_json(MANIFEST)
    widget_rows: list[dict] = []
    all_elements: list[dict] = []
    pages_summary: list[dict] = []

    for page in manifest["pages"]:
        layout_file = REPORT_DIR / f"backgrounds_{page['no']:02d}_{page['name']}.layout.json"
        layout = load_json(layout_file)
        before = len(widget_rows)

        for element in layout["elements"]:
            if element.get("name"):
                add_element_row(all_elements, page, layout_file, element)

        add_status_and_nav(widget_rows, page, layout)
        if page["name"] == "home":
            add_home_controls(widget_rows, page, layout)
        elif page["name"] == "retrieve_select":
            add_list_controls(widget_rows, page, layout, "RETRIEVE")
        elif page["name"] == "browse":
            add_list_controls(widget_rows, page, layout, "BROWSE")
        elif page["name"] in {"retrieve_progress", "return_progress", "treat_progress"}:
            add_progress_controls(widget_rows, page, layout)
        elif page["name"] in {"retrieve_done", "return_done", "treat_done", "error"}:
            add_result_controls(widget_rows, page, layout)
        elif page["name"] == "return_prepare":
            add_return_prepare_controls(widget_rows, page, layout)
        elif page["name"] == "treat_select":
            add_treat_controls(widget_rows, page, layout)
        elif page["name"] == "network":
            add_network_controls(widget_rows, page, layout)
        elif page["name"] == "wifi_password":
            add_password_controls(widget_rows, page, layout)

        pages_summary.append(
            {
                "page_no": f"{page['no']:02d}",
                "page_name": page["name"],
                "page_title": page["title"],
                "recommended_component_count": len(widget_rows) - before,
            }
        )

    return widget_rows, all_elements, pages_summary


def write_csv(path: Path, rows: list[dict], fields: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def write_markdown(path: Path, rows: list[dict], summary: list[dict]) -> None:
    counts = Counter(row["component_type"] for row in rows)
    lines = [
        "# v5 串口屏控件摆放清单",
        "",
        "- 坐标系：800x480 px，左上角为 (0,0)。",
        "- 坐标来源：`reports/backgrounds_*.layout.json`，也就是 v5 自动生成工程导出的真实布局。",
        "- 推荐用法：背景图可直接使用 `previews/backgrounds_*.png`；本表主要列需要交互或运行时更新的控件。",
        "- `optional_dynamic` 表示可做成控件，也可烘焙进背景图，取决于 STM32 是否需要运行时修改。",
        "",
        "## 汇总",
        "",
        "| 类型 | 数量 |",
        "|---|---:|",
    ]
    for component_type, count in sorted(counts.items()):
        lines.append(f"| {component_type} | {count} |")
    lines.extend(["", "## 页面数量", "", "| 页码 | 页面 | 标题 | 推荐控件数 |", "|---|---|---|---:|"])
    for row in summary:
        lines.append(
            f"| {row['page_no']} | {row['page_name']} | {row['page_title']} | {row['recommended_component_count']} |"
        )
    lines.extend(["", "## 控件明细", ""])
    current_page = None
    for idx, row in enumerate(rows):
        page_label = f"{row['page_no']} {row['page_name']} / {row['page_title']}"
        if page_label != current_page:
            current_page = page_label
            lines.extend(
                [
                    f"### {page_label}",
                    "",
                    "| component_id | type | x | y | w | h | text | event | note |",
                    "|---|---|---:|---:|---:|---:|---|---|---|",
                ]
            )
        lines.append(
            "| {component_id} | {component_type} | {x} | {y} | {w} | {h} | {initial_text} | {event} | {note} |".format(
                **row
            )
        )
        if idx == len(rows) - 1 or rows[idx + 1]["page_no"] != row["page_no"]:
            lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    REF_DIR.mkdir(parents=True, exist_ok=True)
    widget_rows, all_elements, summary = build_rows()

    widget_fields = [
        "page_no",
        "page_name",
        "page_title",
        "component_id",
        "component_type",
        "layer",
        "x",
        "y",
        "w",
        "h",
        "initial_text",
        "event",
        "update_source",
        "note",
        "source",
    ]
    all_fields = [
        "page_no",
        "page_name",
        "page_title",
        "element_name",
        "kind",
        "x",
        "y",
        "w",
        "h",
        "text",
        "fillColor",
        "lineColor",
        "source_file",
    ]
    summary_fields = ["page_no", "page_name", "page_title", "recommended_component_count"]

    write_csv(REF_DIR / "widget_placement_list_v5.csv", widget_rows, widget_fields)
    write_csv(REF_DIR / "widget_placement_all_named_elements_v5.csv", all_elements, all_fields)
    write_csv(REF_DIR / "widget_placement_page_summary_v5.csv", summary, summary_fields)
    (REF_DIR / "widget_placement_list_v5.json").write_text(
        json.dumps(widget_rows, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    write_markdown(REF_DIR / "widget_placement_list_v5.md", widget_rows, summary)

    print(
        json.dumps(
            {
                "widget_count": len(widget_rows),
                "named_element_count": len(all_elements),
                "pages": len(summary),
                "out_dir": str(REF_DIR),
            },
            ensure_ascii=False,
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
