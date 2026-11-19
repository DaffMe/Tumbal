"""Musik Desa Karangtumbal: gending gamelan pelog yang muram, disintesis sendiri (tanpa sampel).

Komposisi orisinal untuk TUMBAL. Instrumen dimodelkan dengan parsial tak-harmonis bilah logam
(saron, kenong, gong) dan pasangan nada yang sedikit dilaras berbeda (ombak/beating), ditambah
bunyi lesung kayu "tok... tok... tok..." yang menjadi motif cerita desa.

Hasil: assets/audio/music/desa.ogg (loop mulus). Butuh numpy dan ffmpeg (untuk OGG Vorbis).
    /c/msys64/ucrt64/bin/python.exe tools/make_gamelan.py
"""
import os
import subprocess
import sys
import wave

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'assets', 'audio', 'music', 'desa.ogg')
SR = 22050
BEAT = 1.25                     # detik per ketukan balungan (irama lambat)
BEATS = 64                      # 4 gongan x 16 ketukan
LENGTH = BEAT * BEATS           # 80 detik, lalu diulang
rng = np.random.default_rng(1)

# laras pelog (sen, dibulatkan dari laras pelog rata-rata), nada 1..7
PELOG_CENTS = [0, 120, 270, 540, 670, 785, 950]
BASE = 233.0


def freq(note, octave=0):
    return BASE * 2 ** ((PELOG_CENTS[note - 1] + 1200 * octave) / 1200)


def bar(f, dur, partials, decays, amps, detune=0.0):
    """Bilah/gong logam: jumlah sinus tak-harmonis dengan peluruhan berbeda; detune = ombak."""
    t = np.arange(int(dur * SR)) / SR
    y = np.zeros_like(t)
    for p, d, a in zip(partials, decays, amps):
        for dt in (0.0, detune):
            y += a * np.sin(2 * np.pi * (f * p + dt) * t) * np.exp(-t / d)
    attack = np.minimum(1.0, t / 0.004)
    return y * attack


def saron(f):
    return 0.22 * bar(f, 2.4, [1.0, 2.76, 5.4], [0.9, 0.35, 0.12], [1.0, 0.35, 0.12], detune=1.6)


def kenong(f):
    return 0.18 * bar(f, 4.0, [1.0, 1.52, 2.0, 3.0], [2.2, 1.0, 0.8, 0.4], [1.0, 0.25, 0.3, 0.1], detune=0.9)


def gong():
    return 0.55 * bar(62.0, 9.0, [1.0, 1.48, 2.0, 2.76, 4.1], [5.5, 3.0, 2.2, 1.4, 0.7],
                      [1.0, 0.35, 0.45, 0.2, 0.08], detune=0.55)


def lesung():
    """Alu menumbuk lesung kayu: dentum rendah + derak kayu pendek."""
    n = int(0.45 * SR)
    t = np.arange(n) / SR
    thud = np.sin(2 * np.pi * 95 * t * (1 - 0.25 * t)) * np.exp(-t / 0.07)
    knock = np.sin(2 * np.pi * 410 * t) * np.exp(-t / 0.025) * 0.6
    noise = rng.standard_normal(n) * np.exp(-t / 0.012) * 0.35
    return 0.42 * (thud + knock + noise)


def place(track, sound, at):
    i = int(at * SR) % len(track)
    end = i + len(sound)
    if end <= len(track):
        track[i:end] += sound
    else:                                    # bungkus ke awal agar loop mulus
        k = len(track) - i
        track[i:] += sound[:k]
        track[:end - len(track)] += sound[k:]


def reverb(x, seconds=2.6, mix=0.32):
    n = int(seconds * SR)
    t = np.arange(n) / SR
    ir = rng.standard_normal(n) * np.exp(-t / (seconds / 4.5))
    ir[: int(0.02 * SR)] = 0
    ir /= np.sqrt(np.sum(ir ** 2))
    size = 1 << int(np.ceil(np.log2(len(x) + n)))
    wet = np.fft.irfft(np.fft.rfft(x, size) * np.fft.rfft(ir, size), size)[: len(x) + n]
    # ekor gema dibungkus ke awal (loop)
    out = wet[: len(x)].copy()
    out[: n] += wet[len(x):len(x) + n]
    return (1 - mix) * x + mix * out


def main():
    total = int(LENGTH * SR)
    track = np.zeros(total)
    # balungan (orisinal): dua frasa yang turun-naik pelan, berakhir di nada 6 lalu 1 (gong)
    balungan = [
        3, 2, 3, 1, 3, 2, 1, 6, 5, 6, 1, 2, 1, 6, 5, 6,
        2, 1, 2, 3, 5, 3, 2, 1, 6, 5, 3, 5, 6, 5, 3, 2,
        3, 2, 3, 1, 3, 2, 1, 6, 5, 6, 1, 2, 1, 6, 5, 6,
        5, 6, 5, 3, 2, 1, 2, 3, 1, 2, 6, 5, 3, 5, 6, 1,
    ]
    for b, note in enumerate(balungan):
        at = b * BEAT
        octave = -1 if note >= 5 else 0
        place(track, saron(freq(note, octave)), at + rng.normal(0, 0.008))
        # peking: pukulan rangkap setengah ketukan, lebih lirih
        if b % 2 == 1:
            place(track, 0.35 * saron(freq(note, octave + 1)), at + BEAT / 2)
        # kenong tiap 4 ketukan, kempul (kenong rendah) di tengah
        if b % 4 == 3:
            place(track, kenong(freq(note, -1)), at)
        if b % 8 == 5:
            place(track, 0.8 * kenong(freq(5, -2)), at)
        # gong di akhir setiap gongan (16 ketukan)
        if b % 16 == 15:
            place(track, gong(), at)
    # lesung: tok... tok... tok... di sela-sela, tidak selalu rapi (penumbuk yang letih)
    for g in range(BEATS // 4):
        base = g * 4 * BEAT
        for k, off in enumerate((0.6, 1.85, 3.1)):
            if g % 4 == 3 and k == 2:
                continue                      # sesekali berhenti: "kalau sepi, ia tahu ada yang lari"
            place(track, lesung() * (0.9 - 0.15 * k), base + off * BEAT + rng.normal(0, 0.03))
    # angin malam: desis pelan berfilter
    wind = rng.standard_normal(total)
    k = 300
    wind = np.convolve(wind, np.ones(k) / k, mode='same') * (0.5 + 0.5 * np.sin(np.arange(total) / SR * 0.21))
    track += 0.9 * wind
    track = reverb(track)
    # samakan kenyaringan dengan musik lain (RMS ±-17 dBFS), puncak dijinakkan limiter lunak
    track *= 0.14 / np.sqrt(np.mean(track ** 2))
    track = 0.92 * np.tanh(track / 0.92)
    pcm = (track * 32767).astype(np.int16)
    tmp = OUT[:-4] + '_tmp.wav'
    with wave.open(tmp, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    ffmpeg = os.environ.get('FFMPEG', 'ffmpeg')
    r = subprocess.run([ffmpeg, '-y', '-loglevel', 'error', '-i', tmp, '-c:a', 'libvorbis', '-q:a', '3', OUT])
    os.remove(tmp)
    if r.returncode != 0:
        sys.exit('ffmpeg gagal')
    print('musik desa:', OUT, f'{LENGTH:.0f} detik')


if __name__ == '__main__':
    main()
