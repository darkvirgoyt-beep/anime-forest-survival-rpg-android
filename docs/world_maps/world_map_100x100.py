import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle, Polygon
from matplotlib.lines import Line2D

fig, ax = plt.subplots(figsize=(16, 10), dpi=160)
fig.patch.set_facecolor('#10191d')
ax.set_facecolor('#10191d')

# Three 100x100 logical world regions.
ax.add_patch(Rectangle((0, 0), 34, 100, facecolor='#7fbf75', edgecolor='#1b302b', linewidth=2))
ax.add_patch(Rectangle((34, 0), 34, 100, facecolor='#e4bd72', edgecolor='#503b24', linewidth=2))
ax.add_patch(Rectangle((68, 0), 32, 100, facecolor='#9fd7e6', edgecolor='#284f63', linewidth=2))

# Soft sub-regions and terrain shapes.
for x, y, w, h in [(4, 70, 12, 18), (18, 63, 10, 22), (7, 8, 18, 12), (20, 28, 11, 11)]:
    ax.add_patch(Polygon([(x, y), (x+w*0.55, y+h), (x+w, y+h*0.35), (x+w*0.72, y-h*0.05), (x, y)], closed=True, facecolor='#5d9e65', alpha=0.38, edgecolor='none'))
for x, y, w, h in [(37, 76, 22, 13), (48, 10, 16, 20), (36, 42, 13, 16)]:
    ax.add_patch(Polygon([(x, y), (x+w*0.45, y+h), (x+w, y+h*0.45), (x+w*0.65, y), (x, y)], closed=True, facecolor='#c9974d', alpha=0.35, edgecolor='none'))
for x, y, w, h in [(72, 70, 22, 19), (78, 10, 17, 18), (69, 42, 12, 18)]:
    ax.add_patch(Polygon([(x, y), (x+w*0.45, y+h), (x+w, y+h*0.35), (x+w*0.62, y), (x, y)], closed=True, facecolor='#78b8d1', alpha=0.36, edgecolor='none'))

# River and roads.
river_x = [31, 29.5, 30.5, 28.8, 30.0, 32.2, 31.0, 33.0]
river_y = [0, 14, 29, 43, 58, 73, 87, 100]
ax.plot(river_x, river_y, color='#4d9fc4', linewidth=5, alpha=0.82, solid_capstyle='round')
ax.plot([10, 14, 26, 39, 53, 62, 74, 90], [16, 34, 49, 48, 54, 50, 57, 83], color='#f2e1a8', linewidth=3.2, linestyle=(0, (7, 5)), alpha=0.95)
ax.plot([39, 53], [48, 54], color='#fff0b5', linewidth=1.2, alpha=0.85)

# Forest farms and village.
for x, y in [(9, 37), (13, 36), (17, 36), (9, 43), (13, 43), (17, 43)]:
    ax.add_patch(Rectangle((x-1.4, y-1.0), 2.8, 2.0, facecolor='#456f43', edgecolor='#2d5032', linewidth=0.8))
    ax.plot([x-1, x, x+1], [y+0.4, y+1.2, y+0.4], color='#d8ed92', linewidth=1)
# Forest house marker.
ax.add_patch(Rectangle((11, 46), 6, 4, facecolor='#815d3a', edgecolor='#2b2219', linewidth=1.5))
ax.add_patch(Polygon([(10.4, 50), (14, 54), (17.6, 50)], facecolor='#b77c45', edgecolor='#2b2219', linewidth=1.5))

# Sand settlement and oasis.
ax.add_patch(Rectangle((42, 43), 10, 7, facecolor='#bd7b46', edgecolor='#593b28', linewidth=1.5))
ax.add_patch(Polygon([(40.5, 50), (47, 55), (53.5, 50)], facecolor='#e6a863', edgecolor='#593b28', linewidth=1.5))
ax.add_patch(Circle((55, 69), 5.5, facecolor='#4f9f88', edgecolor='#275e57', linewidth=1.8))
for x in [51.5, 55, 58.5]:
    ax.plot([x, x+0.8], [75, 79], color='#4b6c37', linewidth=2)
    ax.add_patch(Circle((x+0.8, 79), 1.0, facecolor='#6ea759', edgecolor='none'))

# Snow peaks and boss basin.
for points in [((70, 78), (76, 96), (83, 78)), ((78, 75), (88, 98), (96, 75)), ((69, 53), (76, 69), (84, 53))]:
    ax.add_patch(Polygon(points, facecolor='#dff7fb', edgecolor='#679ab1', linewidth=1.2, alpha=0.95))
