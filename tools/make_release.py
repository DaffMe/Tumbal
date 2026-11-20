"""Susun paket rilis Windows TUMBAL -> release/TUMBAL-v<versi>-windows.zip.

Dipanggil `mingw32-make dist` setelah exe rilis & DLL di-build. Hanya memakai pustaka standar Python.
Isi paket (folder TUMBAL/):
    TUMBAL.exe            game (tanpa jendela console)
    TUMBAL-console.exe    mode teks (logika yang sama)
    *.dll                 raylib, GLFW, winpthread
    assets/               hanya aset yang dipakai saat bermain (tanpa assets/src & assets/raw)
    CARA MAIN.txt         petunjuk singkat (Indonesia + English)
    CREDITS.md            kredit & lisensi aset
"""
import os
import shutil
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = sys.argv[1] if len(sys.argv) > 1 else '1.0.0'
OUT = os.path.join(ROOT, 'release')
PKG = os.path.join(OUT, 'TUMBAL')
ASSET_DIRS = ['audio', 'battlebacks', 'battlers', 'chars', 'events', 'faces', 'fonts', 'icon', 'lang', 'maps',
              'props', 'tiles']
DLLS = ['libraylib.dll', 'glfw3.dll', 'libwinpthread-1.dll']

README = f"""TUMBAL v{VERSION}
==============================

[ID] Cara main
--------------
1. Ekstrak seluruh isi zip ini ke satu folder (jangan dijalankan langsung dari dalam zip).
2. Klik dua kali TUMBAL.exe.
3. Bahasa bawaan adalah English. Untuk Bahasa Indonesia: Settings di layar judul, atau menu X saat bermain.

Kontrol: Panah/WASD jalan, Shift lari, Z/Enter pilih & periksa, X/Esc batal & menu,
tahan Z saat battle untuk mempercepat, F1 Inspector struktur data, F9 menu cheat.
Game tidak menyimpan otomatis saat mati: simpan manual lewat menu (3 slot).
File save tersimpan di folder "saves" di samping TUMBAL.exe.

TUMBAL-console.exe = versi teks di terminal dengan logika game yang sama.

Jika Windows SmartScreen muncul ("Windows protected your PC"): klik "More info" lalu "Run anyway".
Peringatan ini muncul karena exe belum ditandatangani secara digital, bukan karena virus.

[EN] How to play
----------------
1. Extract the whole zip into a folder (do not run it from inside the zip).
2. Double-click TUMBAL.exe.
3. Controls: Arrows/WASD move, Shift run, Z/Enter confirm, X/Esc cancel & menu, F9 cheats.
   Save from the menu; the game never saves on death. Saves go to the "saves" folder.

Requirements: Windows 10/11 (64-bit).
Source code & info: https://github.com/DaffMe/Tumbal
Asset credits & licenses: CREDITS.md
"""


def main():
    if os.path.exists(PKG):
        shutil.rmtree(PKG)
    os.makedirs(PKG)
    bin_dir = os.path.join(ROOT, 'bin')
    shutil.copy2(os.path.join(bin_dir, 'rel_gui.exe'), os.path.join(PKG, 'TUMBAL.exe'))
    shutil.copy2(os.path.join(bin_dir, 'rel_console.exe'), os.path.join(PKG, 'TUMBAL-console.exe'))
    for d in DLLS:
        shutil.copy2(os.path.join(bin_dir, d), os.path.join(PKG, d))
    for d in ASSET_DIRS:
        shutil.copytree(os.path.join(ROOT, 'assets', d), os.path.join(PKG, 'assets', d))
    shutil.copy2(os.path.join(ROOT, 'CREDITS.md'), os.path.join(PKG, 'CREDITS.md'))
    with open(os.path.join(PKG, 'CARA MAIN.txt'), 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(README)

    zip_path = os.path.join(OUT, f'TUMBAL-v{VERSION}-windows.zip')
    if os.path.exists(zip_path):
        os.remove(zip_path)
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for base, _, files in os.walk(PKG):
            for name in files:
                full = os.path.join(base, name)
                z.write(full, os.path.relpath(full, OUT))
    size = os.path.getsize(zip_path) / (1024 * 1024)
    print(f'rilis: {zip_path} ({size:.1f} MB)')


if __name__ == '__main__':
    main()
