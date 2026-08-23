import math
from pathlib import Path
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon, Ellipse, Circle, FancyBboxPatch
from matplotlib.lines import Line2D

# Canvas and isometric projection.
W, H = 1600, 1000
fig, ax = plt.subplots(figsize=(16, 10), dpi=160)
fig.patch.set_facecolor('#07141a')
ax.set_facecolor('#07141a')
ax.set_xlim(0, W)
ax.set_ylim(H, 0)
ax.axis('off')

# Majestic night sky: deterministic starfield, Milky Way haze, and moon glow.
import numpy as np
star_rng = np.random.default_rng(240823)
star_x = star_rng.uniform(55, W - 55, 190)
star_y = star_rng.uniform(120, 405, 190)
mask = ~((star_x > 370) & (star_x < 1230) & (star_y < 135))
star_x, star_y = star_x[mask], star_y[mask]
star_sizes = star_rng.choice([3, 4, 5, 7, 10, 14], size=len(star_x), p=[.33, .25, .18, .14, .07, .03])
star_colors = star_rng.choice(['#d9f5ff', '#fff1bd', '#b6ddff', '#f7fbff'], size=len(star_x))
ax.scatter(star_x, star_y, s=star_sizes, c=star_colors, alpha=.78, linewidths=0, zorder=-4)
mw_x = star_rng.uniform(35, W - 35, 170)
mw_y = 330 - 0.18 * mw_x + star_rng.normal(0, 30, 170)
keep = (mw_y > 135) & (mw_y < 415)
ax.scatter(mw_x[keep], mw_y[keep], s=star_rng.uniform(2, 9, keep.sum()), c='#9fd8e8', alpha=.18, linewidths=0, zorder=-5)
for radius, alpha in [(86, .025), (64, .045), (46, .09)]:
    ax.add_patch(Circle((175, 205), radius, facecolor='#b8e8f2', edgecolor='none', alpha=alpha, zorder=-3))
ax.add_patch(Circle((175, 205), 31, facecolor='#edf8df', edgecolor='#fff6c9', linewidth=1.2, alpha=.97, zorder=-2))
ax.add_patch(Circle((187, 196), 31, facecolor='#07141a', edgecolor='none', zorder=-1))

CX, CY = 800, 490
SX, SY, SZ = 5.9, 2.95, 10.5

def iso(x, y, z=0.0):
    return (CX + (x - y) * SX, CY + (x + y) * SY - z * SZ)

def poly(points, face, edge=None, lw=1.0, alpha=1.0, zorder=1):
    ax.add_patch(Polygon([iso(*p) for p in points], closed=True, facecolor=face,
                         edgecolor=edge if edge else face, linewidth=lw, alpha=alpha, zorder=zorder, joinstyle='round'))

def line(points, color, lw=2.0, alpha=1.0, ls='-', zorder=5):
    ax.plot([iso(*p)[0] for p in points], [iso(*p)[1] for p in points], color=color, linewidth=lw,
            alpha=alpha, linestyle=ls, solid_capstyle='round', zorder=zorder)

def label(x, y, text, color='#eef5e9', size=11, weight='bold', z=30, box=False):
    sx, sy = iso(x, y, 2.0)
    bbox = dict(boxstyle='round,pad=0.34', facecolor='#0b2025', edgecolor='#b8d7c0', linewidth=0.8, alpha=0.90) if box else None
    ax.text(sx, sy, text, color=color, fontsize=size, weight=weight, ha='center', va='center', zorder=z, bbox=bbox)

# Ocean base and land extrusion for a raised-island 3D read.
ocean = [(-13, -13, -2.0), (113, -13, -2.0), (113, 113, -2.0), (-13, 113, -2.0)]
poly(ocean, '#123b4a', edge='#2d7790', lw=2.0, alpha=0.98, zorder=0)
for yy in range(-5, 116, 8):
    line([(-10, yy, -1.85), (110, yy + 4, -1.85)], '#2b6d7d', lw=1.0, alpha=0.35, zorder=1)

# Raised world side walls: west, east, and near edge.
land_top = [(0, 0, 0.55), (100, 0, 0.55), (100, 100, 0.55), (0, 100, 0.55)]
poly([(0, 0, 0.55), (100, 0, 0.55), (100, 0, -1.75), (0, 0, -1.75)], '#183a36', edge='#3a6257', lw=1.2, zorder=2)
poly([(100, 0, 0.55), (100, 100, 0.55), (100, 100, -1.75), (100, 0, -1.75)], '#3d3325', edge='#876a3c', lw=1.2, zorder=2)
poly([(100, 100, 0.55), (0, 100, 0.55), (0, 100, -1.75), (100, 100, -1.75)], '#244a59', edge='#508697', lw=1.2, zorder=2)