ax.add_patch(Circle((88, 83), 8.0, facecolor='#6d9fb7', edgecolor='#294f63', linewidth=2.2, alpha=0.88))
ax.add_patch(Circle((88, 83), 4.8, facecolor='#8fbfd0', edgecolor='#d9fbff', linewidth=1.5, alpha=0.95))
ax.add_patch(Circle((84, 59), 4.0, facecolor='#5e91a8', edgecolor='#294f63', linewidth=1.5))

# Landmark points.
landmarks = [
    (10, 16, '#243b2d', 'Forest Camp'),
    (14, 40, '#f1d58b', 'Farming Village'),
    (26, 78, '#2d5a47', 'Moss Cave'),
    (40, 48, '#75452f', 'Sand Gate'),
    (47, 28, '#75452f', 'Sun Kiln'),
    (55, 69, '#d6f0a3', 'Oasis'),
    (74, 57, '#285365', 'Frost Gate'),
    (84, 59, '#162c3b', 'Predator Basin'),
    (88, 83, '#92efff', 'Frostclaw Arena'),
]
for x, y, c, label in landmarks:
    ax.scatter([x], [y], s=125, color=c, edgecolor='#fffaf0', linewidth=1.6, zorder=6)
    ax.text(x+1.3, y+1.2, label, fontsize=9.5, color='#142024', weight='bold', zorder=7)

# Fast travel stones.
for x, y in [(31, 50), (63, 51), (71, 55)]:
    ax.scatter([x], [y], s=80, marker='D', color='#fff0a4', edgecolor='#2c5e65', linewidth=1.2, zorder=6)

# Region labels.
ax.text(17, 96, 'FOREST BIOME', ha='center', va='center', fontsize=18, color='#183b2a', weight='bold')
ax.text(51, 96, 'SAND BIOME', ha='center', va='center', fontsize=18, color='#5d3d20', weight='bold')
ax.text(84, 96, 'SNOW BIOME', ha='center', va='center', fontsize=18, color='#234c61', weight='bold')
ax.text(17, 92, 'people • farms • launch region', ha='center', fontsize=9.5, color='#244c35')
ax.text(51, 92, 'people • kiln • trade route', ha='center', fontsize=9.5, color='#6e4b27')
ax.text(84, 92, 'predators only • 100 HP threats', ha='center', fontsize=9.5, color='#2f5f75')

# Map frame, axes, title, legend.
ax.set_xlim(-3, 103)
ax.set_ylim(-7, 104)
ax.set_aspect('equal')
ax.set_xticks(range(0, 101, 10))
ax.set_yticks(range(0, 101, 10))
ax.grid(color='#22363b', alpha=0.18, linewidth=0.8)
ax.tick_params(colors='#b8c8c6', labelsize=9)
for spine in ax.spines.values():
    spine.set_color('#d8e6df')
    spine.set_linewidth(1.4)
ax.set_xlabel('World X (0–100 km)', color='#d8e6df', labelpad=12, fontsize=11)
ax.set_ylabel('World Y (0–100 km)', color='#d8e6df', labelpad=12, fontsize=11)
ax.set_title('AETHELGARD: WILD HORIZONS — 100 × 100 KM WORLD MAP', color='#f1d58b', fontsize=20, weight='bold', pad=18)

legend = [
    Line2D([0], [0], marker='o', color='w', label='Settlement / major landmark', markerfacecolor='#75452f', markeredgecolor='white', markersize=9),
    Line2D([0], [0], marker='D', color='w', label='Fast-travel stone', markerfacecolor='#fff0a4', markeredgecolor='#2c5e65', markersize=8),
    Line2D([0], [0], color='#4d9fc4', lw=4, label='River / water route'),
    Line2D([0], [0], color='#f2e1a8', lw=3, linestyle=(0, (7, 5)), label='Traversable road'),
]
leg = ax.legend(handles=legend, loc='lower center', bbox_to_anchor=(0.5, -0.16), ncol=4, frameon=False, fontsize=9)
for text in leg.get_texts():
    text.set_color('#d8e6df')

# North arrow and scale.
ax.annotate('N', xy=(99, 12), xytext=(99, 4), color='#f1d58b', fontsize=12, weight='bold', ha='center', arrowprops=dict(arrowstyle='-|>', color='#f1d58b', lw=2))
ax.plot([4, 14], [-3, -3], color='#f1d58b', linewidth=3)
ax.text(9, -5.5, '10 km', color='#d8e6df', fontsize=9, ha='center')

plt.tight_layout()
fig.savefig('/home/ubuntu/aethelgard_world_map_100x100.png', facecolor=fig.get_facecolor(), bbox_inches='tight')
