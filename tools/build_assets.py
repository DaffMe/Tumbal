"""Bangun aset game dari assets/src -> assets/tiles, assets/chars.

Jalankan dari root repo:
    /c/msys64/ucrt64/bin/python.exe tools/build_assets.py

- Karakter dirakit dari potongan Aurora (CC-BY 4.0, Sevarihk) dengan pewarnaan
  (multiply). Hasil: lembar 256x296 = 4 kolom (frame jalan) x 4 baris
  (hadap bawah, kiri, kanan, atas), tiap frame 64x74.
- Autotile RMXP dibakar jadi lembar 47 tile (lihat tools/autotile.py).
"""
import os
import shutil
import sys

from PIL import Image, ImageChops

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import autotile  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'assets', 'src')
AUR = os.path.join(SRC, 'aurora')
SEV = os.path.join(SRC, 'sevarihk')
TILES = os.path.join(ROOT, 'assets', 'tiles')
CHARS = os.path.join(ROOT, 'assets', 'chars')


def tint(name, rgb):
    im = Image.open(os.path.join(AUR, name)).convert('RGBA')
    a = im.split()[3]
    col = Image.new('RGB', im.size, rgb)
    m = ImageChops.multiply(im.convert('RGB'), col).convert('RGBA')
    m.putalpha(a)
    return m


def character(out_name, layers):
    base = None
    for name, rgb in layers:
        t = tint(name, rgb)
        base = t if base is None else Image.alpha_composite(base, t)
    base.save(os.path.join(CHARS, out_name))


# ---------------------------------------------------------------- palet
KULIT_SAWO = (190, 138, 96)
KULIT_PUCAT = (176, 150, 128)
RAMBUT_HITAM = (34, 28, 26)
KAIN_KUSAM = (150, 138, 116)
KAIN_COKLAT = (104, 84, 60)
KAIN_GELAP = (64, 56, 48)
KAIN_LURIK = (88, 70, 52)
KAIN_KUNING = (176, 140, 60)


def build_characters():
    os.makedirs(CHARS, exist_ok=True)
    body = lambda skin: [('human_legs!nl.png', skin), ('human_torso!nt.png', skin),
                         ('human_arms!na.png', skin), ('human_head!nh.png', skin),
                         ('human_ears!ne.png', skin)]
    # Tokoh utama: tumbal kurus, cawat dan baju karung kusam, rambut kusut.
    character('tumbal.png', body(KULIT_SAWO) + [
        ('loincloth+nl+nt.png', KAIN_COKLAT),
        ('medieval_wide_shirt+nt+na+sb.png', KAIN_KUSAM),
        ('rogue_hair.png', RAMBUT_HITAM),
    ])
    # Warga desa (sementara, untuk uji NPC)
    character('warga_lelaki.png', body(KULIT_SAWO) + [
        ('plain_pants+nl-tt.png', KAIN_GELAP),
        ('medievalshirt+nt+nl+na+0b.png', KAIN_LURIK),
        ('bandana.png', KAIN_COKLAT),
    ])
    # Lelaki Tua: tumbal lama yang dibiarkan membusuk di sel
    character('lelaki_tua.png', body((150, 128, 104)) + [
        ('loincloth+nl+nt.png', (84, 70, 52)),
        ('full_beard.png', (190, 186, 176)),
        ('short_receding.png', (190, 186, 176)),
    ])
    # Timun Mas: gadis berkulit kuning keemasan, kain lurik kusam, rambut panjang
    character('timun_mas.png', body((206, 168, 96)) + [
        ('long_skirt+nl-tt.png', (110, 90, 52)),
        ('medieval_blouse+nt+na+sb.png', (150, 120, 60)),
        ('long_hair1.png', (30, 26, 24)),
    ])
    # Musuh di peta (sprite simbol)
    KULIT_MAYAT = (128, 138, 112)
    character('tumbal_gagal.png', body(KULIT_MAYAT) + [
        ('loincloth+nl+nt.png', (70, 60, 46)),
        ('short_receding.png', (40, 40, 36)),
    ])
    thick = lambda skin: [('human_legs_thick!tl.png', skin), ('human_torso_thick!tt.png', skin),
                          ('human_arms_beefy!ba.png', skin), ('human_head!nh.png', skin),
                          ('human_ears!ne.png', skin)]
    character('penjaga_lumbung.png', thick((150, 150, 132)) + [
        ('great_kilt+tt+ba-sl.png', (60, 40, 36)),
        ('shoulder_pads+ba.png', (90, 70, 50)),
        ('nordic_helmet.png', (120, 96, 72)),
    ])
    shutil.copy(os.path.join(SEV, 'rat-charset-with-shadow.png'), os.path.join(CHARS, 'tikus.png'))
    character('mbok_srini.png', body(KULIT_PUCAT) + [
        ('long_skirt+nl-tt.png', KAIN_LURIK),
        ('medieval_blouse+nt+na+sb.png', KAIN_GELAP),
        ('victorian_knot.png', (90, 86, 84)),
    ])