# Biome slabs; boundaries are exact: Forest 0–33, Sand 34–67, Snow 68–100 km.
poly([(0, 0, 0.55), (34, 0, 0.55), (34, 100, 0.55), (0, 100, 0.55)], '#5f9d62', edge='#2f6146', lw=1.4, zorder=3)
poly([(34, 0, 0.57), (68, 0, 0.57), (68, 100, 0.57), (34, 100, 0.57)], '#d6a85b', edge='#8a6131', lw=1.4, zorder=3)
poly([(68, 0, 0.59), (100, 0, 0.59), (100, 100, 0.59), (68, 100, 0.59)], '#a5d6df', edge='#4a8599', lw=1.4, zorder=3)

# Organic terrain patches.
for pts, color in [
    ([(2, 8, .62), (16, 2, .62), (30, 10, .62), (24, 24, .62), (7, 20, .62)], '#77b567'),
    ([(1, 58, .62), (16, 48, .62), (31, 58, .62), (28, 82, .62), (8, 89, .62)], '#478c59'),
    ([(36, 6, .64), (50, 2, .64), (65, 13, .64), (57, 26, .64), (40, 24, .64)], '#e4bc72'),
    ([(37, 61, .64), (51, 48, .64), (66, 63, .64), (61, 91, .64), (40, 85, .64)], '#c99249'),
    ([(71, 7, .66), (87, 2, .66), (98, 16, .66), (89, 31, .66), (73, 27, .66)], '#c7eaf0'),
    ([(70, 53, .66), (86, 44, .66), (99, 58, .66), (94, 91, .66), (74, 84, .66)], '#84c1d2'),
]:
    poly(pts, color, edge=color, lw=0.5, alpha=0.65, zorder=4)

# Contour strokes add terrain depth.
for y0 in [18, 32, 46, 60, 74, 88]:
    line([(2, y0, .69), (30, y0 + 2.5, .69)], '#2f744d', lw=1.0, alpha=0.28, zorder=5)
    line([(37, y0, .70), (65, y0 + 1.8, .70)], '#9f6f37', lw=1.0, alpha=0.25, zorder=5)
    line([(71, y0, .72), (97, y0 + 2.0, .72)], '#4d8fa5', lw=1.0, alpha=0.25, zorder=5)

# River and main road.
river = [(31, -2, .85), (29, 14, .87), (30.5, 28, .88), (28.8, 43, .89), (30, 58, .90), (32, 74, .90), (31, 88, .90), (33, 103, .91)]
line(river, '#51c2df', lw=7.5, alpha=0.92, zorder=9)
line(river, '#b5eff4', lw=1.3, alpha=0.70, zorder=10)
road = [(10, 16, .94), (14, 34, .95), (26, 49, .96), (39, 48, .96), (53, 54, .97), (62, 50, .98), (74, 57, .99), (88, 83, 1.0)]
line(road, '#ffe5a2', lw=4.8, alpha=0.95, ls=(0, (7, 6)), zorder=12)

# Isometric water body helper.
def water(cx, cy, rx, ry, color='#49b9d1'):
    pts = []
    for i in range(40):
        t = 2 * math.pi * i / 40
        pts.append((cx + rx * math.cos(t), cy + ry * math.sin(t), 0.92))
    poly(pts, color, edge='#b6f0f1', lw=1.4, alpha=0.93, zorder=11)
    line([(cx-rx*.6, cy, .95), (cx+rx*.5, cy, .95)], '#b8f4f5', lw=1.1, alpha=.55, zorder=12)

water(14, 61, 4.7, 3.2)
water(55, 69, 5.8, 4.6, '#4ca99c')
water(77, 29, 3.5, 3.0, '#5dbbd7')

