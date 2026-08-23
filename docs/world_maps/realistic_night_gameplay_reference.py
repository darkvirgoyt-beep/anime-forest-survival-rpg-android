from PIL import Image, ImageDraw, ImageFilter
import numpy as np, math

W, H = 1600, 900
rng = np.random.default_rng(240823)

# Deep twilight-to-midnight sky gradient.
yy = np.arange(H)[:, None]
t = yy / H
stops = np.array([[5, 13, 26], [7, 24, 43], [11, 39, 56], [8, 22, 29]], dtype=float)
idx = np.clip(t * 3.0, 0, 2.999)
lo = np.floor(idx).astype(int)
hi = np.minimum(lo + 1, 3)
frac = idx - lo
rgb = stops[lo] * (1 - frac)[:, :, None] + stops[hi] * frac[:, :, None]
arr = np.repeat(rgb, W, axis=1).astype(np.uint8)
img = Image.fromarray(arr, 'RGB').convert('RGBA')

def layer():
    return Image.new('RGBA', (W, H), (0, 0, 0, 0))

# Soft Milky Way haze.
haze = layer(); hd = ImageDraw.Draw(haze)
for i in range(90):
    x = int(rng.uniform(-200, W + 200))
    y = int(420 - .24 * x + rng.normal(0, 70))
    r = int(rng.uniform(30, 130))
    c = (86, 142, 181, int(rng.uniform(3, 13)))
    hd.ellipse((x-r*2, y-r, x+r*2, y+r), fill=c)
haze = haze.filter(ImageFilter.GaussianBlur(48))
img = Image.alpha_composite(img, haze)

# Stars with depth-scaled glow.
stars = layer(); sd = ImageDraw.Draw(stars)
for _ in range(360):
    x = int(rng.integers(20, W-20)); y = int(rng.integers(40, 610))
    if 570 < x < 1120 and y < 120: continue
    r = float(rng.choice([.7, .9, 1.1, 1.4, 1.8, 2.6], p=[.24,.25,.22,.16,.09,.04]))
    col = tuple(rng.choice([(221,241,255), (255,237,187), (184,215,255), (242,248,255)]))
    sd.ellipse((x-r, y-r, x+r, y+r), fill=col+(int(rng.uniform(135, 235)),))
    if r > 2:
        sd.ellipse((x-r*3, y-r*3, x+r*3, y+r*3), fill=col+(25,))
img = Image.alpha_composite(img, stars)

# Crescent moon with layered blue-white glow.
moon = layer(); md = ImageDraw.Draw(moon)
for rad, alpha in [(140, 8), (105, 14), (75, 25)]:
    md.ellipse((1230-rad, 170-rad, 1230+rad, 170+rad), fill=(164, 224, 239, alpha))
moon = moon.filter(ImageFilter.GaussianBlur(18))
img = Image.alpha_composite(img, moon)
moon = layer(); md = ImageDraw.Draw(moon)
md.ellipse((1182, 122, 1278, 218), fill=(239, 248, 218, 250), outline=(255, 245, 191, 255), width=2)
md.ellipse((1210, 108, 1302, 204), fill=(7, 22, 39, 255))
img = Image.alpha_composite(img, moon)

# Fine aurora ribbons.
aur = layer(); ad = ImageDraw.Draw(aur)
for k, col in enumerate([(83, 224, 186, 45), (112, 155, 235, 37), (186, 126, 223, 25)]):
    pts = []
    for x in range(-60, W+70, 12):
        y = 570 + k*42 + 34*math.sin(x/170 + k*.6) + 12*math.sin(x/47)
        pts.append((x, int(y)))
    ad.line(pts, fill=col, width=18-k*4)
    ad.line(pts, fill=tuple(list(col[:3]) + [min(120, col[3]*3)]), width=2)
aur = aur.filter(ImageFilter.GaussianBlur(8))
img = Image.alpha_composite(img, aur)