def _tint_water(im, mul=(0.75, 0.9, 0.82), desat=0.5):
    from PIL import ImageEnhance
    a = im.split()[3]
    rgb = ImageEnhance.Color(im.convert('RGB')).enhance(1.0 - desat)
    r, g, b = rgb.split()
    r = r.point(lambda v: min(255, int(v * mul[0])))
    g = g.point(lambda v: min(255, int(v * mul[1])))
    b = b.point(lambda v: min(255, int(v * mul[2])))
    out = Image.merge('RGB', (r, g, b)).convert('RGBA')
    out.putalpha(a)
    return out


def build_tiles():
    os.makedirs(TILES, exist_ok=True)
    # tileset objek dipakai apa adanya
    for f in ['evildungeon_0.png', 'dead-swamp-v4.png', 'bamboo.png', 'batak_house_0.png',
              'mini-fishers-hut.png', 'tree-trunk-32x32.png', 'tree-trunk-32x32swamp.png',
              'spikes_2.png']:
        shutil.copy(os.path.join(SEV, f), os.path.join(TILES, f))
    # autotile -> lembar 47 tile
    autos = {
        'auto_tembok_bata.png': ('autotile_purple_brick.png', 1),
        'auto_rumput_hutan.png': ('jungle-foliage-ground1.png', 1),
        'auto_rumput_hutan2.png': ('jungle-foliage-ground2.png', 1),
        'auto_semak.png': ('leafs.png', 1),
        'auto_semak2.png': ('leafs2.png', 1),
        'auto_rumput_liar.png': ('dschungel-gras.png', 1),
        'auto_tanah.png': ('dschungel-gras-b.png', 1),
        'auto_bambu_atas.png': ('bamboo-top.png', 1),
        'auto_air_rawa.png': ('swamp-bruhe_animated-v5.png', 4),
    }
    # laut garam: air rawa beranimasi diwarnai biru-kelabu pucat (air mati, asin)
    sea = _tint_water(Image.open(os.path.join(SEV, 'swamp-bruhe_animated-v5.png')).convert('RGBA'), (0.7, 1.05, 1.2), 0.2)
    tmp = os.path.join(TILES, '_laut.png')
    sea.save(tmp)
    autotile.bake(tmp, os.path.join(TILES, 'auto_air_laut.png'), 4)
    os.remove(tmp)
    # pasir garam: tekstur pasir dari pratinjau Sevarihk, diputihkan seperti kerak garam
    sand = Image.open(os.path.join(SEV, 'water-tiles.png')).convert('RGBA').crop((20, 20, 52, 52))
    sand = _tint_water(sand, (1.05, 1.05, 1.08), 0.35)
    sand.save(os.path.join(TILES, 'pasir_garam.png'))
    # rumpun bambu: muka batang (3x3 tile kiri lembar bambu)
    shutil.copy(os.path.join(SEV, 'bamboo.png'), os.path.join(TILES, 'bamboo.png'))
    for out, (src, frames) in autos.items():
        autotile.bake(os.path.join(SEV, src), os.path.join(TILES, out), frames)


if __name__ == '__main__':
    build_characters()
    build_tiles()
    print('aset selesai:', sorted(os.listdir(CHARS)), len(os.listdir(TILES)), 'tileset')


# ------------------------------------------------------------------ battle
RED = os.path.join(SRC, 'redshrike')
NID = os.path.join(SRC, 'nidhoggn')
BATTLERS = os.path.join(ROOT, 'assets', 'battlers')
BACKS = os.path.join(ROOT, 'assets', 'battlebacks')


