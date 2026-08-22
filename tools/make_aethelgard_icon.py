from PIL import Image, ImageDraw, ImageFilter, ImageFont
import math
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "assets" / "ui" / "aethelgard_game_icon.png"
SIZE = 1024
img = Image.new("RGB", (SIZE, SIZE), "#071d27")
d = ImageDraw.Draw(img)

# Dawn gradient sky.
for y in range(SIZE):
    t = y / SIZE
    r = int(7 + 28 * t)
    g = int(29 + 52 * t)
    b = int(39 + 45 * t)
    d.line((0, y, SIZE, y), fill=(r, g, b))

# Sun halo.
for radius in range(260, 18, -8):
    alpha = (260 - radius) / 242
    color = (int(210 + 35 * alpha), int(140 + 70 * alpha), int(55 + 80 * alpha))
    d.ellipse((720-radius, 180-radius, 720+radius, 180+radius), fill=color)

d.polygon([(0, 540), (170, 380), (330, 520), (520, 300), (780, 540), (1024, 350), (1024, 1024), (0, 1024)], fill="#123844")
d.polygon([(0, 630), (210, 490), (420, 650), (650, 440), (1024, 610), (1024, 1024), (0, 1024)], fill="#0c2c35")

# Layered forest silhouettes.
for base, color, count, spread in [(740, "#123c35", 14, 74), (830, "#0b2c2d", 18, 60), (930, "#061d25", 23, 48)]:
    for i in range(count):
        x = int((i + 0.35) * SIZE / count)
        h = spread + ((i * 37) % 90)
        d.rectangle((x - 8, base - h // 3, x + 8, base), fill=color)
        d.polygon([(x, base-h), (x-h//2, base-h//3), (x+h//2, base-h//3)], fill=color)
        d.polygon([(x, base-h*2//3), (x-h*2//5, base-h//8), (x+h*2//5, base-h//8)], fill=color)

# Hero silhouette with amber scarf and teal bow glow.
cx, cy = 490, 590
d.ellipse((cx-92, cy-210, cx+92, cy-26), fill="#f0bb9e", outline="#ffe0bc", width=8)
d.polygon([(cx-120, cy-165), (cx-22, cy-245), (cx+105, cy-180), (cx+65, cy-72), (cx-100, cy-82)], fill="#1a1024")
d.polygon([(cx-112, cy-6), (cx+105, cy-6), (cx+145, cy+250), (cx-150, cy+250)], fill="#8b2b4e", outline="#f4c25f", width=8)
d.polygon([(cx-50, cy+220), (cx-25, cy+410), (cx-106, cy+410), (cx-125, cy+230)], fill="#17253d")
d.polygon([(cx+35, cy+220), (cx+105, cy+410), (cx+25, cy+410), (cx-12, cy+225)], fill="#17253d")
# Scarf.
d.polygon([(cx-80, cy-40), (cx+95, cy-55), (cx+150, cy+35), (cx+15, cy+20)], fill="#d78e45")
# Bow energy arc.
for w, color in [(28, "#1af0e5"), (12, "#b8fff5")]:
    d.arc((cx+105, cy-160, cx+355, cy+230), 105, 260, fill=color, width=w)
d.line((cx+115, cy-105, cx+235, cy+120), fill="#70fff5", width=8)
# Companion fox.
fx, fy = 735, 710
d.ellipse((fx-94, fy-44, fx+94, fy+78), fill="#e8dfc9", outline="#72e6dd", width=6)
d.polygon([(fx-80, fy-15), (fx-138, fy-110), (fx-28, fy-65)], fill="#d6c6ad")
d.polygon([(fx+70, fy-20), (fx+134, fy-112), (fx+28, fy-65)], fill="#d6c6ad")
d.ellipse((fx-35, fy-5, fx-17, fy+13), fill="#e7a83f")
d.ellipse((fx+17, fy-5, fx+35, fy+13), fill="#e7a83f")
d.polygon([(fx-16, fy-78), (fx, fy-135), (fx+22, fy-75)], fill="#3ef6eb")

# Brand plate.
d.rounded_rectangle((44, 52, 980, 178), radius=28, fill="#07141d", outline="#e7b554", width=5)
try:
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf", 54)
    small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)
except OSError:
    font = ImageFont.load_default(); small = font
text = "AETHELGARD"
box = d.textbbox((0, 0), text, font=font)
d.text(((SIZE-(box[2]-box[0]))/2, 70), text, font=font, fill="#f7d27a")
d.text((SIZE/2, 142), "WILD HORIZONS  •  CRAFTING", font=small, fill="#8fe8de", anchor="mm")

img = img.filter(ImageFilter.UnsharpMask(radius=2, percent=130, threshold=3))
OUT.parent.mkdir(parents=True, exist_ok=True)
img.save(OUT, "PNG", optimize=True)
print(OUT)