# Distant atmospheric mountain ranges.
terrain = layer(); td = ImageDraw.Draw(terrain)
td.polygon([(0, 700), (150, 575), (315, 685), (515, 515), (710, 680), (900, 548), (1090, 687), (1270, 500), (1480, 660), (1600, 560), (1600, 900), (0, 900)], fill=(14, 43, 58, 245))
td.polygon([(0, 780), (185, 655), (380, 760), (590, 625), (790, 770), (1015, 620), (1230, 765), (1435, 650), (1600, 730), (1600, 900), (0, 900)], fill=(8, 28, 39, 255))
# Snow highlights on the far peaks.
td.polygon([(1270,500),(1216,596),(1244,574),(1270,540),(1296,580),(1335,615)], fill=(165, 208, 217, 90))
td.polygon([(515,515),(468,582),(500,559),(515,540),(535,568),(565,600)], fill=(125, 178, 192, 65))
img = Image.alpha_composite(img, terrain)

# Ground: dark grass and subtle cool moonlight texture.
ground = layer(); gd = ImageDraw.Draw(ground)
gd.polygon([(0, 745), (1600, 745), (1600, 900), (0, 900)], fill=(5, 18, 20, 255))
for _ in range(1200):
    x = int(rng.integers(0, W)); y = int(rng.integers(745, 900))
    length = int(rng.integers(3, 15))
    col = rng.choice([(27, 54, 46, 95), (39, 70, 54, 75), (74, 105, 76, 45)])
    gd.line((x, y, x + int(rng.normal(0, 2)), y-length), fill=tuple(col), width=1)
img = Image.alpha_composite(img, ground)

# Dark pines, with soft blue rim on the far side.
trees = layer(); tr = ImageDraw.Draw(trees)
def pine(x, base, h, width, foreground=False):
    trunk = (34, 26, 20, 255) if foreground else (11, 35, 37, 255)
    tr.rectangle((x-5, base-h*.22, x+5, base), fill=trunk)
    for frac, mult in [(0.12, .42), (.28, .55), (.45, .68), (.62, .80), (.80, .95)]:
        cy = base - h*frac
        half = width * (frac + .15) / 2
        fill = (8, 26, 28, 255) if foreground else (17, 49, 52, 255)
        tr.polygon([(x, base-h*(frac+.23)), (x-half, cy+h*.10), (x+half, cy+h*.10)], fill=fill)
        if not foreground:
            tr.line([(x, base-h*(frac+.23)), (x-half*.86, cy+h*.09)], fill=(64, 112, 116, 80), width=2)
for x, base, h, w in [(55,790,185,95),(155,780,145,82),(285,805,210,120),(430,790,170,92),(690,805,155,84),(820,790,205,115),(1040,800,165,92),(1160,790,215,120),(1390,795,175,98),(1510,805,205,115)]:
    pine(x, base, h, w, False)
for x, base, h, w in [(15,888,220,120),(125,888,145,82),(260,888,245,128),(460,888,190,108),(770,888,250,132),(1085,888,195,108),(1375,888,235,124),(1550,888,180,98)]:
    pine(x, base, h, w, True)
img = Image.alpha_composite(img, trees)

# Campfire radial orange lighting on ground and nearby foliage.
light = layer(); ld = ImageDraw.Draw(light)
for r, a in [(245, 5), (190, 8), (140, 14), (100, 22), (66, 36)]:
    ld.ellipse((520-r, 778-r*.55, 520+r, 778+r*.55), fill=(255, 130, 35, a))
light = light.filter(ImageFilter.GaussianBlur(26))
img = Image.alpha_composite(img, light)

# Warm-lit rocks and firewood.
props = layer(); pd = ImageDraw.Draw(props)
pd.ellipse((408, 822, 636, 865), fill=(19, 22, 19, 255), outline=(89, 68, 44, 180), width=3)
for x, y in [(456,829),(505,837),(556,832),(596,840)]:
    pd.ellipse((x-25,y-12,x+25,y+12), fill=(73,57,44,255), outline=(175,104,54,170), width=2)
pd.line((482, 825, 565, 855), fill=(105, 56, 31, 255), width=13)
pd.line((565, 825, 482, 855), fill=(129, 66, 32, 255), width=11)
img = Image.alpha_composite(img, props)

# Fire flame: layered translucent glow, blue base, orange body, pale core.
fire = layer(); fd = ImageDraw.Draw(fire)
for r, a in [(95, 20), (62, 38), (39, 60)]:
    fd.ellipse((520-r, 780-r, 520+r, 780+r), fill=(255, 122, 31, a))