def cut(sheet, box, scale, out, flip=False, key=None):
    """Potong satu musuh dari lembar Redshrike, buang latar, perbesar (nearest)."""
    im = Image.open(sheet).convert('RGBA').crop(box)
    px = im.load()
    bg = key if key is not None else Image.open(sheet).convert('RGBA').getpixel((0, 0))
    for y in range(im.height):
        for x in range(im.width):
            p = px[x, y]
            if bg[3] > 0 and abs(p[0] - bg[0]) + abs(p[1] - bg[1]) + abs(p[2] - bg[2]) < 24:
                px[x, y] = (0, 0, 0, 0)
    if flip:
        im = im.transpose(Image.FLIP_LEFT_RIGHT)
    im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
    im.save(os.path.join(BATTLERS, out))


def grade_battleback(src, out, dark=0.5, desat=0.45, tint=(1.0, 0.92, 0.85)):
    """Gelapkan & kurangi saturasi battleback agar sesuai nuansa kelam."""
    from PIL import ImageEnhance, ImageDraw, ImageFilter
    im = Image.open(os.path.join(NID, src)).convert('RGB').resize((640, 360), Image.LANCZOS)
    im = ImageEnhance.Color(im).enhance(1.0 - desat)
    im = ImageEnhance.Brightness(im).enhance(dark)
    r, g, b = im.split()
    r = r.point(lambda v: int(v * tint[0])); g = g.point(lambda v: int(v * tint[1])); b = b.point(lambda v: int(v * tint[2]))
    im = Image.merge('RGB', (r, g, b))
    # vinyet
    mask = Image.new('L', im.size, 0)
    ImageDraw.Draw(mask).ellipse((-120, -80, 760, 470), fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(70))
    black = Image.new('RGB', im.size, (0, 0, 0))
    im = Image.composite(im, black, mask)
    im.save(os.path.join(BACKS, out))


def build_battle():
    os.makedirs(BATTLERS, exist_ok=True)
    os.makedirs(BACKS, exist_ok=True)
    cut(os.path.join(RED, 'fossils3.PNG'), (171, 12, 219, 81), 2, 'tumbal_gagal.png')
    cut(os.path.join(RED, 'more_rpg_enemies.PNG'), (78, 19, 137, 110), 2, 'penjaga_lumbung.png')
    cut(os.path.join(RED, '9RPGenemies.PNG'), (108, 13, 157, 66), 2, 'belatung.png')
    cut(os.path.join(RED, 'rpgcritters2.png'), (9, 52, 46, 86), 3, 'belalang_sangit.png')
    # belalang: hijau terang -> hijau busuk kecokelatan agar sesuai palet ladang
    bl = Image.open(os.path.join(BATTLERS, 'belalang_sangit.png')).convert('RGBA')
    bl = _hue_shift(bl, (1.15, 0.78, 0.55))
    bl.save(os.path.join(BATTLERS, 'belalang_sangit.png'))
    # tikus: frame samping charset Sevarihk, menghadap ke kiri (ke arah party)
    rat = Image.open(os.path.join(SEV, 'rat-charset-with-shadow.png')).convert('RGBA').crop((0, 58, 71, 116))
    rat.resize((142, 116), Image.NEAREST).save(os.path.join(BATTLERS, 'tikus_lumbung.png'))
    cut(os.path.join(RED, 'more_rpg_enemies.PNG'), (9, 207, 51, 269), 3, 'arwah_jarum.png')
    cut(os.path.join(RED, '9RPGenemies.PNG'), (91, 190, 159, 308), 2, 'gagak_bangkai.png')
    cut(os.path.join(RED, 'rpgcritters2.png'), (55, 51, 83, 96), 3, 'ular_bambu.png')
    cut(os.path.join(RED, '9RPGenemies.PNG'), (4, 10, 91, 50), 2, 'hantu_laut.png')
    cut(os.path.join(RED, 'rpgcritters2.png'), (56, 151, 85, 186), 3, 'tengkorak_nelayan.png')
    cut(os.path.join(RED, 'more_rpg_enemies.PNG'), (72, 193, 125, 281), 2, 'akar_rawa.png')
    cut(os.path.join(RED, 'fossils3.PNG'), (234, 43, 347, 81), 2, 'buaya_bangkai.png')
    # warna agar sesuai wilayah: ular & akar lebih kusam
    for n, mul in [('ular_bambu.png', (0.9, 0.62, 0.4)), ('akar_rawa.png', (0.8, 0.85, 0.6))]:
        im = Image.open(os.path.join(BATTLERS, n)).convert('RGBA')
        _hue_shift(im, mul).save(os.path.join(BATTLERS, n))
    grade_battleback('battleback2.png', 'pantai_garam.png', dark=0.55, desat=0.75, tint=(0.85, 0.92, 1.0))
    grade_battleback('battleback1.png', 'rawa.png', dark=0.35, desat=0.4, tint=(0.95, 0.9, 0.55))
    grade_battleback('battleback5.png', 'sarang_buto.png', dark=0.4, desat=0.3, tint=(0.8, 1.0, 0.7))
    grade_battleback('battleback6.png', 'hutan_bambu.png', dark=0.38, desat=0.65, tint=(0.8, 1.0, 0.85))
    grade_battleback('battleback8.png', 'lumbung.png', dark=0.42, desat=0.55, tint=(1.0, 0.9, 0.8))
    grade_battleback('battleback5.png', 'gua.png', dark=0.5, desat=0.4)
    grade_battleback('battleback1.png', 'hutan.png', dark=0.45, desat=0.5, tint=(0.9, 1.0, 0.9))


