"""
Post-process aset mentah (assets/raw) menjadi aset siap pakai (assets/final).

  - enemies : bersihkan fringe warna di tepi alpha, crop ke isi, resize, color grade gelap
  - heroes  : crop 3:4 (fokus wajah di atas), resize, grade
  - bg      : crop 16:9, resize 1920x1080, gelapkan sesuai fungsi
  - icons   : bersihkan alpha, crop persegi, jadikan monokrom terang agar bisa di-tint di game

Jalankan (Python dengan Pillow + NumPy, mis. MSYS2 UCRT64):
    python tools/process_assets.py
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
RAW = ROOT / "assets" / "raw"
FINAL = ROOT / "assets" / "final"


# ----------------------------------------------------------------------------
# util
# ----------------------------------------------------------------------------

def to_np(img):
    return np.asarray(img.convert("RGBA")).astype(np.float32)


def to_img(arr):
    return Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGBA")


def clean_alpha(arr, cut=14):
    """Buang piksel hampir transparan dan ganti warna piksel tepi (fringe)
    dengan rata-rata warna piksel solid di sekitarnya."""
    a = arr[..., 3]
    a[a < cut] = 0
    solid = (a >= 245).astype(np.float32)
    rgb = arr[..., :3]

    def blur(ch, r):
        # box blur terpisah (horizontal lalu vertikal) dengan cumulative sum
        k = 2 * r + 1
        out = ch.astype(np.float64)
        for axis in (0, 1):
            pad = [(0, 0), (0, 0)]
            pad[axis] = (r + 1, r)
            c = np.cumsum(np.pad(out, pad, mode="edge"), axis=axis)
            if axis == 0:
                out = (c[k:] - c[:-k]) / k
            else:
                out = (c[:, k:] - c[:, :-k]) / k
        return out.astype(np.float32)

    w = blur(solid, 4)
    for c in range(3):
        avg = blur(rgb[..., c] * solid, 4) / np.maximum(w, 1e-4)
        edge = (a < 245) & (a > 0) & (w > 0.02)
        rgb[..., c] = np.where(edge, avg, rgb[..., c])
    # sedikit erosi alpha agar tepi tidak berpendar
    alpha_img = Image.fromarray(a.astype(np.uint8), "L").filter(ImageFilter.MinFilter(3))
    arr[..., 3] = np.minimum(a, np.asarray(alpha_img).astype(np.float32) * 0.5 + a * 0.5)
    return arr


def grade(arr, gain=0.85, gamma=1.1, warm=(1.03, 1.0, 0.94)):
    rgb = arr[..., :3] / 255.0
    rgb = np.power(np.clip(rgb * gain, 0, 1), gamma)
    rgb[..., 0] *= warm[0]
    rgb[..., 1] *= warm[1]
    rgb[..., 2] *= warm[2]
    arr[..., :3] = rgb * 255.0
    return arr


def trim(img, pad=10):
    bbox = img.getchannel("A").point(lambda v: 255 if v > 10 else 0).getbbox()
    if not bbox:
        return img
    x0, y0, x1, y1 = bbox
    x0, y0 = max(0, x0 - pad), max(0, y0 - pad)
    x1, y1 = min(img.width, x1 + pad), min(img.height, y1 + pad)
    return img.crop((x0, y0, x1, y1))


def fit_height(img, h):
    if img.height <= h:
        return img
    w = round(img.width * h / img.height)
    return img.resize((w, h), Image.LANCZOS)


def crop_aspect(img, aspect, top_bias=0.5):
    """Crop ke rasio lebar/tinggi `aspect`. top_bias 0 = ambil bagian atas."""
    w, h = img.size
    if w / h > aspect:
        nw = round(h * aspect)
        x = (w - nw) // 2
        return img.crop((x, 0, x + nw, h))
    nh = round(w / aspect)
    y = round((h - nh) * top_bias)
    return img.crop((0, y, w, y + nh))


# ----------------------------------------------------------------------------
# pemroses per kategori
# ----------------------------------------------------------------------------

def process_enemy(src, dst):
    arr = clean_alpha(to_np(Image.open(src)))
    arr = grade(arr, gain=0.86, gamma=1.12)
    img = trim(to_img(arr))
    img = fit_height(img, 900)
    img.save(dst, optimize=True)


def process_hero(src, dst):
    img = Image.open(src).convert("RGBA")
    img = crop_aspect(img, 3 / 4, top_bias=0.15)
    img = img.resize((480, 640), Image.LANCZOS)
    arr = grade(to_np(img), gain=1.08, gamma=0.96)
    arr[..., 3] = 255
    to_img(arr).save(dst, optimize=True)


BG_GAIN = {"battle_floor1": 0.72, "battle_floor2": 0.72, "battle_floor3": 0.70, "map": 0.62, "title": 0.85}


def process_bg(src, dst):
    img = Image.open(src).convert("RGBA")
    img = crop_aspect(img, 16 / 9, top_bias=0.5).resize((1920, 1080), Image.LANCZOS)
    arr = grade(to_np(img), gain=BG_GAIN.get(src.stem, 0.75), gamma=1.08, warm=(1.0, 1.0, 1.0))
    arr[..., 3] = 255
    to_img(arr).save(dst, optimize=True)


def process_icon(src, dst):
    arr = clean_alpha(to_np(Image.open(src)), cut=20)
    # monokrom terang: luminansi dinormalisasi, warna diberikan oleh tint di game
    lum = arr[..., 0] * 0.299 + arr[..., 1] * 0.587 + arr[..., 2] * 0.114
    solid = arr[..., 3] > 40
    hi = np.percentile(lum[solid], 95) if solid.any() else 255.0
    v = np.clip(lum / max(hi, 1.0), 0, 1) * 0.35 + 0.65
    for c in range(3):
        arr[..., c] = v * 255.0
    img = trim(to_img(arr), pad=6)
    side = max(img.size)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.paste(img, ((side - img.width) // 2, (side - img.height) // 2))
    sq.resize((160, 160), Image.LANCZOS).save(dst, optimize=True)


PROCESSORS = {"enemies": process_enemy, "heroes": process_hero, "bg": process_bg, "icons": process_icon}


def main():
    only = set(sys.argv[1:])
    done = 0
    for folder, fn in PROCESSORS.items():
        for src in sorted((RAW / folder).glob("*.png")):
            if only and src.stem not in only and folder not in only:
                continue
            dst = FINAL / folder / src.name
            dst.parent.mkdir(parents=True, exist_ok=True)
            fn(src, dst)
            with Image.open(dst) as im:
                print(f"{folder}/{src.name:24s} -> {im.size[0]}x{im.size[1]}")
            done += 1
    print(f"{done} aset diproses.")


if __name__ == "__main__":
    main()