fd.polygon([(520, 828), (480, 795), (496, 748), (515, 775), (540, 705), (550, 775), (574, 742), (572, 800), (544, 830)], fill=(238, 78, 24, 240))
fd.polygon([(520, 823), (499, 790), (515, 755), (526, 779), (543, 733), (550, 793), (540, 823)], fill=(255, 170, 46, 255))
fd.polygon([(520, 819), (511, 793), (522, 770), (534, 797), (531, 819)], fill=(255, 241, 156, 255))
# Sparks.
for x, y, r in [(493,731,3),(554,703,3),(577,747,2),(477,770,2),(548,681,2),(515,690,2)]:
    fd.ellipse((x-r,y-r,x+r,y+r), fill=(255,196,87,230))
fire = fire.filter(ImageFilter.GaussianBlur(6))
img = Image.alpha_composite(img, fire)

# Smoke column, broken into soft translucent puffs.
smoke = layer(); sm = ImageDraw.Draw(smoke)
for x, y, r, a in [(524,660,24,34),(548,608,30,27),(520,562,37,21),(560,516,43,16),(538,470,52,10)]:
    sm.ellipse((x-r, y-r, x+r, y+r), fill=(155, 180, 177, a))
smoke = smoke.filter(ImageFilter.GaussianBlur(22))
img = Image.alpha_composite(img, smoke)

# Adult adventurer silhouette: practical coat, boots, tied hair, sword; no sexualized framing.
hero = layer(); hd = ImageDraw.Draw(hero)
# moonlit rim edge behind the figure
rim = (113, 177, 187, 150)
hd.ellipse((1007, 544, 1069, 606), fill=rim)
hd.polygon([(1016,601),(1061,601),(1088,720),(1097,799),(980,799),(993,720)], fill=(23, 48, 56, 255), outline=(87, 150, 159, 230))
hd.polygon([(1018,610),(1058,610),(1047,700),(1028,700)], fill=(40, 76, 82, 255))
hd.polygon([(1005,792),(1036,792),(1025,862),(986,862)], fill=(13, 27, 31, 255))
hd.polygon([(1060,792),(1090,792),(1126,859),(1087,859)], fill=(13, 27, 31, 255))
hd.ellipse((1018,548,1062,595), fill=(28, 39, 43, 255), outline=(184, 205, 192, 190), width=2)
hd.polygon([(1018,558),(1005,566),(1025,530),(1058,544),(1072,590),(1060,585),(1046,567)], fill=(39, 28, 31, 255))
# tied hair strand
hd.line((1018,565,982,614), fill=(33,25,28,255), width=12)
# arm, gauntlet, sword
hd.line((1058,641,1118,702), fill=(27, 43, 46, 255), width=18)
hd.line((1118,702,1192,622), fill=(184, 208, 200, 190), width=5)
hd.line((1118,702,1198,616), fill=(75, 105, 105, 210), width=2)
# fire-rim accents
hd.line((1002,610,991,720), fill=(235, 128, 55, 135), width=3)
hd.line((988,795,1024,795), fill=(239, 136, 58, 120), width=3)
img = Image.alpha_composite(img, hero)

# Foreground vignette for cinematic realism and safe focal hierarchy.
vignette = Image.new('L', (W,H), 0); vd = ImageDraw.Draw(vignette)
vd.ellipse((-150,-80,W+150,H+150), fill=210)
vignette = vignette.filter(ImageFilter.GaussianBlur(170))
black = Image.new('RGBA', (W,H), (0,0,0,0)); black.putalpha(Image.eval(vignette, lambda p: 210-p))
img = Image.alpha_composite(img, black)

# Minimal non-intrusive gameplay cues: compass and cycle indicator.
over = layer(); od = ImageDraw.Draw(over)
od.rounded_rectangle((60, 58, 318, 122), radius=18, fill=(4, 15, 24, 170), outline=(186, 219, 207, 100), width=2)
od.text((84, 77), 'NIGHT  •  DAY 03', fill=(231, 241, 218, 235))
od.text((84, 100), 'Campfire warmth active', fill=(238, 182, 99, 230))
od.line((1315, 825, 1505, 825), fill=(224, 235, 222, 170), width=2)
od.text((1382, 840), 'N', fill=(244, 223, 157, 235))
img = Image.alpha_composite(img, over)

img.convert('RGB').save('/home/ubuntu/aethelgard_realistic_night_gameplay_reference.png', quality=95)
