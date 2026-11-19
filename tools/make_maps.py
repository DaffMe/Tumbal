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
}
ANIMATED = {'lumpur': (4, 48, 260)}                 # frame, tile per frame, durasi ms
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
        # penanda (huruf) di atas jalan/lumpur: medan di bawahnya ikut tetangga
        terrain = '".~&%'
        for y in range(self.h):
            row = list(self.rows[y])
            for x in range(self.w):
                if row[x] in terrain:
                    continue
                around = [self.ch(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
                row[x] = '.' if around.count('.') >= 2 else '"'
            self.rows[y] = ''.join(row)
        AUTO = {'.': ('tanah', 'tanah', False), '~': ('air', 'lumpur', True),
                '&': ('semak', 'semak', True), '%': ('semak', 'hutan', True)}
        for y in range(self.h):
            for x in range(self.w):
                self.set('lantai', x, y, 'rumput', full)
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

# id peta (sama dengan world.json)
SEL_TUMBAL, LORONG_LUMBUNG, DESA, LADANG = 1, 2, 3, 4


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
    g[17][W - 4] = 'E'                        # tiba dari ladang
    g[H - 4][22] = 'J'                        # ujung jalan selatan
    g[20][26] = 'L'; g[25][36] = 'L'; g[6][20] = 'L'
    g[24][28] = 'M'; g[9][35] = 'M'; g[27][10] = 'M'
    rows = [''.join(r) for r in g]

    b = MapBuilder('Desa Karangtumbal', rows, ['rumput', 'tanah', 'semak', 'hutan', 'lumpur', 'tabrak'])
    b.props.update(nama='Desa Karangtumbal', musik='desa', gelap=0.62, battleback='assets/battlebacks/hutan.png')
    loots = {(26, 20): 'jamu', (36, 25): 'sukma_kecil', (20, 6): 'penawar'}
    marks = {
        'S': lambda b, x, y: b.obj('spawn', x, y, name='dari_lumbung', dir=0),
        'T': lambda b, x, y: (b.obj('tungku', x, y, name='tungku_desa'), b.block(x, y)),
        'B': lambda b, x, y: (b.obj('npc', x, y, name='mbok_srini', event='mbok_srini',
                                    walker='assets/chars/mbok_srini.png', dir=0), b.block(x, y)),
        'W': lambda b, x, y: (b.obj('npc', x, y, name='warga_lesung', event='warga_lesung',
                                    walker='assets/chars/warga_lelaki.png', dir=1), b.block(x, y)),
        'E': lambda b, x, y: (b.obj('spawn', x, y, name='dari_ladang', dir=1),
                              [b.obj('door', W - 1, yy, to=LADANG, spawn='dari_desa') for yy in (16, 17, 18)]),
        'J': lambda b, x, y: b.obj('event', x, y, name='jalan_bambu', event='jalan_bambu', trigger='touch', w=3),
        'L': lambda b, x, y: b.obj('loot', x, y, item=loots.get((x, y), 'sukma_kecil')),
        'M': lambda b, x, y: b.obj('enemy', x, y, troop=TR_TUMBAL_TIKUS if x > 20 else TR_TUMBAL_GAGAL,
                                   walker='assets/chars/tumbal_gagal.png', dir=0),
    }
    b.build_outdoor(marks)

    # bangunan & properti (gambar besar, diurut y saat digambar)
    lw, lh = b.prop('lumbung.png', 4, 9, foot=(0, 9, 3))
    # pintu lumbung = tile gerbang di baris bawah bangunan
    b.obj('door', 8, 9, to=LORONG_LUMBUNG, spawn='dari_desa', dir=1)
    b.prop('rumah_panggung.png', 30, 10, foot=(1, 8, 3))
    b.prop('rumah_panggung_tua.png', 4, 26, foot=(1, 8, 3))
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
    fill(W - 3, 8, W - 1, 10, '"')            # celah timur (hutan bambu, belum dibuka)
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

    b = MapBuilder('Ladang Timun Busuk', rows, ['rumput', 'tanah', 'semak', 'hutan', 'lumpur', 'tabrak'])
    b.props.update(nama='Ladang Timun Busuk', musik='ladang', gelap=0.58, battleback='assets/battlebacks/hutan.png')
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
        'E': lambda b, x, y: b.obj('event', x, y - 1, name='ujung_ladang', event='ujung_ladang', trigger='touch', h=3),
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
    b.save('ladang.tmj')


WORLD = {
    'start': {'map': SEL_TUMBAL, 'spawn': 'awal'},
    'maps': [
        {'id': SEL_TUMBAL, 'name': 'Sel Tumbal', 'file': 'sel_tumbal.tmj'},
        {'id': LORONG_LUMBUNG, 'name': 'Lorong Lumbung', 'file': 'lorong_lumbung.tmj'},
        {'id': DESA, 'name': 'Desa Karangtumbal', 'file': 'desa.tmj'},
        {'id': LADANG, 'name': 'Ladang Timun Busuk', 'file': 'ladang.tmj'},
    ],
}

if __name__ == '__main__':
    os.makedirs(MAPS_DIR, exist_ok=True)
    ensure_collision_png()
    map_sel_tumbal()
    map_lorong_lumbung()
    map_desa()
    map_ladang()
    with open(os.path.join(MAPS_DIR, 'world.json'), 'w', encoding='utf-8') as f:
        json.dump(WORLD, f, indent=2, ensure_ascii=False)
    print('peta selesai:', sorted(os.listdir(MAPS_DIR)))
