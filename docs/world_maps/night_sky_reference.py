import math
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Polygon

W, H = 1600, 900
fig, ax = plt.subplots(figsize=(16, 9), dpi=160)
ax.set_xlim(0, W)
ax.set_ylim(H, 0)
ax.axis('off')
fig.patch.set_facecolor('#06111c')
ax.set_facecolor('#06111c')

# Deep-blue vertical night gradient.
for y in range(H):
    t = y / H
    c = (0.02 + .018*t, 0.065 + .055*t, 0.11 + .09*t)
    ax.axhline(y, color=c, linewidth=1, zorder=0)

rng = np.random.default_rng(240823)
# Broad Milky Way haze made from translucent diagonal clouds.
for _ in range(95):
    x = rng.uniform(-100, W + 100)
    y = 455 - .36 * x + rng.normal(0, 72)
    if 60 < y < 750:
        ax.scatter(x, y, s=rng.uniform(200, 1500), color=rng.choice(['#5a9db7', '#6c89c2', '#9db4d6']), alpha=rng.uniform(.008, .024), linewidths=0, zorder=1)

# Main stars plus a few intentional constellations.
xs = rng.uniform(30, W - 30, 330)
ys = rng.uniform(55, 720, 330)
sizes = rng.choice([2, 3, 4, 6, 9, 14], size=len(xs), p=[.34, .25, .18, .12, .07, .04])
colors = rng.choice(['#d9f4ff', '#fff1c0', '#c0dcff', '#ffffff'], size=len(xs))
ax.scatter(xs, ys, s=sizes, c=colors, alpha=.86, linewidths=0, zorder=3)
for pts in [[(360,210),(401,168),(448,215),(425,276),(378,262)], [(1115,178),(1158,145),(1205,181),(1172,230)]]:
    ax.plot([p[0] for p in pts], [p[1] for p in pts], color='#bce8f2', alpha=.45, linewidth=1.1, zorder=2)
    ax.scatter([p[0] for p in pts], [p[1] for p in pts], s=20, c='#fff2b8', edgecolors='#ffffff', linewidths=.6, zorder=4)

# Crescent moon and glow.
for r, a in [(125,.025),(96,.04),(71,.075)]:
    ax.add_patch(Circle((1270, 225), r, facecolor='#a9e2ef', edgecolor='none', alpha=a, zorder=2))
ax.add_patch(Circle((1270,225), 52, facecolor='#eaf7df', edgecolor='#fff3bd', linewidth=2, zorder=5))
ax.add_patch(Circle((1292,211), 52, facecolor='#071421', edgecolor='none', zorder=6))

# Fine aurora ribbons near the horizon.
for i, color in enumerate(['#6fe6c0', '#78b8ee', '#bd8ee9']):
    t = np.linspace(0, 1, 240)
    x = 0 + 1600*t
    y = 600 + 30*i + 45*np.sin(t*math.pi*2.2 + i*.8) + 18*np.sin(t*math.pi*7.4)
    ax.plot(x, y, color=color, linewidth=15-i*3, alpha=.045, zorder=4)
    ax.plot(x, y, color=color, linewidth=2.1, alpha=.17, zorder=5)

# Layered mountains and pines create a usable survival-RPG horizon read.
ax.add_patch(Polygon([(0,760),(170,610),(310,755),(500,565),(680,760),(860,585),(1040,760),(1210,535),(1420,760),(1600,620),(1600,900),(0,900)], facecolor='#102b3b', edgecolor='none', zorder=8))
ax.add_patch(Polygon([(0,815),(210,690),(390,815),(580,675),(820,815),(1030,680),(1260,815),(1450,700),(1600,800),(1600,900),(0,900)], facecolor='#0b202b', edgecolor='none', zorder=9))
for x, h in [(80,120),(155,170),(250,95),(350,145),(478,110),(640,175),(760,125),(900,155),(1040,105),(1170,180),(1310,110),(1450,165),(1530,110)]:
    base = 850
    ax.add_patch(Polygon([(x-46,base),(x,base-h),(x+46,base)], facecolor='#091a25', edgecolor='#21414d', linewidth=.8, zorder=10))
    ax.add_patch(Polygon([(x,base-h),(x+46,base),(x+17,base-12)], facecolor='#112c37', edgecolor='none', zorder=11))
# Foreground ridge.
ax.add_patch(Polygon([(0,835),(220,795),(420,825),(630,785),(820,830),(1020,790),(1210,825),(1400,780),(1600,815),(1600,900),(0,900)], facecolor='#07141c', edgecolor='#31535c', linewidth=1.1, zorder=12))

# A small warm campfire anchor makes the night readable as a game reference.
ax.scatter([520], [785], s=550, color='#f2a13f', alpha=.055, linewidths=0, zorder=13)
ax.scatter([520], [785], s=150, color='#ffcf63', alpha=.18, linewidths=0, zorder=14)
ax.plot([520, 510, 531, 520], [812, 782, 812, 812], color='#8b4f2d', linewidth=4, zorder=15)
ax.plot([520, 520], [805, 755], color='#ffbd54', linewidth=5, alpha=.85, zorder=16)
ax.plot([520, 505, 535, 520], [790, 767, 790, 790], color='#ffe9a3', linewidth=3, zorder=17)

ax.text(80, 105, 'AETHELGARD NIGHT SKY', color='#f7df9e', fontsize=27, weight='bold', ha='left', va='center', zorder=20)
ax.text(82, 145, 'Majestic celestial lighting reference • stars • moonlight • aurora • cool snow glow', color='#c9e3e4', fontsize=13, ha='left', va='center', zorder=20)
ax.text(80, 852, 'Night mood target: quiet, luminous, navigable, and safe for gameplay readability', color='#d6e8dd', fontsize=12, ha='left', va='center', zorder=20)

fig.savefig('/home/ubuntu/aethelgard_night_sky_reference.png', facecolor=fig.get_facecolor(), bbox_inches='tight')