# Mountain helper with three-dimensional triangular sides.
def peak(cx, cy, radius, height, main, snowcap=False):
    base = [(cx-radius, cy-radius*.55, .82), (cx+radius, cy-radius*.55, .82), (cx+radius*.75, cy+radius*.55, .82), (cx-radius*.75, cy+radius*.55, .82)]
    apex = (cx, cy, .82 + height)
    poly([base[0], base[1], apex], main, edge='#4e7f8d', lw=.6, alpha=.96, zorder=13)
    poly([base[1], base[2], apex], '#77aebe' if not snowcap else '#b9e5ed', edge='#4e7f8d', lw=.6, alpha=.96, zorder=13)
    poly([base[2], base[3], apex], '#365f72' if not snowcap else '#86b9c9', edge='#4e7f8d', lw=.6, alpha=.96, zorder=13)
    poly([base[3], base[0], apex], '#477e88' if not snowcap else '#d9f5f8', edge='#4e7f8d', lw=.6, alpha=.96, zorder=13)
    if snowcap:
        poly([(cx, cy, .82+height), (cx-radius*.23, cy-radius*.18, .82+height*.54), (cx+radius*.22, cy-radius*.12, .82+height*.48)], '#f4ffff', edge='#d4f4f6', lw=.4, alpha=.96, zorder=15)

for p in [(10, 82, 6.5, 3.2, '#2c6b50', False), (23, 72, 5.0, 2.6, '#356f54', False), (45, 80, 6.5, 2.8, '#a8753e', False), (57, 17, 6.5, 2.5, '#b98042', False), (76, 82, 7.0, 8.0, '#78afc1', True), (88, 87, 9.5, 10.5, '#71a7b9', True), (94, 65, 6.0, 5.8, '#83b7c5', True)]:
    peak(*p)

# Forest tree markers.
def tree(x, y, s=1.0):
    poly([(x-.5*s, y-.5*s, .75), (x+.5*s, y-.5*s, .75), (x+.5*s, y+.5*s, .75), (x-.5*s, y+.5*s, .75)], '#5b3e26', edge='#2b241b', lw=.4, zorder=16)
    poly([(x-.9*s, y, 1.0), (x, y-1.4*s, 1.0), (x+.9*s, y, 1.0)], '#1b5a43', edge='#123a32', lw=.5, zorder=17)
    poly([(x-.75*s, y-.6*s, 1.5), (x, y-1.85*s, 1.5), (x+.75*s, y-.6*s, 1.5)], '#3b8854', edge='#1e573c', lw=.5, zorder=18)
    poly([(x-.55*s, y-1.0*s, 1.9), (x, y-2.15*s, 1.9), (x+.55*s, y-1.0*s, 1.9)], '#76b761', edge='#356b4e', lw=.5, zorder=19)

for tx, ty, ts in [(5, 24, 1.4), (21, 22, 1.2), (7, 52, 1.3), (19, 66, 1.1), (24, 86, 1.5), (30, 72, 1.0), (10, 76, 1.0)]:
    tree(tx, ty, ts)

# Settlement/building marker.
def house(x, y, c_body, c_roof, scale=1.0):
    poly([(x-2.5*scale, y-1.5*scale, .85), (x+2.5*scale, y-1.5*scale, .85), (x+2.5*scale, y+1.5*scale, .85), (x-2.5*scale, y+1.5*scale, .85)], c_body, edge='#38291c', lw=.8, zorder=20)
    poly([(x-3.0*scale, y-1.4*scale, 1.0), (x, y-4.0*scale, 1.0), (x+3.0*scale, y-1.4*scale, 1.0)], c_roof, edge='#38291c', lw=1.0, zorder=21)
    poly([(x-.55*scale, y+.3*scale, .86), (x+.55*scale, y+.3*scale, .86), (x+.55*scale, y+1.5*scale, .86), (x-.55*scale, y+1.5*scale, .86)], '#3b2b22', edge='#38291c', lw=.4, zorder=22)

house(14, 46, '#865e3c', '#c78b50', 1.3)
house(47, 48, '#b66f46', '#e2ad68', 1.8)
house(52, 42, '#c78450', '#f0c07d', 1.1)
# Farm beds.
for fx, fy in [(9, 38), (14, 38), (19, 38), (9, 43), (14, 43), (19, 43)]:
    poly([(fx-1.7, fy-1.0, .86), (fx+1.7, fy-1.0, .86), (fx+1.7, fy+1.0, .86), (fx-1.7, fy+1.0, .86)], '#315d38', edge='#25472f', lw=.35, zorder=15)
    line([(fx-1.0, fy, .94), (fx, fy-.5, .95), (fx+1.0, fy, .94)], '#d0e986', lw=1.0, alpha=.9, zorder=16)

