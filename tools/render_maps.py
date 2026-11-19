"""Render pratinjau seluruh peta (tanpa cahaya) ke docs/map_preview/*.png untuk diperiksa.

Hanya alat bantu pengembangan: membaca .tmj di assets/maps lalu menggambar layer tile + props
(+ penanda objek kecil). Jalankan dari root repo:
    /c/msys64/ucrt64/bin/python.exe tools/render_maps.py [skala]
"""
import json
import os
import sys

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAPS = os.path.join(ROOT, 'assets', 'maps')
OUT = os.path.join(ROOT, 'docs', 'map_preview')
COLORS = {'enemy': (230, 60, 60), 'loot': (250, 220, 90), 'npc': (90, 200, 250), 'tungku': (255, 140, 30),
          'door': (180, 120, 255), 'event': (120, 255, 140), 'spawn': (255, 255, 255)}


def render(path, scale):
    m = json.load(open(path, encoding='utf-8'))
    T, W, H = m['tilewidth'], m['width'], m['height']
    sheets = []
    for ts in m['tilesets']:
        img = Image.open(os.path.normpath(os.path.join(MAPS, ts['image']))).convert('RGBA')
        sheets.append((ts['firstgid'], ts['columns'], img))
    canvas = Image.new('RGBA', (W * T, H * T), (0, 0, 0, 255))
    props = []
    for L in m['layers']:
        if L['type'] == 'tilelayer' and L['name'] != 'collision':
            for i, gid in enumerate(L['data']):
                if not gid:
                    continue
                first, cols, img = max((s for s in sheets if s[0] <= gid), key=lambda s: s[0])
                k = gid - first
                tile = img.crop(((k % cols) * T, (k // cols) * T, (k % cols + 1) * T, (k // cols + 1) * T))
                canvas.alpha_composite(tile, ((i % W) * T, (i // W) * T))
        elif L['type'] == 'objectgroup':
            for o in L['objects']:
                pr = {p['name']: p['value'] for p in o.get('properties', [])}
                if o['type'] == 'prop':
                    props.append((o['y'] + o['height'], o, pr))
                else:
                    d = ImageDraw.Draw(canvas)
                    c = COLORS.get(o['type'], (200, 200, 200))
                    d.rectangle([o['x'] + 8, o['y'] + 8, o['x'] + max(o['width'], T) - 8, o['y'] + max(o['height'], T) - 8],
                                outline=c, width=3)
    for _, o, pr in sorted(props, key=lambda p: p[0]):
        img = Image.open(os.path.join(ROOT, pr['image'])).convert('RGBA')
        canvas.alpha_composite(img, (int(o['x']), int(o['y'])))
    if scale != 1:
        canvas = canvas.resize((int(W * T * scale), int(H * T * scale)), Image.NEAREST if scale >= 1 else Image.BILINEAR)
    return canvas


if __name__ == '__main__':
    scale = float(sys.argv[1]) if len(sys.argv) > 1 else 0.5
    os.makedirs(OUT, exist_ok=True)
    for f in sorted(os.listdir(MAPS)):
        if f.endswith('.tmj'):
            render(os.path.join(MAPS, f), scale).convert('RGB').save(os.path.join(OUT, f[:-4] + '.png'))
            print('ok', f)