# ------------------------------------------------------------------ props desa
PROPS = os.path.join(ROOT, 'assets', 'props')


def _crop(name, box):
    return Image.open(os.path.join(SEV, name)).convert('RGBA').crop(box)


def _night(im, dark=0.82, sat=0.75):
    """Sedikit gelap & pudar agar cocok dengan desa malam."""
    from PIL import ImageEnhance
    a = im.split()[3]
    rgb = ImageEnhance.Color(im.convert('RGB')).enhance(sat)
    rgb = ImageEnhance.Brightness(rgb).enhance(dark)
    out = rgb.convert('RGBA')
    out.putalpha(a)
    return out


def _hue_shift(im, rgb_mul):
    a = im.split()[3]
    r, g, b = im.convert('RGB').split()
    r = r.point(lambda v: min(255, int(v * rgb_mul[0])))
    g = g.point(lambda v: min(255, int(v * rgb_mul[1])))
    b = b.point(lambda v: min(255, int(v * rgb_mul[2])))
    out = Image.merge('RGB', (r, g, b)).convert('RGBA')
    out.putalpha(a)
    return out


def save_prop(im, name):
    im.save(os.path.join(PROPS, name))


def build_props():
    os.makedirs(PROPS, exist_ok=True)
    swamp = 'dead-swamp-v4.png'
    # rumah panggung (Fisher's Hut Sevarihk tanpa dermaga panjang)
    rumah = _crop('mini-fishers-hut.png', (16, 18, 304, 314))
    # buang percikan air biru di kaki tiang (rumah ini berdiri di tanah)
    px = rumah.load()
    for yy in range(rumah.height):
        for xx in range(rumah.width):
            r, g, b, a = px[xx, yy]
            if a > 0 and b > r + 30 and b > g:
                px[xx, yy] = (52, 36, 28, int(a * 0.6))
    save_prop(_night(rumah), 'rumah_panggung.png')
    save_prop(_night(_hue_shift(rumah, (0.78, 0.74, 0.70)), 0.75, 0.6), 'rumah_panggung_tua.png')

    # gubuk bambu: dinding bambu + atap ijuk + tirai pintu
    wall_src = _crop(swamp, (0, 119, 96, 224))           # tekstur bambu tegak
    roof = _crop(swamp, (22, 230, 169, 364))             # atap ijuk
    door = _crop(swamp, (175, 48, 238, 128))             # pintu tirai
    W, H = 160, 200
    g = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    wall = Image.new('RGBA', (128, 96), (0, 0, 0, 0))
    for x in range(0, 128, wall_src.width):
        wall.alpha_composite(wall_src.crop((0, 0, min(wall_src.width, 128 - x), 96)), (x, 0))
    g.alpha_composite(wall, (16, H - 96))
    g.alpha_composite(door, (W // 2 - door.width // 2, H - door.height))
    roof2 = roof.resize((int(roof.width * 1.15), roof.height), Image.NEAREST)
    g.alpha_composite(roof2, (W // 2 - roof2.width // 2, 0))
    save_prop(_night(g), 'gubuk.png')

    # bangunan lumbung Buto: dinding batu lumbung + gerbang tengkorak + atap ijuk lebar
    dung = Image.open(os.path.join(SRC, 'sevarihk', 'evildungeon_0.png')).convert('RGBA')
    T = 32
    def tile(i):
        return dung.crop(((i % 8) * T, (i // 8) * T, (i % 8) * T + T, (i // 8) * T + T))
    cols = 7
    body = Image.new('RGBA', (cols * T, 3 * T), (0, 0, 0, 0))
    plain = [[24, 29, 30, 31, 24, 29, 30], [32, 37, 38, 39, 32, 37, 38], [40, 45, 46, 47, 40, 45, 46]]
    for r in range(3):
        for c in range(cols):
            body.alpha_composite(tile(plain[r][c]), (c * T, r * T))
    arch = [[52, 53, 54], [60, 61, 62], [68, 69, 70]]
    for r in range(3):
        for c in range(3):
            body.alpha_composite(tile(arch[r][c]), ((2 + c) * T, r * T))
    roofw = roof.resize((cols * T + 48, int(roof.height * 1.25)), Image.NEAREST)
    lum = Image.new('RGBA', (roofw.width, roofw.height + 3 * T - 40), (0, 0, 0, 0))
    lum.alpha_composite(body, (24, lum.height - 3 * T))
    lum.alpha_composite(roofw, (0, 0))
    save_prop(_night(lum, 0.8), 'lumbung.png')

    # pohon mati & tiang tengkorak
    save_prop(_crop(swamp, (164, 1222, 251, 1332)), 'pohon_mati_a.png')
    save_prop(_crop(swamp, (4, 1231, 128, 1360)), 'pohon_mati_b.png')
    save_prop(_crop(swamp, (4, 1391, 159, 1496)), 'pohon_mati_c.png')
    save_prop(_crop(swamp, (102, 1120, 230, 1216)), 'orang_orangan.png')
    save_prop(_crop(swamp, (44, 1024, 215, 1054)), 'pagar.png')
    save_prop(_crop(swamp, (0, 1538, 64, 1616)), 'alang_alang.png')
    save_prop(_crop(swamp, (192, 227, 256, 350)), 'rak_bambu.png')
    save_prop(_night(_crop('bamboo.png', (160, 128, 224, 320)), 0.8), 'bambu_tinggi_a.png')
    save_prop(_night(_crop('bamboo.png', (200, 128, 256, 320)), 0.8), 'bambu_tinggi_b.png')
    save_prop(_night(_crop('tree-trunk-32x32swamp.png', (70, 164, 256, 494)), 0.7), 'tunggul_rawa.png')


# ------------------------------------------------------------------ aset Codex
# Gambar dari Codex (assets/raw/codex) berlatar magenta #FF00FF dengan grid
# piksel besar (mis. 8 px = 1 piksel seni). Diproses: latar -> transparan,
# kembali ke grid asli (ambil tengah tiap sel), lalu diskalakan ke kepadatan
# piksel yang sama dengan aset lain.
CODEX = os.path.join(ROOT, 'assets', 'raw', 'codex')


def _degrid(path, cell):
    im = Image.open(path).convert('RGBA')
    w, h = im.width // cell, im.height // cell
    out = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    src = im.load()
    dst = out.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = src[x * cell + cell // 2, y * cell + cell // 2]
            magenta = r > 180 and b > 180 and g < 110
            dst[x, y] = (0, 0, 0, 0) if magenta or a < 20 else (r, g, b, 255)
    return out


def _trim(im):
    box = im.getbbox()
    return im.crop(box) if box else im


def _components(im, gap=3):
    """Kotak pembatas bagian-bagian yang terpisah (untuk lembar props)."""
    w, h = im.size
    a = im.split()[3].load()
    seen = [[False] * h for _ in range(w)]
    boxes = []
    for sy in range(h):
        for sx in range(w):
            if seen[sx][sy] or a[sx, sy] == 0:
                continue
            stack = [(sx, sy)]
            seen[sx][sy] = True
            x0 = x1 = sx
            y0 = y1 = sy
            n = 0
            while stack:
                x, y = stack.pop()
                n += 1
                x0, x1, y0, y1 = min(x0, x), max(x1, x), min(y0, y), max(y1, y)
                for dx in range(-gap, gap + 1):
                    for dy in range(-gap, gap + 1):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < w and 0 <= ny < h and not seen[nx][ny] and a[nx, ny] > 0:
                            seen[nx][ny] = True
                            stack.append((nx, ny))
            if n > 20:
                boxes.append((x0, y0, x1 + 1, y1 + 1))
    # dua baris (atas/bawah lembar), masing-masing urut kiri ke kanan
    boxes.sort(key=lambda b: (0 if (b[1] + b[3]) / 2 < h / 2 else 1, b[0]))
    return boxes


def build_codex():
    os.makedirs(BATTLERS, exist_ok=True)
    os.makedirs(PROPS, exist_ok=True)
    # battler: grid 8 -> 128 sel; diperkecil ke kepadatan battler Redshrike (1 sel = 2 px)
    memedi = _trim(_degrid(os.path.join(CODEX, 'memedi_sawah.png'), 8))
    m = memedi.resize((int(memedi.width * 0.75), int(memedi.height * 0.75)), Image.NEAREST)
    m.resize((m.width * 2, m.height * 2), Image.NEAREST).save(os.path.join(BATTLERS, 'memedi_sawah.png'))
    # Memedi berdiri diam di ladang (prop peta, kepadatan 1 sel = 1 px)
    mp = memedi.resize((int(memedi.width * 0.62), int(memedi.height * 0.62)), Image.NEAREST)
    save_prop(_night(mp, 0.85), 'memedi_peta.png')

    timun = _trim(_degrid(os.path.join(CODEX, 'timun_busuk.png'), 8))
    t = timun.resize((int(timun.width * 0.6), int(timun.height * 0.6)), Image.NEAREST)
    t.resize((t.width * 2, t.height * 2), Image.NEAREST).save(os.path.join(BATTLERS, 'timun_busuk.png'))

    # Buto Ijo: tiga bagian (tangan kiri, kepala, tangan kanan) dengan palet sama
    for src, out, k in [('buto_kepala.png', 'buto_kepala.png', 0.8), ('buto_tangan_kiri.png', 'buto_kiri.png', 0.6),
                        ('buto_tangan_kanan.png', 'buto_kanan.png', 0.6)]:
        im = _trim(_degrid(os.path.join(CODEX, src), 8))
        im = im.resize((int(im.width * k), int(im.height * k)), Image.NEAREST)
        im.resize((im.width * 2, im.height * 2), Image.NEAREST).save(os.path.join(BATTLERS, out))
    head = _trim(_degrid(os.path.join(CODEX, 'buto_kepala.png'), 8))
    save_prop(_night(head, 0.8), 'buto_peta.png')
    # lembar props ladang: grid 4 -> 1 sel = 1 px, sudah seskala tile 32 px
    sheet = _degrid(os.path.join(CODEX, 'ladang_props.png'), 4)
    names = ['ajir.png', 'ajir_roboh.png', 'gubuk_sawah.png', 'timun_tumpuk.png', 'ember.png', 'cangkul.png',
             'pagar_bambu.png']
    boxes = _components(sheet)
    for name, box in zip(names, boxes):
        save_prop(_night(sheet.crop(box), 0.85), name)
    # sprite peta musuh ladang: battler diperkecil, 4 frame x 4 arah (bergoyang tipis)
    for src, out, size in [('belalang_sangit.png', 'belalang.png', 40), ('timun_busuk.png', 'timun_busuk.png', 44),
                           ('arwah_jarum.png', 'arwah.png', 46), ('gagak_bangkai.png', 'gagak.png', 50),
                           ('ular_bambu.png', 'ular.png', 40)]:
        b = Image.open(os.path.join(BATTLERS, src)).convert('RGBA')
        k = size / max(b.width, b.height)
        small = b.resize((max(1, int(b.width * k)), max(1, int(b.height * k))), Image.NEAREST)
        fw, fh = 64, 74
        sheet = Image.new('RGBA', (fw * 4, fh * 4), (0, 0, 0, 0))
        for row in range(4):
            img = small.transpose(Image.FLIP_LEFT_RIGHT) if row == 2 else small
            for col in range(4):
                dy = 1 if col in (1, 3) else 0
                sheet.alpha_composite(img, (fw // 2 - img.width // 2, fh - 6 - img.height + dy + (row * fh)), )
                sheet.paste(sheet.crop((0, row * fh, fw, row * fh + fh)), (col * fw, row * fh)) if col else None
        # isi kolom 1-3 dengan frame yang sama, kolom ganjil turun 1 px
        base = sheet.crop((0, 0, fw, fh * 4))
        final = Image.new('RGBA', sheet.size, (0, 0, 0, 0))
        for col in range(4):
            final.alpha_composite(base, (col * fw, 1 if col % 2 else 0))
        final.save(os.path.join(CHARS, out))
    return len(boxes)


if __name__ == '__main__':
    build_battle()
    build_props()
    print('komponen props ladang:', build_codex())
    print('battler:', sorted(os.listdir(BATTLERS)), 'battleback:', sorted(os.listdir(BACKS)))
    print('props:', sorted(os.listdir(PROPS)))
