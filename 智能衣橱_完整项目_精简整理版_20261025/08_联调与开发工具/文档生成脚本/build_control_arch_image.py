from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont


OUT = Path(r"C:\Users\ECHO\Desktop\家居\artifacts\电控配图_树莓派双板架构.png")
OUT.parent.mkdir(parents=True, exist_ok=True)

W, H = 1600, 980


def load_font(size, bold=False):
    names = [
        "msyhbd.ttc" if bold else "msyh.ttc",
        "simhei.ttf",
        "arial.ttf",
    ]
    for name in names:
        try:
            return ImageFont.truetype(name, size=size)
        except Exception:
            continue
    return ImageFont.load_default()


img = Image.new("RGBA", (W, H), (255, 255, 255, 0))
canvas = Image.new("RGBA", (W, H), (250, 250, 247, 255))

shadow = Image.new("RGBA", (W, H), (0, 0, 0, 0))
sd = ImageDraw.Draw(shadow)
sd.rounded_rectangle((70, 70, W - 70, H - 70), radius=30, fill=(0, 0, 0, 40))
shadow = shadow.filter(ImageFilter.GaussianBlur(22))
canvas.alpha_composite(shadow)

d = ImageDraw.Draw(canvas)
d.rounded_rectangle((90, 90, W - 90, H - 90), radius=28, fill=(255, 255, 255, 255), outline=(224, 228, 236, 255), width=2)

title_font = load_font(26, bold=True)
sub_font = load_font(20, bold=False)
label_font = load_font(22, bold=True)
small_font = load_font(18, bold=False)

ink = (47, 58, 77, 255)
muted = (111, 122, 140, 255)
green = (70, 158, 106, 255)
blue = (77, 125, 222, 255)
orange = (241, 127, 37, 255)
line = (205, 213, 227, 255)
board_green = (71, 143, 88, 255)
board_dark = (43, 97, 57, 255)
pin = (214, 220, 228, 255)

d.text((120, 118), "树莓派 + 双主板电控架构示意", font=title_font, fill=ink)
d.text((120, 158), "树莓派负责触摸屏网页、外接摄像头拍照上传与串口桥接", font=sub_font, fill=muted)


def draw_board(x, y, w, h, title, subtitle, icon_color):
    d.rounded_rectangle((x, y, x + w, y + h), radius=22, fill=(245, 251, 246, 255), outline=(135, 179, 146, 255), width=2)
    d.rounded_rectangle((x + 18, y + 18, x + w - 18, y + h - 18), radius=18, fill=board_green, outline=board_dark, width=2)
    for i in range(10):
        px = x + 42 + i * ((w - 84) // 9)
        d.rectangle((px, y + 10, px + 10, y + 22), fill=pin)
        d.rectangle((px, y + h - 22, px + 10, y + h - 10), fill=pin)
    d.rounded_rectangle((x + 34, y + 40, x + 144, y + 120), radius=10, fill=(38, 61, 47, 255))
    d.rectangle((x + w - 128, y + 42, x + w - 46, y + 96), fill=(39, 66, 49, 255))
    d.ellipse((x + w // 2 - 26, y + h // 2 - 26, x + w // 2 + 26, y + h // 2 + 26), fill=(61, 113, 74, 255))
    d.text((x + 30, y + h + 16), title, font=label_font, fill=icon_color)
    d.text((x + 30, y + h + 50), subtitle, font=small_font, fill=muted)


def arrow(x1, y1, x2, y2, color):
    d.line((x1, y1, x2, y2), fill=color, width=12)
    import math
    ang = math.atan2(y2 - y1, x2 - x1)
    ah = 24
    aw = 14
    p1 = (x2, y2)
    p2 = (x2 - ah * math.cos(ang) + aw * math.sin(ang), y2 - ah * math.sin(ang) - aw * math.cos(ang))
    p3 = (x2 - ah * math.cos(ang) - aw * math.sin(ang), y2 - ah * math.sin(ang) + aw * math.cos(ang))
    d.polygon([p1, p2, p3], fill=color)


def sensor_box(x, y, w, h, title):
    d.rounded_rectangle((x, y, x + w, y + h), radius=18, fill=(250, 251, 255, 255), outline=(205, 213, 227, 255), width=2)
    d.text((x + 18, y + 18), title, font=small_font, fill=ink)


def actuator_icon(x, y, shape, title):
    if shape == "square":
        d.rounded_rectangle((x, y, x + 52, y + 52), radius=10, outline=(95, 103, 115, 255), fill=(244, 246, 249, 255), width=4)
    elif shape == "circle":
        d.ellipse((x, y, x + 52, y + 52), outline=(95, 103, 115, 255), fill=(244, 246, 249, 255), width=4)
    elif shape == "hex":
        pts = [(x + 26, y), (x + 48, y + 13), (x + 48, y + 39), (x + 26, y + 52), (x + 4, y + 39), (x + 4, y + 13)]
        d.polygon(pts, outline=(95, 103, 115, 255), fill=(244, 246, 249, 255), width=4)
    d.text((x - 6, y + 66), title, font=small_font, fill=muted)


draw_board(220, 430, 350, 200, "树莓派", "网页承载 / 摄像头 / 串口桥接", green)
draw_board(640, 300, 420, 230, "主板1（调度层）", "传感器汇聚 / 状态判定 / 任务编排", orange)
draw_board(1120, 430, 320, 200, "主板2（执行层）", "步进电机 / 舵机 / 继电器输出", blue)

sensor_box(610, 620, 170, 72, "除湿袋重力")
sensor_box(800, 620, 170, 72, "温湿度")
sensor_box(990, 620, 190, 72, "限位开关点位")

actuator_icon(1160, 690, "square", "步进电机")
actuator_icon(1240, 690, "circle", "舵机")
actuator_icon(1320, 690, "hex", "继电器")

arrow(570, 530, 640, 430, orange)
arrow(1060, 430, 1120, 530, orange)
arrow(726, 530, 700, 620, green)
arrow(915, 530, 885, 620, green)
arrow(1080, 530, 1085, 620, green)
arrow(1280, 630, 1280, 690, blue)

d.text((650, 264), "环境状态感知与点位约束判定", font=small_font, fill=orange)
d.text((1136, 286), "执行控制与动作落地", font=small_font, fill=blue)

img.alpha_composite(canvas)
img.save(OUT)
print(f"IMAGE_OK {OUT}")
