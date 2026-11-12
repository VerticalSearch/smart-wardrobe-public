from pathlib import Path

from docx import Document
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.shared import Pt, Inches, RGBColor


ROOT = Path(r"C:\Users\ECHO\Desktop\家居")
MD_PATH = ROOT / "豆包_PPT提示词_当前方案.md"
DOCX_PATH = ROOT / "豆包_PPT提示词_当前方案.docx"


def set_run_font(run, name="Arial", size=11, bold=False, color=None):
    run.font.name = name
    run._element.rPr.rFonts.set(qn("w:ascii"), name)
    run._element.rPr.rFonts.set(qn("w:hAnsi"), name)
    run._element.rPr.rFonts.set(qn("w:eastAsia"), "Microsoft YaHei")
    run.font.size = Pt(size)
    run.bold = bold
    if color:
        run.font.color.rgb = RGBColor.from_string(color)


def add_code_block(doc, text):
    p = doc.add_paragraph()
    p.paragraph_format.space_before = Pt(4)
    p.paragraph_format.space_after = Pt(8)
    p.paragraph_format.left_indent = Inches(0.2)
    p.paragraph_format.right_indent = Inches(0.2)
    p.paragraph_format.line_spacing = 1.1
    run = p.add_run(text)
    set_run_font(run, name="Consolas", size=9)


def add_heading(doc, text, level=1):
    p = doc.add_paragraph()
    p.style = f"Heading {level}"
    p.paragraph_format.space_before = Pt(14 if level == 1 else 10)
    p.paragraph_format.space_after = Pt(6)
    run = p.add_run(text)
    set_run_font(run, name="Arial", size=16 if level == 1 else 13, bold=True, color="1F4E79")


def add_body(doc, text):
    p = doc.add_paragraph()
    p.paragraph_format.space_after = Pt(6)
    p.paragraph_format.line_spacing = 1.15
    run = p.add_run(text)
    set_run_font(run, name="Arial", size=11)


def main():
    raw = MD_PATH.read_text(encoding="utf-8")

    overview_start = raw.index("## 一页看懂当前方案")
    image_start = raw.index("## 图片插入清单")
    direct_start = raw.index("## 给豆包的压缩版提示词")
    use_start = raw.index("## 使用建议")

    overview_block = raw[overview_start:image_start]
    image_block = raw[image_start:direct_start]
    direct_block = raw[direct_start:use_start]
    use_block = raw[use_start:]

    def extract_code(block):
        first = block.index("```") + 3
        second = block.index("\n", first) + 1
        end = block.rindex("```")
        return block[second:end].strip()

    overview_text = "\n".join(
        line for line in overview_block.splitlines()
        if line.strip() and not line.startswith("## ")
    ).strip()
    image_text = "\n".join(
        line for line in image_block.splitlines()
        if line.strip() and not line.startswith("## ")
    ).strip()
    direct_text = extract_code(direct_block)
    use_text = extract_code(use_block)

    doc = Document()
    section = doc.sections[0]
    section.top_margin = Inches(1)
    section.bottom_margin = Inches(1)
    section.left_margin = Inches(1)
    section.right_margin = Inches(1)

    title = doc.add_paragraph()
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title.paragraph_format.space_after = Pt(10)
    r = title.add_run("豆包 PPT 提示词")
    set_run_font(r, name="Arial", size=22, bold=True, color="17365D")

    sub = doc.add_paragraph()
    sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
    sub.paragraph_format.space_after = Pt(18)
    r = sub.add_run("智能衣橱当前正式方案")
    set_run_font(r, name="Arial", size=12, color="555555")

    add_body(doc, "用途：把下面内容直接贴进豆包，让它根据当前正式方案生成答辩 PPT。")

    add_heading(doc, "一页看懂当前方案", 1)
    for part in overview_text.split("\n"):
        if part.strip():
            add_body(doc, part.strip())

    add_heading(doc, "图片插入清单", 1)
    for part in image_text.split("\n"):
        if part.strip():
            add_body(doc, part.strip())

    add_heading(doc, "给豆包的压缩版提示词", 1)
    add_code_block(doc, direct_text)

    add_heading(doc, "使用建议", 1)
    add_code_block(doc, use_text)

    doc.save(DOCX_PATH)
    print(f"DOCX_OK {DOCX_PATH}")


if __name__ == "__main__":
    main()
