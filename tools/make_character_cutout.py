from pathlib import Path

import numpy as np
from PIL import Image

from collections import deque

source = Path(__file__).resolve().parents[1] / "app/src/main/res/drawable-nodpi/aethelgard_heroine_character.png"
image = Image.open(source).convert("RGB")
rgb = np.asarray(image, dtype=np.uint8)

# The generated file has a light checkerboard matte. Remove only near-neutral,
# bright pixels connected to the canvas boundary; white armor inside the figure
# is not connected to that boundary and is therefore preserved.
brightness = rgb.mean(axis=2)
chroma = rgb.max(axis=2).astype(np.int16) - rgb.min(axis=2).astype(np.int16)
background_candidate = (brightness >= 178.0) & (chroma <= 28)
height, width = background_candidate.shape
background = np.zeros((height, width), dtype=bool)
queue = deque()
for x in range(width):
    if background_candidate[0, x]: queue.append((0, x))
    if background_candidate[height - 1, x]: queue.append((height - 1, x))
for y in range(1, height - 1):
    if background_candidate[y, 0]: queue.append((y, 0))
    if background_candidate[y, width - 1]: queue.append((y, width - 1))
while queue:
    y, x = queue.pop()
    if background[y, x] or not background_candidate[y, x]:
        continue
    background[y, x] = True
    if y > 0: queue.append((y - 1, x))
    if y + 1 < height: queue.append((y + 1, x))
    if x > 0: queue.append((y, x - 1))
    if x + 1 < width: queue.append((y, x + 1))

rgba = np.dstack((rgb, np.full(rgb.shape[:2], 255, dtype=np.uint8)))
rgba[background, 3] = 0
Image.fromarray(rgba, mode="RGBA").save(source, optimize=True)
print({"path": str(source), "removed_background_pixels": int(background.sum()), "total_pixels": int(background.size)})
