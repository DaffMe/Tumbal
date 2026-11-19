"""Generator peta TUMBAL: tata letak ASCII -> berkas Tiled (.tmj) di assets/maps.

Hasilnya peta Tiled biasa: bisa dibuka & disunting di Tiled (mapeditor.org).
Peta yang sudah disunting manual di Tiled sebaiknya dikeluarkan dari daftar
MAPS di bawah agar tidak tertimpa.

Jalankan dari root repo:
    /c/msys64/ucrt64/bin/python.exe tools/make_maps.py

Legenda ASCII (peta dungeon):
    #  massa tembok (otomatis jadi atap autotile + muka tembok 3 tile)
    .  lantai bata     ,  lantai lempeng     :  lantai retak
    huruf lain = penanda di atas lantai (diterjemahkan lewat `marks`)
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import autotile  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAPS_DIR = os.path.join(ROOT, 'assets', 'maps')
TILES_DIR = os.path.join(ROOT, 'assets', 'tiles')
T = 32

# --------------------------------------------------------------- tileset
# (nama, berkas, kolom, jumlah tile)
TILESETS = {
    'dungeon': ('evildungeon_0.png', 8, 8 * 17),
    'bata':    ('auto_tembok_bata.png', 8, 48),
    'tabrak':  ('collision.png', 1, 1),
}


def ensure_collision_png():
    from PIL import Image, ImageDraw
    p = os.path.join(TILES_DIR, 'collision.png')
    if os.path.exists(p):
        return
    im = Image.new('RGBA', (T, T), (220, 40, 40, 110))
    d = ImageDraw.Draw(im)
    d.line([(0, 0), (T - 1, T - 1)], fill=(255, 80, 80, 200))
    d.line([(T - 1, 0), (0, T - 1)], fill=(255, 80, 80, 200))
    im.save(p)


class MapBuilder:
    def __init__(self, name, rows, tileset_names):
        self.name = name
        self.rows = [r.rstrip('\n') for r in rows]
        self.h = len(self.rows)
        self.w = max(len(r) for r in self.rows)
        self.rows = [r.ljust(self.w, '#') for r in self.rows]
        self.firstgid = {}
        gid = 1
        self.ts_order = tileset_names
        for n in tileset_names:
            self.firstgid[n] = gid
            gid += TILESETS[n][2]
        self.layers = {k: [0] * (self.w * self.h) for k in ('lantai', 'tembok', 'hiasan', 'atap', 'collision')}
        self.objects = []
        self.props = {}

    # ---- util
    def ch(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.rows[y][x]
        return '#'

    def is_wall(self, x, y):
        return self.ch(x, y) == '#'

    def set(self, layer, x, y, ts, local):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.layers[layer][y * self.w + x] = self.firstgid[ts] + local

    def block(self, x, y, on=True):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.layers['collision'][y * self.w + x] = self.firstgid['tabrak'] if on else 0

    def obj(self, type_, x, y, name='', w=1, h=1, **props):
        self.objects.append(dict(type=type_, x=x, y=y, name=name, w=w, h=h, props=props))

    # ---- dungeon: tembok + lantai
    def build_dungeon(self, marks):
        face_rows = {0: 24, 1: 32, 2: 40}   # baris atas/tengah/bawah muka tembok
        plain_cols = [0, 5, 6, 7, 0, 3]
        self.face = set()
        for y in range(self.h):
            for x in range(self.w):
                c = self.ch(x, y)
                if c != '#':
                    floor = {',': 3, ':': 4}.get(c, 2)
                    if c == '.' and (x * 7 + y * 13) % 11 == 0:
                        floor = 4
                    self.set('lantai', x, y, 'dungeon', floor)
                    continue
                self.block(x, y)
                # jarak ke lantai di bawah (muka tembok setinggi 3 tile)
                for k in (1, 2, 3):
                    if not self.is_wall(x, y + k):
                        if all(self.is_wall(x, y + j) for j in range(1, k)):
                            row = {3: 0, 2: 1, 1: 2}[k]
                            col = plain_cols[(x * 3 + y) % len(plain_cols)]
                            if row == 0 and col == 3:
                                col = 0      # tile 27 = puncak lengkung, bukan bata polos
                            # pilar tengkorak berirama setiap 7 kolom
                            if x % 7 == 3:
                                col = 1
                            self.set('tembok', x, y, 'dungeon', face_rows[row] + col)
                            self.face.add((x, y))
                        break
        # atap: autotile bata di atas massa tembok yang bukan muka tembok
        def ceil(x, y):
            return (x < 0 or y < 0 or x >= self.w or y >= self.h) or (self.is_wall(x, y) and (x, y) not in self.face)
        for y in range(self.h):
            for x in range(self.w):
                if not ceil(x, y):
                    continue
                m = 0
                for bit, (dx, dy) in ((autotile.N, (0, -1)), (autotile.NE, (1, -1)), (autotile.E, (1, 0)),
                                      (autotile.SE, (1, 1)), (autotile.S, (0, 1)), (autotile.SW, (-1, 1)),
                                      (autotile.W, (-1, 0)), (autotile.NW, (-1, -1))):
                    if ceil(x + dx, y + dy):
                        m |= bit
                self.set('atap', x, y, 'bata', autotile.index_for(m))
        # penanda
        for y in range(self.h):
            for x in range(self.w):
                c = self.ch(x, y)
                if c in marks:
                    marks[c](self, x, y)

    # ---- elemen dungeon
    def arch(self, x, y, **door_props):
        """Gerbang lengkung tengkorak 3x3 di muka tembok, tepat di atas lantai (x,y)."""
        tiles = [[52, 53, 54], [60, 61, 62], [68, 69, 70]]
        for r in range(3):
            for c in range(3):
                self.set('tembok', x - 1 + c, y - 3 + r, 'dungeon', tiles[r][c])
        self.obj('door', x, y - 1, **door_props)

    def deco(self, x, y, local, solid=False, layer='hiasan'):
        self.set(layer, x, y, 'dungeon', local)
        if solid:
            self.block(x, y)

    # ---- simpan
    def save(self, filename):
        layers = []
        lid = 1
        for name in ('lantai', 'tembok', 'hiasan', 'atap', 'collision'):
            L = dict(id=lid, name=name, type='tilelayer', width=self.w, height=self.h, x=0, y=0,
                     opacity=1, visible=(name != 'collision'), data=self.layers[name])
            if name == 'atap':
                L['properties'] = [dict(name='above', type='bool', value=True)]
            layers.append(L)
            lid += 1
        objs = []
        for i, o in enumerate(self.objects, start=1):
            props = [dict(name=k, type=('bool' if isinstance(v, bool) else 'int' if isinstance(v, int) else 'string'),
                          value=v) for k, v in o['props'].items()]
            objs.append(dict(id=i, name=o['name'], type=o['type'], x=o['x'] * T, y=o['y'] * T,
                             width=o['w'] * T, height=o['h'] * T, rotation=0, visible=True, properties=props))
        layers.append(dict(id=lid, name='objek', type='objectgroup', x=0, y=0, opacity=1, visible=True,
                           draworder='topdown', objects=objs))
        tilesets = []
        for n in self.ts_order:
            img, cols, count = TILESETS[n]
            from PIL import Image
            iw, ih = Image.open(os.path.join(TILES_DIR, img)).size
            tilesets.append(dict(firstgid=self.firstgid[n], name=n, image='../tiles/' + img, imagewidth=iw,
                                 imageheight=ih, tilewidth=T, tileheight=T, columns=cols, tilecount=count,
                                 margin=0, spacing=0))
        data = dict(type='map', version='1.10', tiledversion='1.10.2', orientation='orthogonal',
                    renderorder='right-down', width=self.w, height=self.h, tilewidth=T, tileheight=T,
                    infinite=False, nextlayerid=lid + 1, nextobjectid=len(objs) + 1, layers=layers,
                    tilesets=tilesets,
                    properties=[dict(name=k, type='string', value=str(v)) for k, v in self.props.items()])
        with open(os.path.join(MAPS_DIR, filename), 'w', encoding='utf-8') as f:
            json.dump(data, f, separators=(',', ':'))


# ============================================================== PETA
# id peta (sama dengan world.json)
SEL_TUMBAL, LORONG_LUMBUNG = 1, 2


def map_sel_tumbal():
    rows = [
        "################################",
        "################################",
        "################################",
        "####....#####....#####....######",
        "####....#####....#####..L.######",
        "####.S..#####....#####....######",
        "####....#####.L..#####....######",
        "#####.#########.#########.######",
        "#####.#########.#########.######",
        "#####.#########.#########.######",
        "#####.#########.#########.######",
        "###,,,,,,,,,,,,,,,,,,,,,,,,A,###",
        "###,,,,,,,,,,,,,,,,,,,,,,,,,,###",
        "###,,,,,,,,,,,,,,,,,,,,,,,,,,###",
        "###,,,,,,,,,,,,,,,,,,,,,,,,,,###",
        "################################",
        "################################",
    ]
    b = MapBuilder('Sel Tumbal', rows, ['dungeon', 'bata', 'tabrak'])
    b.props.update(nama='Sel Tumbal', musik='lumbung', gelap=0.72)
    marks = {
        'S': lambda b, x, y: b.obj('spawn', x, y, name='awal', dir=0),
        'L': lambda b, x, y: b.obj('loot', x, y, item='sukma_kecil'),
        'A': lambda b, x, y: (b.arch(x, y, to=LORONG_LUMBUNG, spawn='dari_sel'),
                              b.obj('spawn', x, y, name='dari_lorong', dir=0)),
    }
    b.build_dungeon(marks)
    # hiasan sel: ceruk tengkorak & rantai di muka tembok, tulang di lantai
    b.deco(6, 1, 57)
    b.deco(15, 1, 56)
    b.deco(24, 1, 58)
    b.deco(24, 2, 66)
    b.deco(7, 6, 64)
    b.deco(16, 4, 64)
    b.deco(10, 12, 65)
    b.deco(20, 9, 59)
    b.save('sel_tumbal.tmj')


def map_lorong_lumbung():
    rows = [
        "##########################################",
        "##########################################",
        "##########################################",
        "######,,,,,,,,,,,######,,,,,,,,,,,,,######",
        "######,,,,,,,,,,,######,,,,,,,,,,,,,######",
        "######,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,######",
        "######,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,######",
        "######,,,,,,,,,,,######,,,,,,,,,,,,,######",
        "######,,,,,,,,,,,######,,,,,,L,,,,,,######",
        "##########,,,################,,,##########",
        "##########,,,################,,,##########",
        "##########,,,################,,,##########",
        "##########,,,################,,,##########",
        "#####..B......#############.........######",
        "#####...L.....#############.........######",
        "#####.........#############....A....######",
        "#####.........#############.........######",
        "##########################################",
        "##########################################",
    ]
    b = MapBuilder('Lorong Lumbung', rows, ['dungeon', 'bata', 'tabrak'])
    b.props.update(nama='Lorong Lumbung', musik='lumbung', gelap=0.78)
    marks = {
        'L': lambda b, x, y: b.obj('loot', x, y, item='sukma_kecil'),
        'B': lambda b, x, y: (b.arch(x, y, to=SEL_TUMBAL, spawn='dari_lorong'),
                              b.obj('spawn', x, y, name='dari_sel', dir=0)),
        'A': lambda b, x, y: b.obj('tungku', x, y, name='tungku_lumbung'),
    }
    b.build_dungeon(marks)
    # kursi tengkorak & patung di aula atas
    b.deco(11, 3, 100); b.deco(11, 4, 108, solid=True)
    b.deco(28, 3, 97); b.deco(28, 4, 105, solid=True)
    b.deco(8, 2, 57); b.deco(25, 2, 56); b.deco(33, 2, 58); b.deco(33, 1, 58, layer='tembok')
    b.deco(20, 6, 64); b.deco(30, 14, 65)
    b.save('lorong_lumbung.tmj')


WORLD = {
    'start': {'map': SEL_TUMBAL, 'spawn': 'awal'},
    'maps': [
        {'id': SEL_TUMBAL, 'name': 'Sel Tumbal', 'file': 'sel_tumbal.tmj'},
        {'id': LORONG_LUMBUNG, 'name': 'Lorong Lumbung', 'file': 'lorong_lumbung.tmj'},
    ],
}

if __name__ == '__main__':
    os.makedirs(MAPS_DIR, exist_ok=True)
    ensure_collision_png()
    map_sel_tumbal()
    map_lorong_lumbung()
    with open(os.path.join(MAPS_DIR, 'world.json'), 'w', encoding='utf-8') as f:
        json.dump(WORLD, f, indent=2, ensure_ascii=False)
    print('peta selesai:', sorted(os.listdir(MAPS_DIR)))
