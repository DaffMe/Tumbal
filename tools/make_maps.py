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
    'rumput':  ('auto_rumput_liar.png', 8, 48),
    'tanah':   ('auto_tanah.png', 8, 48),
    'semak':   ('auto_semak.png', 8, 48),
    'hutan':   ('auto_rumput_hutan.png', 8, 48),
    'lumpur':  ('auto_air_rawa.png', 8, 48 * 4),     # 4 frame animasi
    'laut':    ('auto_air_laut.png', 8, 48 * 4),
    'pasir':   ('pasir_garam.png', 1, 1),
    'bambu':   ('bamboo.png', 8, 80),
    'kanopi':  ('auto_bambu_atas.png', 8, 48),
    'rumput_rawa':  ('auto_rumput_rawa.png', 8, 48),
    'hutan_rawa':   ('auto_hutan_rawa.png', 8, 48),
    'tanah_rawa':   ('auto_tanah_rawa.png', 8, 48),
    'rumput_sarang': ('auto_rumput_sarang.png', 8, 48),
    'hutan_sarang':  ('auto_hutan_sarang.png', 8, 48),
    'dekor':   ('dekor.png', 8, 16),
}
# indeks tile dekor.png (lihat build_decor_tiles di tools/build_assets.py)
DEKOR_SEMANGGI = (0, 1, 2)
DEKOR_DAUN = (3, 4, 5)
DEKOR_TENGKORAK, DEKOR_TULANG, DEKOR_RANTING = 6, 7, 8
DEKOR_BATU = (9, 10)
ANIMATED = {'lumpur': (4, 48, 260), 'laut': (4, 48, 300)}   # frame, tile per frame, durasi ms
LAYERS = ('lantai', 'tanah', 'air', 'tembok', 'semak', 'hiasan', 'atap', 'collision')


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
        self.layers = {k: [0] * (self.w * self.h) for k in LAYERS}
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

    def prop(self, image, x, bottom, foot=None, solid=True, name=''):
        """Objek gambar besar (rumah, pohon). x = tile kiri, bottom = baris tile paling bawah
        (inklusif). foot = (kolom_awal, lebar, tinggi) tile yang terhalang di bagian bawah."""
        from PIL import Image
        iw, ih = Image.open(os.path.join(ROOT, 'assets', 'props', image)).size
        px = x * T
        py = (bottom + 1) * T - ih
        self.objects.append(dict(type='prop', x=px, y=py, name=name or image, w=iw, h=ih,
                                 props=dict(image='assets/props/' + image), pixel=True))
        if solid:
            fx, fw, fh = foot if foot else (0, (iw + T - 1) // T, 1)
            for yy in range(bottom - fh + 1, bottom + 1):
                for xx in range(x + fx, x + fx + fw):
                    self.block(xx, yy)
        return (iw, ih)

    # ---- luar ruangan: rumput dasar + autotile tanah/lumpur/semak/hutan
    def build_outdoor(self, marks):
        original = list(self.rows)
        full = autotile.index_for(0xFF)
        base_ts = getattr(self, 'base_ts', 'rumput')
        water_ts = getattr(self, 'water_ts', 'lumpur')
        # penanda (huruf) di atas jalan/lumpur: medan di bawahnya ikut tetangga
        terrain = '".~&%#'
        for y in range(self.h):
            row = list(self.rows[y])
            for x in range(self.w):
                if row[x] in terrain:
                    continue
                around = [self.ch(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
                row[x] = '.' if around.count('.') >= 2 else '"'
            self.rows[y] = ''.join(row)
        path_ts = getattr(self, 'path_ts', 'tanah')
        forest_ts = getattr(self, 'forest_ts', 'hutan')
        AUTO = {'.': ('tanah', path_ts, False), '~': ('air', water_ts, True),
                '&': ('semak', 'semak', True), '%': ('semak', forest_ts, True)}
        for y in range(self.h):
            for x in range(self.w):
                self.set('lantai', x, y, base_ts, 0 if base_ts == 'pasir' else full)
                c = self.ch(x, y)
                if c not in AUTO:
                    continue
                layer, ts, solid = AUTO[c]
                same = lambda xx, yy: self.ch(xx, yy) == c if (0 <= xx < self.w and 0 <= yy < self.h) else c in '%&'
                m = 0
                for bit, (dx, dy) in ((autotile.N, (0, -1)), (autotile.NE, (1, -1)), (autotile.E, (1, 0)),
                                      (autotile.SE, (1, 1)), (autotile.S, (0, 1)), (autotile.SW, (-1, 1)),
                                      (autotile.W, (-1, 0)), (autotile.NW, (-1, -1))):
                    if same(x + dx, y + dy):
                        m |= bit
                self.set(layer, x, y, ts, autotile.index_for(m))
                if solid:
                    self.block(x, y)
        for y in range(self.h):
            for x in range(self.w):
                c = original[y][x]
                if c in marks:
                    marks[c](self, x, y)

    def scatter(self, grass, path=(), grass_rate=0.10, path_rate=0.04, seed=None):
        """Hiasan kecil acak (deterministik) di layer `hiasan`: tidak menghalangi, digambar di bawah karakter.
        grass/path = daftar indeks tile dekor untuk petak rumput / jalan tanah."""
        import random
        rnd = random.Random(seed or self.name)
        taken = {(o['x'], o['y']) for o in self.objects if not o.get('pixel')}
        for y in range(self.h):
            for x in range(self.w):
                if (x, y) in taken or self.layers['collision'][y * self.w + x] or self.layers['hiasan'][y * self.w + x]:
                    continue
                c = self.ch(x, y)
                pool, rate = (grass, grass_rate) if c == '"' else (path, path_rate) if c == '.' else ((), 0)
                if pool and rnd.random() < rate:
                    self.set('hiasan', x, y, 'dekor', rnd.choice(pool))

    def scatter_props(self, images, count, seed=None, solid=False, avoid=2):
        """Sebar props kecil (gelagah, ranting) di petak rumput kosong, jauh dari jalan & objek."""
        import random
        rnd = random.Random((seed or self.name) + 'props')
        taken = {(o['x'], o['y']) for o in self.objects if not o.get('pixel')}
        placed, tries = 0, 0
        while placed < count and tries < count * 60:
            tries += 1
            x, y = rnd.randrange(1, self.w - 1), rnd.randrange(2, self.h - 1)
            ok = all(self.ch(x + dx, y + dy) == '"' and not self.layers['collision'][(y + dy) * self.w + x + dx]
                     and (x + dx, y + dy) not in taken
                     for dx in range(-avoid, avoid + 1) for dy in range(-avoid, avoid + 1)
                     if 0 <= x + dx < self.w and 0 <= y + dy < self.h)
            if not ok:
                continue
            self.prop(rnd.choice(images), x, y, foot=(0, 1, 1), solid=solid)
            taken.add((x, y))
            placed += 1

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


    # ---- rumpun bambu: massa '#' seperti dinding (kanopi di atas, batang 3 tile)
    def build_grove(self):
        def wall(x, y):
            return self.ch(x, y) == '#'
        faces = set()
        for y in range(self.h):
            for x in range(self.w):
                if not wall(x, y):
                    continue
                self.block(x, y)
                for k in (1, 2, 3):
                    if not wall(x, y + k):
                        if all(wall(x, y + j) for j in range(1, k)):
                            row = {3: 0, 2: 1, 1: 2}[k]
                            col = 0 if not wall(x - 1, y + k - 1 if k > 1 else y) else (2 if not wall(x + 1, y) else 1)
                            self.set('tembok', x, y, 'bambu', row * 8 + col)
                            faces.add((x, y))
                        break
        def canopy(x, y):
            return (x < 0 or y < 0 or x >= self.w or y >= self.h) or (wall(x, y) and (x, y) not in faces)
        for y in range(self.h):
            for x in range(self.w):
                if not canopy(x, y):
                    continue
                m = 0
                for bit, (dx, dy) in ((autotile.N, (0, -1)), (autotile.NE, (1, -1)), (autotile.E, (1, 0)),
                                      (autotile.SE, (1, 1)), (autotile.S, (0, 1)), (autotile.SW, (-1, 1)),
                                      (autotile.W, (-1, 0)), (autotile.NW, (-1, -1))):
                    if canopy(x + dx, y + dy):
                        m |= bit
                self.set('atap', x, y, 'kanopi', autotile.index_for(m))

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
        for name in LAYERS:
            if name != 'collision' and not any(self.layers[name]):
                continue
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
            k = 1 if o.get('pixel') else T
            objs.append(dict(id=i, name=o['name'], type=o['type'], x=o['x'] * k, y=o['y'] * k,
                             width=o['w'] * k, height=o['h'] * k, rotation=0, visible=True, properties=props))
        layers.append(dict(id=lid, name='objek', type='objectgroup', x=0, y=0, opacity=1, visible=True,
                           draworder='topdown', objects=objs))
        tilesets = []
        for n in self.ts_order:
            img, cols, count = TILESETS[n]
            from PIL import Image
            iw, ih = Image.open(os.path.join(TILES_DIR, img)).size
            ts = dict(firstgid=self.firstgid[n], name=n, image='../tiles/' + img, imagewidth=iw,
                      imageheight=ih, tilewidth=T, tileheight=T, columns=cols, tilecount=count,
                      margin=0, spacing=0)
            if n in ANIMATED:
                frames, per, dur = ANIMATED[n]
                ts['tiles'] = [dict(id=i, animation=[dict(tileid=i + per * f, duration=dur) for f in range(frames)])
                               for i in range(per)]
            tilesets.append(ts)
        data = dict(type='map', version='1.10', tiledversion='1.10.2', orientation='orthogonal',
                    renderorder='right-down', width=self.w, height=self.h, tilewidth=T, tileheight=T,
                    infinite=False, nextlayerid=lid + 1, nextobjectid=len(objs) + 1, layers=layers,
                    tilesets=tilesets,
                    properties=[dict(name=k, type='string', value=str(v)) for k, v in self.props.items()])
        with open(os.path.join(MAPS_DIR, filename), 'w', encoding='utf-8') as f:
            json.dump(data, f, separators=(',', ':'))


# ============================================================== PETA
# id troop (sama dengan src/core/data/GameData.hpp)
TR_TUMBAL_GAGAL, TR_TIKUS_2, TR_TUMBAL_TIKUS, TR_BELATUNG_3, TR_PENJAGA = 1, 2, 3, 4, 5
TR_BELALANG_2, TR_TIMUN_BELALANG, TR_TIMUN_2, TR_MEMEDI = 6, 7, 8, 9
TR_ARWAH_2, TR_GAGAK, TR_ULAR_ARWAH, TR_HANTU_LAUT, TR_TENGKORAK_2, TR_HANTU_TENGKORAK = 10, 11, 12, 13, 14, 15
TR_AKAR_2, TR_BUAYA, TR_AKAR_BELATUNG, TR_BUTO = 16, 17, 18, 19

# id peta (sama dengan world.json)
SEL_TUMBAL, LORONG_LUMBUNG, DESA, LADANG, HUTAN_BAMBU, LAUT_GARAM, RAWA_TERASI, SARANG_BUTO = 1, 2, 3, 4, 5, 6, 7, 8


def map_sel_tumbal():
    rows = [
        "################################",
        "################################",
        "################################",
        "####....#####....#####....######",
        "####....#####....#####..L.######",
        "####.S..#####....#####.N..######",
        "####....#####.K..#####....######",
        "#####.#########.#########.######",
        "#####.#########.#########.######",
        "#####.#########.#########.######",
        "#####.#########.#########.######",
        "###,,,,,,,,,,,,,,,,,,,,,,,,A,###",
        "###,,,,,,,,,,,,,,,,,,,,,,,,,,###",
        "###,,,,,,,,,,,,,,,M,,,,,,,,,,###",
        "###,,,,,,,,,,,,,,,,,,,,,,,,,,###",
        "################################",
        "################################",
    ]
    b = MapBuilder('Sel Tumbal', rows, ['dungeon', 'bata', 'tabrak'])
    b.props.update(nama='Sel Tumbal', musik='lumbung', gelap=0.72, battleback='assets/battlebacks/lumbung.png')
    marks = {
        'S': lambda b, x, y: b.obj('spawn', x, y, name='awal', dir=0),
        'L': lambda b, x, y: b.obj('loot', x, y, item='penawar'),
        'K': lambda b, x, y: b.obj('loot', x, y, event='mayat_sel'),
        'N': lambda b, x, y: (b.obj('npc', x, y, name='lelaki_tua', event='lelaki_tua',
                                    walker='assets/chars/lelaki_tua.png', dir=0), b.block(x, y)),
        'M': lambda b, x, y: b.obj('enemy', x, y, troop=TR_TUMBAL_GAGAL, walker='assets/chars/tumbal_gagal.png', dir=1),
        'A': lambda b, x, y: (b.arch(x, y, to=LORONG_LUMBUNG, spawn='dari_sel'),
                              b.obj('spawn', x, y, name='dari_lorong', dir=0)),
    }
    b.build_dungeon(marks)
    b.obj('event', 0, 0, name='pembuka', event='pembuka', trigger='auto', once='pembuka')
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
        "######,,,,,,,,,,,######,,,,,,,G,,,,,######",
        "######,,,,,,,,,,,######,,,,,,,,,,,,,######",
        "######,,,,,,,,R,,,,,,,,,,,,,,,,,,,,,######",
        "######,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,######",
        "######,,,,,,,,,,,######,,,,,P,,,,,,,######",
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
    b.props.update(nama='Lorong Lumbung', musik='lumbung', gelap=0.78, battleback='assets/battlebacks/lumbung.png')
    marks = {
        'L': lambda b, x, y: b.obj('loot', x, y, item=('sukma_kecil' if y < 10 else 'abu_pulang')),
        'B': lambda b, x, y: (b.arch(x, y, to=SEL_TUMBAL, spawn='dari_lorong'),
                              b.obj('spawn', x, y, name='dari_sel', dir=0)),
        'A': lambda b, x, y: (b.obj('tungku', x, y, name='tungku_lumbung'), b.block(x, y)),
        'G': lambda b, x, y: (b.arch(x, y, to=DESA, spawn='dari_lumbung'),
                              b.obj('spawn', x, y, name='dari_desa', dir=0)),
        'R': lambda b, x, y: b.obj('enemy', x, y, troop=TR_TIKUS_2, walker='assets/chars/tikus.png'),
        'P': lambda b, x, y: b.obj('enemy', x, y, troop=TR_PENJAGA, walker='assets/chars/penjaga_lumbung.png'),
    }
    b.build_dungeon(marks)
    # kursi tengkorak & patung di aula atas
    b.deco(11, 3, 100); b.deco(11, 4, 108, solid=True)
    b.deco(27, 3, 97); b.deco(27, 4, 105, solid=True)
    b.deco(8, 2, 57); b.deco(25, 2, 56); b.deco(33, 2, 58); b.deco(33, 1, 58, layer='tembok')
    b.deco(20, 6, 64); b.deco(30, 14, 65)
    b.obj('event', 8, 2, name='goresan', event='goresan_dinding', trigger='action')
    b.obj('event', 30, 10, name='aula_peringatan', event='aula_peringatan', trigger='touch', once='aula_peringatan')
    b.save('lorong_lumbung.tmj')


def map_desa():
    W, H = 46, 34
    g = [['"'] * W for _ in range(H)]

    def fill(x0, y0, x1, y1, c):
        for yy in range(y0, y1 + 1):
            for xx in range(x0, x1 + 1):
                if 0 <= xx < W and 0 <= yy < H:
                    g[yy][xx] = c

    # hutan rapat di tepi (celah timur ke ladang, celah selatan ke hutan bambu)
    fill(0, 0, W - 1, 2, '%'); fill(0, H - 3, W - 1, H - 1, '%')
    fill(0, 0, 2, H - 1, '%'); fill(W - 3, 0, W - 1, H - 1, '%')
    fill(W - 3, 16, W - 1, 18, '"')          # celah timur
    fill(21, H - 3, 23, H - 1, '"')          # celah selatan
    # jalan tanah
    fill(8, 10, 9, 14, '.')                  # dari pintu lumbung turun
    fill(8, 13, 18, 14, '.')                 # ke barat alun-alun
    fill(17, 12, 29, 21, '.')                # alun-alun
    fill(29, 16, W - 1, 18, '.')             # ke timur (ladang)
    fill(21, 21, 23, H - 1, '.')             # ke selatan
    fill(30, 11, 31, 16, '.')                # ke rumah panggung
    fill(11, 18, 17, 19, '.'); fill(11, 18, 12, 24, '.')   # ke rumah tua
    # kolam lumpur & semak
    fill(12, 4, 15, 7, '~'); fill(35, 21, 39, 24, '~'); fill(4, 28, 7, 30, '~')
    fill(25, 4, 27, 5, '&'); fill(38, 27, 41, 28, '&'); fill(15, 27, 17, 28, '&')
    fill(3, 15, 4, 17, '&')

    g[11][8] = 'S'                            # tiba dari lumbung
    g[16][23] = 'T'                           # Tungku alun-alun
    g[15][21] = 'B'                           # Mbok Srini
    g[17][32] = 'W'                           # warga di jalan timur
    g[11][28] = 'P'                           # Empu Karta di depan gubuk tempa
    g[20][19] = 'D'                           # pedagang keliling di sudut alun-alun
    g[17][W - 4] = 'E'                        # tiba dari ladang
    g[H - 4][22] = 'J'                        # ujung jalan selatan
    g[20][26] = 'L'; g[25][36] = 'L'; g[6][20] = 'L'
    g[24][28] = 'M'; g[9][42] = 'M'; g[27][10] = 'M'   # (42,9): di luar tapak rumah limasan
    rows = [''.join(r) for r in g]

    b = MapBuilder('Desa Karangtumbal', rows, ['rumput', 'tanah', 'semak', 'hutan', 'lumpur', 'dekor', 'tabrak'])
    b.props.update(nama='Desa Karangtumbal', musik='desa', gelap=0.62, battleback='assets/battlebacks/hutan.png')
    loots = {(26, 20): 'jamu', (36, 25): 'sukma_kecil', (20, 6): 'penawar'}
    marks = {
        'S': lambda b, x, y: b.obj('spawn', x, y, name='dari_lumbung', dir=0),
        'T': lambda b, x, y: (b.obj('tungku', x, y, name='tungku_desa'), b.block(x, y)),
        'B': lambda b, x, y: (b.obj('npc', x, y, name='mbok_srini', event='mbok_srini',
                                    walker='assets/chars/mbok_srini.png', dir=0), b.block(x, y)),
        'P': lambda b, x, y: (b.obj('npc', x, y, name='pandai_besi', event='pandai_besi',
                                    walker='assets/chars/empu_karta.png', dir=0), b.block(x, y)),
        'D': lambda b, x, y: (b.obj('npc', x, y, name='pedagang', event='pedagang',
                                    walker='assets/chars/pedagang.png', dir=0), b.block(x, y)),
        'W': lambda b, x, y: (b.obj('npc', x, y, name='warga_lesung', event='warga_lesung',
                                    walker='assets/chars/warga_lelaki.png', dir=1), b.block(x, y)),
        'E': lambda b, x, y: (b.obj('spawn', x, y, name='dari_ladang', dir=1),
                              [b.obj('door', W - 1, yy, to=LADANG, spawn='dari_desa') for yy in (16, 17, 18)]),
        'J': lambda b, x, y: (b.obj('spawn', x, y - 1, name='dari_bambu', dir=3),
                              [b.obj('door', xx, H - 1, to=HUTAN_BAMBU, spawn='dari_desa', locked=True)
                               for xx in (21, 22, 23)]),
        'L': lambda b, x, y: b.obj('loot', x, y, item=loots.get((x, y), 'sukma_kecil')),
        'M': lambda b, x, y: b.obj('enemy', x, y, troop=TR_TUMBAL_TIKUS if x > 20 else TR_TUMBAL_GAGAL,
                                   walker='assets/chars/tumbal_gagal.png', dir=0),
    }
    b.build_outdoor(marks)

    # bangunan & properti (gambar besar, diurut y saat digambar)
    lw, lh = b.prop('lumbung.png', 4, 9, foot=(0, 9, 3))
    # pintu lumbung = tile gerbang di baris bawah bangunan
    b.obj('door', 8, 9, to=LORONG_LUMBUNG, spawn='dari_desa', dir=1)
    # rumah adat Jawa (limasan) dan rumah kampung tua yang runtuh
    b.prop('rumah_limasan.png', 30, 10, foot=(0, 8, 3))
    b.prop('rumah_jawa_tua.png', 4, 26, foot=(0, 8, 3))
    b.prop('gubuk.png', 34, 30, foot=(0, 5, 2))
    b.prop('gubuk.png', 25, 9, foot=(0, 5, 2))
    for (tx, ty, img) in [(18, 8, 'pohon_mati_a.png'), (40, 12, 'pohon_mati_b.png'), (5, 21, 'pohon_mati_c.png'),
                          (27, 29, 'pohon_mati_a.png'), (14, 30, 'pohon_mati_b.png'), (39, 6, 'pohon_mati_c.png')]:
        iw, _ = b.prop(img, tx, ty, foot=(1, 1, 1))
    b.prop('orang_orangan.png', 39, 15, foot=(1, 2, 1))
    b.prop('orang_orangan.png', 14, 22, foot=(1, 2, 1))
    b.prop('pagar.png', 33, 19, solid=False)
    b.prop('pagar.png', 10, 16, solid=False)
    b.prop('alang_alang.png', 11, 8, solid=False)
    b.prop('alang_alang.png', 35, 25, solid=False)
    b.prop('rak_bambu.png', 28, 22, foot=(0, 2, 1))
    b.scatter(grass=DEKOR_SEMANGGI * 3 + DEKOR_DAUN + DEKOR_BATU, path=DEKOR_DAUN)
    b.save('desa.tmj')


def map_ladang():
    W, H = 52, 36
    g = [['"'] * W for _ in range(H)]

    def fill(x0, y0, x1, y1, c):
        for yy in range(y0, y1 + 1):
            for xx in range(x0, x1 + 1):
                if 0 <= xx < W and 0 <= yy < H:
                    g[yy][xx] = c

    fill(0, 0, W - 1, 2, '%'); fill(0, H - 3, W - 1, H - 1, '%')
    fill(0, 0, 2, H - 1, '%'); fill(W - 3, 0, W - 1, H - 1, '%')
    fill(0, 16, 2, 18, '"')                   # celah barat ke desa
    fill(W - 3, 8, W - 1, 10, '"')            # celah timur ke hutan bambu (terbuka setelah Memedi tumbang)
    # petak ladang (tanah digarap)
    fill(6, 5, 18, 13, '.'); fill(22, 5, 30, 13, '.')
    fill(6, 24, 18, 31, '.'); fill(22, 24, 34, 31, '.')
    # parit lumpur memanjang dengan satu jembatan tanah
    fill(5, 20, 44, 21, '~'); fill(24, 20, 26, 21, '"')
    # lapangan Memedi
    fill(34, 6, 46, 16, '.')
    fill(46, 8, W - 1, 10, '.')
    # semak liar
    fill(40, 25, 45, 28, '&'); fill(19, 14, 21, 16, '&')

    g[17][3] = 'S'                            # tiba dari desa
    g[27][20] = 'T'                           # Tungku di gubuk sawah
    g[9][W - 4] = 'E'                         # ujung timur
    g[16][10] = 'M'; g[9][26] = 'M'; g[28][12] = 'M'; g[27][30] = 'M'; g[17][36] = 'M'
    g[11][14] = 'K'                           # petani mati (loot + cerita)
    g[30][28] = 'L'; g[7][8] = 'L'; g[25][44] = 'L'
    rows = [''.join(r) for r in g]

    b = MapBuilder('Ladang Timun Busuk', rows, ['rumput', 'tanah', 'semak', 'hutan', 'lumpur', 'dekor', 'tabrak'])
    b.props.update(nama='Ladang Timun Busuk', musik='ladang', gelap=0.58, kabut=0.15, battleback='assets/battlebacks/hutan.png')
    troops = {(10, 16): TR_BELALANG_2, (26, 9): TR_TIMUN_BELALANG, (12, 28): TR_TIMUN_2,
              (30, 27): TR_BELALANG_2, (36, 17): TR_TIMUN_BELALANG}
    walkers = {TR_BELALANG_2: 'assets/chars/belalang.png', TR_TIMUN_2: 'assets/chars/timun_busuk.png',
               TR_TIMUN_BELALANG: 'assets/chars/timun_busuk.png'}
    loots = {(28, 30): 'jamu', (8, 7): 'sukma_kecil', (44, 25): 'bom_minyak'}
    marks = {
        'S': lambda b, x, y: (b.obj('spawn', x, y, name='dari_desa', dir=2),
                              b.obj('event', 0, 0, name='ladang_masuk', event='ladang_masuk', trigger='auto',
                                    once='ladang_masuk'),
                              [b.obj('door', 0, yy, to=DESA, spawn='dari_ladang') for yy in (16, 17, 18)]),
        'T': lambda b, x, y: (b.obj('tungku', x, y, name='tungku_ladang'), b.block(x, y)),
        'E': lambda b, x, y: (b.obj('spawn', x, y, name='dari_bambu', dir=1),
                              [b.obj('door', W - 1, yy, to=HUTAN_BAMBU, spawn='dari_ladang', locked=True,
                                     pesan='Sulur timun yang menggeliat menutup jalan. Sesuatu di tengah ladang menahannya.')
                               for yy in (8, 9, 10)]),
        'M': lambda b, x, y: b.obj('enemy', x, y, troop=troops.get((x, y), TR_BELALANG_2),
                                   walker=walkers.get(troops.get((x, y), TR_BELALANG_2)), dir=0),
        'K': lambda b, x, y: b.obj('loot', x, y, event='orang_mati_ladang'),
        'L': lambda b, x, y: b.obj('loot', x, y, item=loots.get((x, y), 'sukma_kecil')),
    }
    b.build_outdoor(marks)

    # ajir timun berbaris di petak ladang (tapak 1 tile di kaki tiap ajir)
    for (x0, y0, x1, y1) in [(6, 5, 18, 13), (22, 5, 30, 13), (6, 24, 18, 31), (22, 24, 34, 31)]:
        for yy in range(y0 + 3, y1 + 1, 4):
            for xx in range(x0, x1 - 1, 3):
                if (xx, yy) in [(9, 8), (24, 12), (12, 31), (31, 27)]:
                    b.prop('ajir_roboh.png', xx, yy, foot=(0, 3, 1))
                else:
                    b.prop('ajir.png', xx, yy, foot=(0, 2, 1))
    b.prop('gubuk_sawah.png', 16, 27, foot=(0, 4, 2))
    b.prop('memedi_peta.png', 39, 10, foot=(0, 0, 0), solid=False, name='memedi')
    b.objects[-1]['props']['hideIf'] = 'memedi_mati'
    b.objects[-1]['props']['glow'] = 70
    # pemicu: mendekati Memedi
    b.obj('event', 37, 12, name='memedi', event='memedi_bangun', trigger='touch', w=5, once='memedi_mati')
    b.block(40, 10); b.block(41, 10)
    for (tx, ty, img) in [(27, 33, 'timun_tumpuk.png'), (19, 12, 'timun_tumpuk.png'), (33, 9, 'timun_tumpuk.png'),
                          (21, 27, 'ember.png'), (22, 28, 'cangkul.png')]:
        b.prop(img, tx, ty, solid=False)
    b.prop('pagar_bambu.png', 31, 14, foot=(0, 3, 1))
    b.prop('pagar_bambu.png', 43, 17, foot=(0, 3, 1))
    b.prop('orang_orangan.png', 4, 12, foot=(1, 2, 1))
    b.prop('pohon_mati_c.png', 46, 30, foot=(1, 1, 1))
    b.prop('pohon_mati_a.png', 3, 28, foot=(1, 1, 1))
    b.scatter(grass=DEKOR_SEMANGGI * 3 + DEKOR_DAUN + DEKOR_BATU, path=DEKOR_DAUN + (DEKOR_RANTING,), path_rate=0.06)
    b.save('ladang.tmj')


def map_hutan_bambu():
    W, H = 46, 34
    g = [['#'] * W for _ in range(H)]

    def carve(x0, y0, x1, y1, c='"'):
        for yy in range(y0, y1 + 1):
            for xx in range(x0, x1 + 1):
                if 0 <= xx < W and 0 <= yy < H:
                    g[yy][xx] = c

    # lorong berkelok di antara rumpun bambu
    carve(0, 20, 9, 22)                    # masuk dari ladang (barat)
    carve(7, 13, 9, 22)
    carve(7, 13, 20, 15)
    carve(18, 6, 20, 15)
    carve(18, 6, 34, 8)
    carve(24, 10, 30, 16, '.')             # lapangan Tungku
    carve(22, 8, 24, 11)
    carve(30, 14, 40, 16)
    carve(38, 14, 40, 28)
    carve(26, 26, 40, 28)
    carve(26, 22, 28, 28)
    carve(12, 22, 28, 24)
    carve(12, 24, 14, 31)                  # ke selatan: laut garam
    carve(12, 30, 14, H - 1)
    carve(32, 3, 34, 8)                    # utara: jalan pintas ke desa
    carve(32, 0, 34, 3)
    carve(16, 25, 20, 29, '.')             # makam jarum
    g[21][1] = 'S'
    g[13][28] = 'T'
    g[4][33] = 'U'                         # pintu desa (jalan pintas)
    g[H - 2][13] = 'L'                     # ke laut
    g[7][24] = 'M'; g[15][38] = 'M'; g[27][33] = 'M'; g[23][20] = 'M'; g[14][12] = 'M'
    g[27][18] = 'J'
    g[7][30] = 'K'; g[27][39] = 'K'; g[23][15] = 'K'
    rows = [''.join(r) for r in g]
    b = MapBuilder('Hutan Bambu Jarum', rows, ['rumput', 'tanah', 'semak', 'hutan', 'lumpur', 'bambu', 'kanopi', 'dekor', 'tabrak'])
    b.props.update(nama='Hutan Bambu Jarum', musik='lumbung', gelap=0.7, kabut=0.25, battleback='assets/battlebacks/hutan_bambu.png')
    troops = {(24, 7): TR_ARWAH_2, (38, 15): TR_GAGAK, (33, 27): TR_ULAR_ARWAH, (20, 23): TR_GAGAK, (12, 14): TR_ARWAH_2}
    walk = {TR_ARWAH_2: 'assets/chars/arwah.png', TR_GAGAK: 'assets/chars/gagak.png', TR_ULAR_ARWAH: 'assets/chars/ular.png'}
    loots = {(30, 7): 'jamu', (39, 27): 'sukma_sedang', (15, 23): 'bijih_besi'}
    marks = {
        'S': lambda b, x, y: (b.obj('spawn', x, y, name='dari_ladang', dir=2),
                              [b.obj('door', 0, yy, to=LADANG, spawn='dari_bambu') for yy in (20, 21, 22)],
                              b.obj('event', 0, 0, name='bambu_masuk', event='bambu_masuk', trigger='auto', once='bambu_masuk')),
        'T': lambda b, x, y: (b.obj('tungku', x, y, name='tungku_bambu'), b.block(x, y)),
        'U': lambda b, x, y: (b.obj('spawn', x, y, name='dari_desa', dir=0),
                              [b.obj('door', xx, 0, to=DESA, spawn='dari_bambu') for xx in (32, 33, 34)],
                              b.obj('event', x - 1, y + 1, name='tebas_bambu', event='tebas_bambu', trigger='touch', w=3,
                                    once='jalan_pintas_desa')),
        'L': lambda b, x, y: (b.obj('spawn', x, y - 1, name='dari_laut', dir=3),
                              [b.obj('door', xx, H - 1, to=LAUT_GARAM, spawn='dari_bambu') for xx in (12, 13, 14)]),
        'M': lambda b, x, y: b.obj('enemy', x, y, troop=troops.get((x, y), TR_ARWAH_2),
                                   walker=walk.get(troops.get((x, y), TR_ARWAH_2)), dir=0),
        'J': lambda b, x, y: b.obj('loot', x, y, event='makam_jarum'),
        'K': lambda b, x, y: b.obj('loot', x, y, item=loots.get((x, y), 'sukma_kecil')),
    }
    b.build_outdoor(marks)
    b.build_grove()
    for (tx, ty, img) in [(26, 12, 'bambu_tinggi_a.png'), (29, 16, 'bambu_tinggi_b.png'), (16, 29, 'bambu_tinggi_a.png'),
                          (20, 26, 'bambu_tinggi_b.png'), (39, 22, 'bambu_tinggi_a.png')]:
        b.prop(img, tx, ty, foot=(0, 1, 1))
    b.prop('orang_orangan.png', 17, 26, foot=(1, 2, 1))
    b.scatter(grass=DEKOR_DAUN * 2 + DEKOR_SEMANGGI + (DEKOR_RANTING, DEKOR_TULANG), path=DEKOR_DAUN, grass_rate=0.14)
    b.save('hutan_bambu.tmj')


def map_laut_garam():
    W, H = 46, 30
    g = [['"'] * W for _ in range(H)]

    def fill(x0, y0, x1, y1, c):
        for yy in range(y0, y1 + 1):
            for xx in range(x0, x1 + 1):
                if 0 <= xx < W and 0 <= yy < H:
                    g[yy][xx] = c

    fill(0, 0, W - 1, 6, '~')                    # laut di utara
    fill(30, 0, W - 1, 14, '~')                  # teluk timur
    fill(36, 15, W - 1, 18, '~')
    fill(0, 7, 1, H - 1, '%'); fill(0, H - 2, W - 1, H - 1, '%')
    g[8][3] = 'S'
    g[3][19] = 'T'                               # Timun Mas di ujung dermaga
    g[H - 3][28] = 'R'                           # ke rawa
    g[14][8] = 'M'; g[18][30] = 'M'; g[24][14] = 'M'; g[12][24] = 'M'
    g[20][6] = 'K'; g[10][28] = 'K'; g[25][38] = 'K'
    rows = [''.join(r) for r in g]
    b = MapBuilder('Laut Garam', rows, ['pasir', 'tanah', 'semak', 'hutan', 'laut', 'dekor', 'tabrak'])
    b.base_ts = 'pasir'
    b.water_ts = 'laut'
    b.props.update(nama='Laut Garam', musik='laut', gelap=0.55, kabut=0.5, battleback='assets/battlebacks/pantai_garam.png')
    troops = {(8, 14): TR_HANTU_LAUT, (30, 18): TR_TENGKORAK_2, (14, 24): TR_HANTU_TENGKORAK, (24, 12): TR_TENGKORAK_2}
    walk = {TR_HANTU_LAUT: 'assets/chars/arwah.png', TR_TENGKORAK_2: 'assets/chars/tumbal_gagal.png',
            TR_HANTU_TENGKORAK: 'assets/chars/tumbal_gagal.png'}
    loots = {(6, 20): 'bijih_besi', (28, 10): 'jamu', (38, 25): 'sukma_sedang'}
    marks = {
        'S': lambda b, x, y: (b.obj('spawn', x, y, name='dari_bambu', dir=0),
                              b.obj('event', 0, 0, name='laut_masuk', event='laut_masuk', trigger='auto', once='laut_masuk'),
                              [b.obj('door', xx, H - 2, to=HUTAN_BAMBU, spawn='dari_laut') for xx in (2, 3, 4)]),
        'T': lambda b, x, y: (b.obj('npc', x, y, name='timun_mas', event='timun_mas', walker='assets/chars/timun_mas.png',
                                    dir=0, hideIf='timun_bergabung'), b.block(x, y)),
        'R': lambda b, x, y: (b.obj('spawn', x, y - 1, name='dari_rawa', dir=3),
                              [b.obj('door', xx, H - 1, to=RAWA_TERASI, spawn='dari_laut') for xx in (27, 28, 29)]),
        'M': lambda b, x, y: b.obj('enemy', x, y, troop=troops.get((x, y), TR_TENGKORAK_2),
                                   walker=walk.get(troops.get((x, y), TR_TENGKORAK_2)), dir=0),
        'K': lambda b, x, y: b.obj('loot', x, y, item=loots.get((x, y), 'sukma_kecil')),
    }
    # celah hutan selatan-barat untuk pintu dari bambu; pintu ke rawa di tepi selatan
    for xx in (2, 3, 4):
        g[H - 2][xx] = '"'; g[H - 1][xx] = '"'
    for xx in (27, 28, 29):
        g[H - 2][xx] = '"'; g[H - 1][xx] = '"'
    b.rows = [''.join(r) for r in g]
    b.build_outdoor(marks)
    b.prop('rumah_panggung_tua.png', 6, 16, foot=(1, 8, 2))
    b.obj('event', 10, 16, name='rumah_nelayan', event='rumah_nelayan', trigger='action', w=3)
    for (tx, ty, img) in [(16, 18, 'pohon_mati_a.png'), (34, 26, 'pohon_mati_b.png'), (24, 15, 'pohon_mati_c.png')]:
        b.prop(img, tx, ty, foot=(1, 2, 1))
    # dermaga ke utara: papan di atas air bisa dipijak (collision air dibuka)
    b.prop('dermaga.png', 18, 7, solid=False, name='dermaga')
    b.objects[-1]['props']['flat'] = True
    for yy in range(1, 7):
        for xx in (18, 19):
            b.block(xx, yy, False)
    b.prop('tong.png', 16, 8, foot=(0, 1, 1))
    b.prop('tong.png', 21, 9, foot=(0, 1, 1))
    b.prop('jemuran_jala.png', 9, 12, foot=(0, 5, 1))
    b.prop('jemuran_jala.png', 22, 22, foot=(0, 5, 1))
    b.prop('pagar.png', 26, 13, solid=False)
    b.scatter(grass=DEKOR_BATU * 2 + (DEKOR_TULANG, DEKOR_TENGKORAK, DEKOR_RANTING), grass_rate=0.05)
    b.save('laut_garam.tmj')


def map_rawa_terasi():
    W, H = 48, 34
    g = [['"'] * W for _ in range(H)]

    def fill(x0, y0, x1, y1, c):
        for yy in range(y0, y1 + 1):
            for xx in range(x0, x1 + 1):
                if 0 <= xx < W and 0 <= yy < H:
                    g[yy][xx] = c

    fill(0, 0, W - 1, 2, '%'); fill(0, H - 3, W - 1, H - 1, '%')
    fill(0, 0, 2, H - 1, '%'); fill(W - 3, 0, W - 1, H - 1, '%')
    fill(26, 0, 28, 2, '"')                       # utara: dari laut
    fill(6, 6, 16, 12, '~'); fill(30, 8, 42, 14, '~'); fill(8, 20, 20, 27, '~'); fill(30, 22, 40, 28, '~')
    fill(20, 4, 24, 30, '.'); fill(24, 16, 44, 18, '.'); fill(42, 16, 44, 30, '.')
    fill(4, 15, 20, 17, '.')
    fill(42, H - 3, 44, H - 1, '.')               # ke sarang (satu arah)
    g[3][27] = 'S'
    g[16][12] = 'T'
    g[H - 4][43] = 'Z'
    g[10][22] = 'M'; g[17][32] = 'M'; g[25][22] = 'M'; g[22][43] = 'M'; g[16][6] = 'M'
    g[29][23] = 'J'; g[5][29] = 'K'; g[30][12] = 'K'
    rows = [''.join(r) for r in g]
    b = MapBuilder('Rawa Terasi', rows, ['rumput_rawa', 'tanah_rawa', 'semak', 'hutan_rawa', 'lumpur', 'dekor', 'tabrak'])
    b.base_ts, b.path_ts, b.forest_ts = 'rumput_rawa', 'tanah_rawa', 'hutan_rawa'
    b.props.update(nama='Rawa Terasi', musik='rawa', gelap=0.72, kabut=0.55, battleback='assets/battlebacks/rawa.png')
    troops = {(22, 10): TR_AKAR_2, (32, 17): TR_BUAYA, (22, 25): TR_AKAR_BELATUNG, (43, 22): TR_BUAYA, (6, 16): TR_AKAR_2}
    loots = {(29, 5): 'jamu', (12, 30): 'bijih_besi'}
    marks = {
        'S': lambda b, x, y: (b.obj('spawn', x, y, name='dari_laut', dir=0),
                              [b.obj('door', xx, 0, to=LAUT_GARAM, spawn='dari_rawa') for xx in (26, 27, 28)],
                              b.obj('event', 0, 0, name='rawa_masuk', event='rawa_masuk', trigger='auto', once='rawa_masuk')),
        'T': lambda b, x, y: (b.obj('tungku', x, y, name='tungku_rawa'), b.block(x, y)),
        'Z': lambda b, x, y: [b.obj('door', xx, H - 1, to=SARANG_BUTO, spawn='dari_rawa', oneway=True) for xx in (42, 43, 44)],
        'M': lambda b, x, y: b.obj('enemy', x, y, troop=troops.get((x, y), TR_AKAR_2),
                                   walker='assets/chars/timun_busuk.png' if troops.get((x, y)) != TR_BUAYA
                                   else 'assets/chars/tikus.png', dir=0),
        'J': lambda b, x, y: b.obj('loot', x, y, event='terasi_bekal'),
        'K': lambda b, x, y: b.obj('loot', x, y, item=loots.get((x, y), 'sukma_kecil')),
    }
    b.build_outdoor(marks)
    # tunggul raksasa berlubang di timur laut (sebagian tertutup hutan tepi peta)
    b.prop('tunggul_rawa.png', 31, 7, foot=(2, 8, 2))
    for (tx, ty, img) in [(36, 30, 'pohon_mati_b.png'), (25, 26, 'pohon_mati_a.png'), (14, 31, 'pohon_mati_c.png'),
                          (45, 12, 'pohon_mati_a.png'), (16, 19, 'pohon_akar.png'), (3, 13, 'pohon_akar.png')]:
        b.prop(img, tx, ty, foot=(1, 1, 1))
    b.prop('orang_orangan.png', 37, 21, foot=(1, 2, 1))
    b.prop('tiang_tengkorak.png', 41, 30, solid=False)
    b.prop('tiang_tengkorak.png', 45, 30, solid=False)
    b.prop('pagar_tali.png', 4, 14, solid=False)
    # pemuda desa yang disuruh mengintip Buto, berjaga di dekat Tungku rawa
    b.obj('npc', 15, 15, name='pemuda_rawa', event='pemuda_rawa', walker='assets/chars/warga_lelaki.png', dir=0)
    b.block(15, 15)
    b.scatter_props(['gelagah.png', 'gelagah_b.png', 'gelagah.png', 'ranting_mati.png'], 18, avoid=1)
    b.scatter(grass=DEKOR_DAUN * 2 + DEKOR_BATU + (DEKOR_TULANG, DEKOR_TENGKORAK, DEKOR_RANTING), path=DEKOR_DAUN,
              grass_rate=0.14)
    b.save('rawa_terasi.tmj')


def map_sarang_buto():
    W, H = 34, 40
    g = [['%'] * W for _ in range(H)]

    def fill(x0, y0, x1, y1, c):
        for yy in range(y0, y1 + 1):
            for xx in range(x0, x1 + 1):
                if 0 <= xx < W and 0 <= yy < H:
                    g[yy][xx] = c

    fill(14, 0, 19, 15, '"')                       # lereng masuk yang sempit
    fill(4, 15, 29, 36, '"')                       # lembah sarang
    fill(9, 27, 24, 35, '~')                       # kolam terasi tempat Buto tidur
    fill(16, 0, 17, 25, '.')                       # jejak seretan menuju kolam
    fill(3, 20, 4, 30, '%'); fill(29, 18, 30, 31, '%')
    g[2][16] = 'S'
    rows = [''.join(r) for r in g]
    b = MapBuilder('Sarang Buto', rows, ['rumput_sarang', 'tanah_rawa', 'semak', 'hutan_sarang', 'lumpur', 'dekor', 'tabrak'])
    b.base_ts, b.path_ts, b.forest_ts = 'rumput_sarang', 'tanah_rawa', 'hutan_sarang'
    b.props.update(nama='Sarang Buto', musik='rawa', gelap=0.66, kabut=0.6, battleback='assets/battlebacks/sarang_buto.png')
    marks = {
        'S': lambda b, x, y: (b.obj('spawn', x, y, name='dari_rawa', dir=0),
                              b.obj('event', 0, 0, name='sarang_masuk', event='sarang_masuk', trigger='auto',
                                    once='sarang_masuk')),
    }
    b.build_outdoor(marks)
    # Buto: kepala menyembul dari kolam
    b.prop('buto_peta.png', 15, 29, solid=False, name='buto')
    b.objects[-1]['props']['hideIf'] = 'buto_mati'
    b.objects[-1]['props']['glow'] = 110
    b.obj('event', 10, 24, name='buto', event='buto_bangun', trigger='touch', w=14, once='buto_mati')
    # tiang tengkorak berbaris di kiri-kanan lereng: peringatan bagi yang turun
    for yy in (5, 9, 13):
        b.prop('tiang_tengkorak.png', 14, yy, solid=False)
        b.prop('tiang_tengkorak.png', 19, yy, solid=False)
    b.obj('event', 16, 18, name='tumpukan_tulang', event='tumpukan_tulang', trigger='action')
    b.prop('tunggul_rawa.png', -2, 24, foot=(2, 8, 2))
    for (tx, ty, img) in [(25, 20, 'pohon_mati_b.png'), (26, 34, 'pohon_akar.png'), (5, 34, 'pohon_mati_c.png'),
                          (21, 17, 'pohon_mati_a.png')]:
        b.prop(img, tx, ty, foot=(1, 1, 1))
    b.prop('pagar_tali.png', 7, 25, solid=False)
    b.prop('pagar_tali.png', 22, 25, solid=False)
    b.prop('orang_orangan.png', 10, 19, foot=(1, 2, 1))
    b.scatter_props(['gelagah.png', 'gelagah_b.png', 'ranting_mati.png'], 8, avoid=1)
    b.scatter(grass=(DEKOR_TULANG, DEKOR_TULANG, DEKOR_TENGKORAK, DEKOR_RANTING) + DEKOR_DAUN + DEKOR_BATU,
              path=(DEKOR_TULANG, DEKOR_TENGKORAK) + DEKOR_DAUN, grass_rate=0.18, path_rate=0.12)
    b.save('sarang_buto.tmj')


WORLD = {
    'start': {'map': SEL_TUMBAL, 'spawn': 'awal'},
    'maps': [
        {'id': SEL_TUMBAL, 'name': 'Sel Tumbal', 'file': 'sel_tumbal.tmj'},
        {'id': LORONG_LUMBUNG, 'name': 'Lorong Lumbung', 'file': 'lorong_lumbung.tmj'},
        {'id': DESA, 'name': 'Desa Karangtumbal', 'file': 'desa.tmj'},
        {'id': LADANG, 'name': 'Ladang Timun Busuk', 'file': 'ladang.tmj'},
        {'id': HUTAN_BAMBU, 'name': 'Hutan Bambu Jarum', 'file': 'hutan_bambu.tmj'},
        {'id': LAUT_GARAM, 'name': 'Laut Garam', 'file': 'laut_garam.tmj'},
        {'id': RAWA_TERASI, 'name': 'Rawa Terasi', 'file': 'rawa_terasi.tmj'},
        {'id': SARANG_BUTO, 'name': 'Sarang Buto', 'file': 'sarang_buto.tmj'},
    ],
}

if __name__ == '__main__':
    os.makedirs(MAPS_DIR, exist_ok=True)
    ensure_collision_png()
    map_sel_tumbal()
    map_lorong_lumbung()
    map_desa()
    map_ladang()
    map_hutan_bambu()
    map_laut_garam()
    map_rawa_terasi()
    map_sarang_buto()
    with open(os.path.join(MAPS_DIR, 'world.json'), 'w', encoding='utf-8') as f:
        json.dump(WORLD, f, indent=2, ensure_ascii=False)
    print('peta selesai:', sorted(os.listdir(MAPS_DIR)))
