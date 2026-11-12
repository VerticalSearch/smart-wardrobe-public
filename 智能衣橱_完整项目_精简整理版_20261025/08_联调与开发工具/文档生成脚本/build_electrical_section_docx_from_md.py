# -*- coding: utf-8 -*-
from pathlib import Path

from docx import Document
from docx.enum.table import WD_ALIGN_VERTICAL, WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt, RGBColor


ROOT = Path(__file__).resolve().parent
DOC_DIR = ROOT / "01_智能衣橱方案"
MD_PATH = DOC_DIR / "智能衣橱申报书_电控部分内容.md"
DOCX_PATH = DOC_DIR / "智能衣橱申报书_电控部分内容.docx"

FONT_BODY = "宋体"
FONT_HEAD = "微软雅黑"
BLUE = RGBColor(37, 99, 145)
DARK = RGBColor(31, 45, 61)
MUTED = RGBColor(92, 106, 126)


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


def add_heading(doc, text):
    p = doc.add_paragraph(style="Heading 1")
    p.paragraph_format.space_before = Pt(8)
    p.paragraph_format.space_after = Pt(5)
    run = p.add_run(text)
    set_run_font(run, FONT_HEAD, 15, True, BLUE)


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


def parse_markdown(path):
    raw_lines = path.read_text(encoding="utf-8").splitlines()
    blocks = []
    i = 0
    while i < len(raw_lines):
        line = raw_lines[i].strip()
        if not line:
            i += 1
            continue
        if line.startswith("# "):
            blocks.append(("title", line[2:].strip()))
            i += 1
        elif line.startswith("## "):
            blocks.append(("h1", line[3:].strip()))
            i += 1
        elif line.startswith(">"):
            blocks.append(("note", line.lstrip(">").strip()))
            i += 1
        elif line.startswith("|"):
            rows = []
            while i < len(raw_lines) and raw_lines[i].strip().startswith("|"):
                row = [cell.strip() for cell in raw_lines[i].strip().strip("|").split("|")]
                if not all(set(cell) <= {"-", ":"} for cell in row):
                    rows.append(row)
                i += 1
            if rows:
                blocks.append(("table", rows))
        else:
            parts = [line]
            i += 1
            while i < len(raw_lines):
                nxt = raw_lines[i].strip()
                if not nxt or nxt.startswith(("#", ">", "|")):
                    break
                parts.append(nxt)
                i += 1
            blocks.append(("p", "".join(parts)))
    return blocks


def build_docx():
    blocks = parse_markdown(MD_PATH)
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

    style = styles["Heading 1"]
    style.font.name = FONT_HEAD
    style._element.rPr.rFonts.set(qn("w:eastAsia"), FONT_HEAD)
    style.font.bold = True
    style.font.color.rgb = BLUE

    footer = section.footer.paragraphs[0]
    footer.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = footer.add_run("智能衣橱申报书电控部分内容")
    set_run_font(run, FONT_BODY, 9, False, MUTED)

    for index, (kind, value) in enumerate(blocks):
        if kind == "title":
            p = doc.add_paragraph()
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            p.paragraph_format.space_after = Pt(8)
            run = p.add_run(value)
            set_run_font(run, FONT_HEAD, 20, True, DARK)
        elif kind == "p" and index == 1:
            add_paragraph(doc, value, size=10.5, color=MUTED, align=WD_ALIGN_PARAGRAPH.CENTER, after=12)
        elif kind == "h1":
            add_heading(doc, value)
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


if __name__ == "__main__":
    print(build_docx())
