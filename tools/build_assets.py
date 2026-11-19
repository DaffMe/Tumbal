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
