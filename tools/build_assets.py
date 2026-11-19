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
        'auto_bambu_atas.png': ('bamboo-top.png', 1),
        'auto_air_rawa.png': ('swamp-bruhe_animated-v5.png', 4),
    }
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
    # tikus: frame samping charset Sevarihk, menghadap ke kiri (ke arah party)
    rat = Image.open(os.path.join(SEV, 'rat-charset-with-shadow.png')).convert('RGBA').crop((0, 58, 71, 116))
    rat.resize((142, 116), Image.NEAREST).save(os.path.join(BATTLERS, 'tikus_lumbung.png'))
    grade_battleback('battleback8.png', 'lumbung.png', dark=0.42, desat=0.55, tint=(1.0, 0.9, 0.8))
    grade_battleback('battleback5.png', 'gua.png', dark=0.5, desat=0.4)
    grade_battleback('battleback1.png', 'hutan.png', dark=0.45, desat=0.5, tint=(0.9, 1.0, 0.9))


if __name__ == '__main__':
    build_battle()
    print('battler:', sorted(os.listdir(BATTLERS)), 'battleback:', sorted(os.listdir(BACKS)))