# Waystones and key landmark markers.
def marker(x, y, color, size=95, diamond=False):
    sx, sy = iso(x, y, 1.2)
    ax.scatter([sx], [sy], s=size, marker='D' if diamond else 'o', color=color, edgecolor='#fff8dc', linewidth=1.4, zorder=28)

for pt in [(31, 50), (63, 51), (71, 55)]:
    marker(*pt, '#fff0a2', size=90, diamond=True)
for pt, color in [((10,16),'#f2d27f'), ((14,40),'#fff0ae'), ((26,78),'#80d89a'), ((40,48),'#f0a365'), ((47,28),'#ffd176'), ((55,69),'#a8f1b6'), ((74,57),'#8fe4ef'), ((84,59),'#68c9de'), ((88,83),'#c4f8ff')]:
    marker(*pt, color)

# Landmark labels, placed at readable heights.
for x, y, text, color in [(10,16,'Forest Camp','#f5e0a2'), (14,40,'Farming Village','#fff2bc'), (26,78,'Moss Cave','#9fe4b8'), (40,48,'Sand Gate','#ffd2a1'), (47,28,'Sun Kiln','#ffd987'), (55,69,'Oasis','#bdf6c4'), (74,57,'Frost Gate','#baf5fb'), (84,59,'Predator Basin','#9fe8f2'), (88,83,'Frostclaw Arena','#d8fbff')]:
    label(x, y, text, color=color, size=9.2, z=40, box=True)

# Region banners.
label(17, 8, 'FOREST  •  FARMS  •  PEOPLE', color='#d5f2a5', size=14, z=45, box=True)
label(51, 8, 'SAND  •  OASIS  •  TRADE', color='#ffe0a0', size=14, z=45, box=True)
label(84, 8, 'SNOW  •  PREDATORS  •  100 HP', color='#d9fbff', size=14, z=45, box=True)

# Title and explanatory callouts.
ax.text(W/2, 54, 'AETHELGARD: WILD HORIZONS', color='#f6dc99', fontsize=25, weight='bold', ha='center', va='center', zorder=60)
ax.text(W/2, 87, 'ORIGINAL 3D-STYLE WORLD MAP  •  100 × 100 KM  •  10,000 KM²', color='#d6e9df', fontsize=12, ha='center', va='center', zorder=60)
ax.text(146, 875, 'OPEN OCEAN', color='#75d8ef', fontsize=13, weight='bold', ha='center', zorder=55)
ax.text(1452, 392, 'N', color='#f6dc99', fontsize=17, weight='bold', ha='center', zorder=55)
ax.annotate('', xy=(1452, 412), xytext=(1452, 468), arrowprops=dict(arrowstyle='-|>', color='#f6dc99', lw=2.2), zorder=55)

# Coordinate callouts and scale bar.
for x, y, t in [(0, 0, '0,0 km'), (100, 0, '100,0 km'), (100, 100, '100,100 km'), (0, 100, '0,100 km')]:
    sx, sy = iso(x, y, -2.0)
    ax.text(sx, sy, t, color='#c8dbd3', fontsize=8.5, ha='center', va='center', zorder=60)
ax.plot([95, 210], [908, 908], color='#f2d38c', linewidth=4, zorder=60)
ax.text(152, 932, '10 km', color='#d7e7df', fontsize=9, ha='center', zorder=60)

# Legend.
legend = [
    Line2D([0], [0], marker='o', color='w', label='Settlement / landmark', markerfacecolor='#f2d27f', markeredgecolor='white', markersize=8),
    Line2D([0], [0], marker='D', color='w', label='Fast-travel stone', markerfacecolor='#fff0a2', markeredgecolor='#3e777e', markersize=7),
    Line2D([0], [0], color='#51c2df', lw=4, label='River and ponds'),
    Line2D([0], [0], color='#ffe5a2', lw=3, linestyle=(0, (7, 6)), label='Road route'),
    Line2D([0], [0], marker='^', color='w', label='Forest / snow peaks', markerfacecolor='#3b8854', markeredgecolor='#d9f5f8', markersize=8),
]
leg = ax.legend(handles=legend, loc='lower center', bbox_to_anchor=(0.5, 0.015), ncol=5, frameon=False, fontsize=8.5)
for txt in leg.get_texts():
    txt.set_color('#d8e6df')

fig.savefig('/home/ubuntu/aethelgard_world_map_3d_100x100km.png', facecolor=fig.get_facecolor(), bbox_inches='tight')
