"""Kumpulkan semua teks yang tampil ke pemain (kunci terjemahan) dan cek cakupan assets/lang/en.json.

Sumber kunci:
  - literal string di src/gui, src/console, src/core (kecuali jejak dsOp, path, kunci data teknis)
  - nama & deskripsi di src/core/data/GameData.cpp
  - skrip event assets/events/*.json (who, text, options, notice)
  - nama peta (world.json, property peta "nama") dan pesan pintu terkunci ("pesan")

Pemakaian (dari root repo):
    /c/msys64/ucrt64/bin/python.exe tools/lang_keys.py          # laporan kunci yang belum diterjemahkan
    /c/msys64/ucrt64/bin/python.exe tools/lang_keys.py --list   # cetak semua kunci (JSON)
Keluar dengan kode 1 bila ada kunci tanpa terjemahan.
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STR = re.compile(r'"((?:[^"\\]|\\.)*)"')
JSONISH = re.compile(r'":|\{"|\["|"\]|\],|^,|companionState')   # potongan berkas simpan JSON
SKIP_LINE = re.compile(r'dsOp\(|ds\(|#include|TakeScreenshot|getString\(|getInt\(|getBool\(|getNumber\(|setFlag\(|flag\(|'
                       r'runEvent\(|texture\(|path\(|hostOpenShop|props\(|o->type|->type ==|type\(\) ==|get\("|'
                       r'Assets::|musicPath|playMusic|"assets/|std::fprintf\(stderr')
TECH = re.compile(r'^[a-z0-9_./%-]*$')          # kunci data teknis: "jamu", "dari_desa", "%d"


def unescape(s):
    return bytes(s, 'utf-8').decode('unicode_escape').encode('latin-1').decode('utf-8')


def literal_runs(line):
    """Gabungkan literal yang bersebelahan ("a" "b") seperti kompiler C++."""
    out, cur, pos = [], None, 0
    for m in STR.finditer(line):
        gap = line[pos:m.start()]
        if cur is not None and gap.strip() == '':
            cur += m.group(1)
        else:
            if cur is not None:
                out.append(cur)
            cur = m.group(1)
        pos = m.end()
    if cur is not None:
        out.append(cur)
    return out


def code_keys():
    keys = set()
    files = glob.glob(os.path.join(ROOT, 'src', 'gui', '*.cpp')) + glob.glob(os.path.join(ROOT, 'src', 'console', 'ConsoleGame.cpp')) \
        + glob.glob(os.path.join(ROOT, 'src', 'core', '*', '*.cpp'))
    for f in files:
        if f.endswith(('Json.cpp', 'TileMap.cpp', 'Lang.cpp', 'Script.cpp')):
            continue
        text = open(f, encoding='utf-8').read()
        # gabungkan baris lanjutan literal ("a"\n   "b")
        text = re.sub(r'"\s*\n\s*"', '', text)
        for line in text.split('\n'):
            cut = line.find('//')
            if cut >= 0 and line[:cut].count('"') % 2 == 0:
                line = line[:cut]                      # buang komentar di ujung baris
            if SKIP_LINE.search(line):
                continue
            for s in literal_runs(line):
                s = unescape(s)
                letters = re.sub(r'%[-0-9.*]*[sdc]', '', s)
                if not re.search(r'[A-Za-z]{2,}', letters) or TECH.match(s) or JSONISH.search(s):
                    continue
                keys.add(s)
    return keys


def event_keys():
    keys = set()

    def walk(cmds):
        for c in cmds:
            for k in ('who', 'text'):
                if c.get(k):
                    keys.add(c[k])
            for o in c.get('options', []):
                keys.add(o)
            for k in ('branches',):
                for b in c.get(k, []):
                    walk(b)
            for k in ('then', 'else'):
                walk(c.get(k, []))
    for f in glob.glob(os.path.join(ROOT, 'assets', 'events', '*.json')):
        for script in json.load(open(f, encoding='utf-8')).values():
            walk(script)
    return keys


def map_keys():
    keys = set()
    world = json.load(open(os.path.join(ROOT, 'assets', 'maps', 'world.json'), encoding='utf-8'))
    for m in world['maps']:
        keys.add(m['name'])
    for f in glob.glob(os.path.join(ROOT, 'assets', 'maps', '*.tmj')):
        m = json.load(open(f, encoding='utf-8'))
        for p in m.get('properties', []):
            if p['name'] == 'nama':
                keys.add(p['value'])
        for layer in m['layers']:
            for o in layer.get('objects', []):
                for p in o.get('properties', []):
                    if p['name'] == 'pesan' and p['value']:
                        keys.add(p['value'])
    return keys


IGNORE = {'TUMBAL', 'HP', 'MP', 'AP', 'RP', 'Lv', 'English', 'Bahasa Indonesia', 'CREDITS.md', 'HEAD ', '  <-> ',
          'Tumbal', 'TUMBAL - %s',
          # jejak/teknis yang tidak tampil sebagai teks permainan
          'if barang', 'panggil ', 'pilihan "', 'skrip ', 'buka %d %d', 'id peta ganda: ', 'Gagal memuat dunia: ',
          'Gagal memuat dunia: %s\n', '%s%s  sisa %d tick%s'}


# kata UI huruf kecil satu kata (lolos dari filter TECH) yang memang tampil ke pemain
UI_WORDS = {'rapal', 'tunda', 'mati', 'habis', 'terbuka', 'ikut', 'menunggu', 'dikunjungi', 'belum'}


def all_keys():
    keys = code_keys() | event_keys() | map_keys() | UI_WORDS
    return sorted(k for k in keys if k not in IGNORE and k.strip())


if __name__ == '__main__':
    keys = all_keys()
    if '--list' in sys.argv:
        print(json.dumps(keys, ensure_ascii=False, indent=1))
        sys.exit(0)
    path = os.path.join(ROOT, 'assets', 'lang', 'en.json')
    have = json.load(open(path, encoding='utf-8')) if os.path.exists(path) else {}
    missing = [k for k in keys if not have.get(k)]
    unused = [k for k in have if not k.startswith('_') and k not in keys and k not in IGNORE]
    print(f'{len(keys)} kunci, {len(keys) - len(missing)} sudah diterjemahkan, {len(missing)} belum, {len(unused)} tak terpakai')
    for k in missing:
        print('  BELUM:', json.dumps(k, ensure_ascii=False))
    for k in unused[:40]:
        print('  tak terpakai:', json.dumps(k, ensure_ascii=False))
    sys.exit(1 if missing else 0)
