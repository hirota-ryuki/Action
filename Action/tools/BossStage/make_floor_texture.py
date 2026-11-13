"""ボスステージの床（黒大理石のタイルに金の目地）のテクスチャを作る。

Blender に入っている Python と numpy だけで動く（画面は出さない）。
    blender -b --factory-startup -P make_floor_texture.py -- <出力フォルダ>

出力（どれも上下左右の端がつながる）:
    BossFloor_albedo.png  色（sRGB）
    BossFloor_normal.png  法線（タンジェント空間）
    BossFloor_spec.png    R=金属度、A=滑らかさ（エンジンの g_spacular と同じ並び）
    BossFloor_preview.png 確かめ用（2×2 に並べて縮めたもの）
"""
import os
import sys

import bpy
import numpy as np

# ---- 調整値 ----
SIZE = 2048                 # テクスチャの1辺（ピクセル）
TILES = 4                   # 1辺に並ぶタイルの数（1枚 512 ピクセル）
GOLD_WIDTH = 10             # 金の目地の幅（ピクセル）
SEED = 7

BASE_COLOR = np.array([16, 16, 18]) / 255.0       # 大理石の地（sRGB）。Meshy の画像は 7 くらいだが、黒騎士が溶けないよう少し明るくする
CLOUD_STRENGTH = 0.35                              # 地の濃淡のむら
VEIN_COLOR = np.array([150, 148, 140]) / 255.0     # 白い筋（sRGB）
FINE_VEIN_COLOR = np.array([70, 70, 68]) / 255.0   # 細かいひび（sRGB）
GOLD_COLOR = np.array([196, 164, 108]) / 255.0     # シャンパンゴールド（sRGB）

SMOOTH_MARBLE = 0.75
SMOOTH_VEIN = 0.65
SMOOTH_GOLD = 0.55
GOLD_DEPTH = 1.5            # 目地のへこみ（法線の強さに効く）

rng = np.random.default_rng(SEED)
N = SIZE
TILE = SIZE // TILES


def fbm(beta, lo=1.0):
    """端がつながる（周期的な）なめらかな乱数の場。beta が大きいほど大きな模様になる。"""
    white = rng.standard_normal((N, N))
    fx = np.fft.fftfreq(N) * N
    k = np.sqrt(fx[None, :] ** 2 + fx[:, None] ** 2)
    k[0, 0] = 1.0
    amp = np.where(k < lo, 0.0, k ** (-beta / 2.0))
    f = np.real(np.fft.ifft2(np.fft.fft2(white) * amp))
    return (f - f.mean()) / f.std()


def sample(field, x, y):
    """周期的な場を、ずらした座標で読む（双線形）。"""
    x0 = np.floor(x).astype(int); y0 = np.floor(y).astype(int)
    tx = x - x0; ty = y - y0
    x0 %= N; y0 %= N; x1 = (x0 + 1) % N; y1 = (y0 + 1) % N
    a = field[y0, x0] * (1 - tx) + field[y0, x1] * tx
    b = field[y1, x0] * (1 - tx) + field[y1, x1] * tx
    return a * (1 - ty) + b * ty


def lines(f, width):
    """場 f の 0 の等高線を、どこでも同じ太さの線にする（0〜1）。"""
    gy, gx = np.gradient(f)
    g = np.sqrt(gx ** 2 + gy ** 2) + 1e-6
    return np.exp(-(f / (g * width)) ** 2)


# ---- 場を作る ----
cloud = fbm(3.2)
warp_x = fbm(3.6) * 40.0      # 筋を大きくうねらせる
warp_y = fbm(3.6) * 40.0
jitter_x = fbm(2.0, lo=40.0) * 1.2   # 筋の縁を少しだけギザギザにする（ひびらしさ）
jitter_y = fbm(2.0, lo=40.0) * 1.2
vein_field = fbm(4.2, lo=1.0)        # 大きくなめらか → 等高線が長い線になる
fine_field = fbm(3.6, lo=2.0)
visible = fbm(3.4)            # 筋が濃いところ・消えるところ

yy, xx = np.mgrid[0:N, 0:N].astype(np.float64)

