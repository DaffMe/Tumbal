"""Autotile gaya RPG Maker XP (96x128) -> lembar 47 tile "blob" untuk Tiled.

Format sumber RMXP (tile 32px, dipecah jadi mini-tile 16px):
  - kiri-atas 32x32      : ikon tunggal (tidak dipakai)
  - kanan-atas (64,0)    : 4 sudut-dalam (inner corner), tiap kuadran 16px
  - bawah 96x96 (y=32..) : blok 3x3 -> sudut luar, tepi, tengah

Setiap tile hasil disusun dari 4 kuadran (TL, TR, BL, BR). Kuadran ditentukan
oleh tetangga horizontal (h), vertikal (v), dan diagonal (d) di sisi kuadran:
  !h & !v -> sudut luar   | h & !v -> tepi atas/bawah | !h & v -> tepi kiri/kanan
   h &  v & d -> tengah   | h & v & !d -> sudut dalam

`mask` 8-bit: N=1, NE=2, E=4, SE=8, S=16, SW=32, W=64, NW=128 (1 = tetangga sama).
Diagonal hanya berarti bila kedua ortogonalnya juga sama -> 47 kombinasi unik.
"""
from PIL import Image

N, NE, E, SE, S, SW, W, NW = 1, 2, 4, 8, 16, 32, 64, 128


def canonical(mask):
    m = mask & (N | E | S | W)
    if (mask & NE) and (mask & N) and (mask & E): m |= NE
    if (mask & SE) and (mask & S) and (mask & E): m |= SE
    if (mask & SW) and (mask & S) and (mask & W): m |= SW
    if (mask & NW) and (mask & N) and (mask & W): m |= NW
    return m


# Daftar 47 mask kanonik, urutan tetap -> indeks tile di lembar hasil.
MASKS = sorted({canonical(m) for m in range(256)})
assert len(MASKS) == 47, len(MASKS)
INDEX = {m: i for i, m in enumerate(MASKS)}
COLS = 8


def index_for(mask):
    return INDEX[canonical(mask)]


def _quarter_src(qx, qy, h, v, d):
    """Koordinat piksel (x, y) mini-tile 16px sumber untuk satu kuadran."""
    if h and v and not d:                       # sudut dalam
        return 64 + qx * 16, qy * 16
    if not h and not v:
        cx, cy = (0 if qx == 0 else 2), (0 if qy == 0 else 2)
    elif h and not v:
        cx, cy = 1, (0 if qy == 0 else 2)
    elif not h and v:
        cx, cy = (0 if qx == 0 else 2), 1
    else:
        cx, cy = 1, 1
    return cx * 32 + qx * 16, 32 + cy * 32 + qy * 16


def compose_tile(src, mask, frame_x=0):
    t = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    for qy in (0, 1):
        for qx in (0, 1):
            h = bool(mask & (W if qx == 0 else E))
            v = bool(mask & (N if qy == 0 else S))
            if qx == 0 and qy == 0: d = bool(mask & NW)
            elif qx == 1 and qy == 0: d = bool(mask & NE)
            elif qx == 0: d = bool(mask & SW)
            else: d = bool(mask & SE)
            sx, sy = _quarter_src(qx, qy, h, v, d)
            t.paste(src.crop((frame_x + sx, sy, frame_x + sx + 16, sy + 16)), (qx * 16, qy * 16))
    return t


def bake(src_path, out_path, frames=1):
    """Lembar hasil: 8 kolom x 6 baris per frame; frame animasi disusun ke bawah."""
    src = Image.open(src_path).convert('RGBA')
    rows = (len(MASKS) + COLS - 1) // COLS
    out = Image.new('RGBA', (COLS * 32, rows * 32 * frames), (0, 0, 0, 0))
    for f in range(frames):
        for i, m in enumerate(MASKS):
            t = compose_tile(src, m, frame_x=f * 96)
            out.paste(t, ((i % COLS) * 32, (f * rows + i // COLS) * 32))
    out.save(out_path)
    return rows * COLS   # jumlah tile per frame (termasuk sisa kosong)