# タイルごとに模様の位置と向きを変えて、タイルが1枚ずつ別の石に見えるようにする。
# 向きは斜め（45° ± 25°）にそろえ、筋の方向へ引き伸ばす（見本の斜めの筋に合わせる）。
ox = np.zeros((N, N)); oy = np.zeros((N, N)); ang = np.zeros((N, N))
for ty in range(TILES):
    for tx in range(TILES):
        sl = (slice(ty * TILE, (ty + 1) * TILE), slice(tx * TILE, (tx + 1) * TILE))
        ox[sl] = rng.uniform(0, N); oy[sl] = rng.uniform(0, N)
        ang[sl] = np.radians(rng.choice([45.0, 135.0]) + rng.uniform(-25.0, 25.0))
STRETCH = 0.25                # 筋の方向へ 4 倍に引き伸ばす
c, s = np.cos(ang), np.sin(ang)
u = (xx * c + yy * s) * STRETCH
v = -xx * s + yy * c
px = u + ox + sample(jitter_x, xx, yy)
py = v + oy + sample(jitter_y, xx, yy)
wx = px + sample(warp_x, px, py)
wy = py + sample(warp_y, px, py)
vein = lines(sample(vein_field, wx, wy), 1.8)
fine = lines(sample(fine_field, wx * 1.7 + 300, wy * 1.7 + 300), 0.8)
vis = np.clip(sample(visible, xx + ox, yy + oy) * 0.9 + 0.6, 0, 1)
vein *= vis
fine *= np.clip(vis - 0.3, 0, 1) * 0.7
cl = sample(cloud, xx + ox, yy + oy)

# ---- 金の目地（タイルの境目。画像の端では半分ずつになるので、並べると1本につながる） ----
dx = np.abs(((xx + TILE / 2) % TILE) - TILE / 2)
dy = np.abs(((yy + TILE / 2) % TILE) - TILE / 2)
d = np.minimum(dx, dy)
half = GOLD_WIDTH / 2.0
gold = np.clip(half + 0.5 - d, 0, 1)   # 端を1ピクセルぼかす

# ---- 色 ----
base = BASE_COLOR[None, None, :] * (1.0 + CLOUD_STRENGTH * cl[..., None])
col = base
col = col * (1 - fine[..., None]) + FINE_VEIN_COLOR * fine[..., None]
col = col * (1 - vein[..., None]) + VEIN_COLOR * vein[..., None]
col = col * (1 - gold[..., None]) + GOLD_COLOR * gold[..., None]
albedo = np.clip(col, 0, 1)

# ---- 高さ → 法線（端がつながるよう、差分は周期的に取る） ----
h = -gold * GOLD_DEPTH
hx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) * 0.5
hy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) * 0.5
n = np.stack([-hx, hy, np.ones_like(h)], axis=-1)   # 画像の上を +Y にする
n /= np.linalg.norm(n, axis=-1, keepdims=True)
normal = n * 0.5 + 0.5

# ---- 金属度・滑らかさ ----
smooth = SMOOTH_MARBLE + (SMOOTH_VEIN - SMOOTH_MARBLE) * vein + 0.03 * cl
smooth = smooth * (1 - gold) + SMOOTH_GOLD * gold
spec = np.stack([gold, np.zeros_like(h), np.zeros_like(h), np.clip(smooth, 0, 1)], axis=-1)


def save(name, rgb_or_rgba, colorspace, outdir, size=None):
    arr = rgb_or_rgba
    if arr.shape[-1] == 3:
        arr = np.concatenate([arr, np.ones(arr.shape[:2] + (1,))], axis=-1)
    hgt, wid = arr.shape[:2]
    img = bpy.data.images.new(name, wid, hgt, alpha=True)
    img.colorspace_settings.name = colorspace
    img.pixels.foreach_set(arr[::-1].astype(np.float32).ravel())   # Blender は下の行から
    img.filepath_raw = os.path.join(outdir, name + ".png")
    img.file_format = "PNG"
    img.save()
    print("SAVED", img.filepath_raw)


outdir = sys.argv[sys.argv.index("--") + 1]
os.makedirs(outdir, exist_ok=True)
save("BossFloor_albedo", albedo, "sRGB", outdir)
save("BossFloor_normal", normal, "Non-Color", outdir)
save("BossFloor_spec", spec, "Non-Color", outdir)

# 確かめ用: 2×2 に並べ、半分に縮める（つなぎ目が見える位置に来る）
tiled = np.tile(albedo, (2, 2, 1))[::2, ::2]
save("BossFloor_preview", tiled, "sRGB", outdir)
