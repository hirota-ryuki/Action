"""ボスステージの壁（暗い切り石を互い違いに積んだもの）のテクスチャを作る。

Blender に入っている Python と numpy だけで動く（画面は出さない）。
    blender -b --factory-startup -P make_wall_texture.py -- <出力フォルダ>

出力（どれも上下左右の端がつながる）:
    BossWall_albedo.png  色（sRGB）
    BossWall_normal.png  法線（タンジェント空間）
    BossWall_spec.png    R=金属度、A=滑らかさ（エンジンの g_spacular と同じ並び）
    preview/BossWall_preview.png 確かめ用（2×2 に並べて縮めたもの。このスクリプトの隣。git に入れない）

テクスチャ1枚を壁の 4m 四方に貼る（make_boss_stage.py の壁の箱の uv_meters）。
石1個は 1m × 0.5m で、1段ごとに半分ずらす。
"""
import os
import sys

import bpy
import numpy as np

# ---- 調整値 ----
SIZE = 2048                 # 模様を作るときの1辺（ピクセル）。4m に貼るので 1m が 512 ピクセル
OUT_SIZE = 1024             # 書き出すテクスチャの1辺。模様は SIZE で作り、保存するときに縮める（起動時の読み込みを軽くする）
COLUMNS = 4                 # 1段に並ぶ石の数
ROWS = 8                    # 段の数
MORTAR_WIDTH = 10           # 目地の幅（ピクセル）
BEVEL_WIDTH = 28            # 石の角の丸み（ピクセル）
SEED = 11

STONE_COLOR = np.array([20, 21, 27]) / 255.0       # 石の地（sRGB）。灰色がかった青の暗い石。床より少しだけ明るく
STONE_VARIATION = 0.18                             # 石ごとの明るさのばらつき
CLOUD_STRENGTH = 0.30                              # 石の中の濃淡のむら
MORTAR_COLOR = np.array([8, 8, 10]) / 255.0        # 目地（sRGB）
PIT_COLOR = np.array([10, 10, 13]) / 255.0         # 小さな欠け・くぼみ（sRGB）

SMOOTH_STONE = 0.35
SMOOTH_MORTAR = 0.1
STONE_HEIGHT = 3.0          # 石の盛り上がり（法線の強さに効く）
SURFACE_BUMP = 0.6          # 石の表面のざらつき

rng = np.random.default_rng(SEED)
N = SIZE
BW = N // COLUMNS           # 石の幅（ピクセル）
BH = N // ROWS              # 石の高さ（ピクセル）


def fbm(beta, lo=1.0):
    """端がつながる（周期的な）なめらかな乱数の場。beta が大きいほど大きな模様になる。"""
    white = rng.standard_normal((N, N))
    fx = np.fft.fftfreq(N) * N
    k = np.sqrt(fx[None, :] ** 2 + fx[:, None] ** 2)
    k[0, 0] = 1.0
    amp = np.where(k < lo, 0.0, k ** (-beta / 2.0))
    f = np.real(np.fft.ifft2(np.fft.fft2(white) * amp))
    return (f - f.mean()) / f.std()


yy, xx = np.mgrid[0:N, 0:N].astype(np.float64)

# ---- 石の並び（1段ごとに半分ずらす。ROWS が偶数なので、上下の端もつながる） ----
row = (yy // BH).astype(int)
shift = (row % 2) * (BW / 2)
sx = (xx + shift) % N
col = (sx // BW).astype(int)
lx = sx - col * BW          # 石の中の位置
ly = yy - row * BH
edge = np.minimum(np.minimum(lx, BW - lx), np.minimum(ly, BH - ly))   # いちばん近い縁までの距離

half = MORTAR_WIDTH / 2.0
mortar = np.clip(half + 0.5 - edge, 0, 1)
# 縁を少しガタガタにして、切り石らしくする
rough_edge = fbm(2.2, lo=30.0) * 3.0
bevel = np.clip((edge - half + rough_edge) / BEVEL_WIDTH, 0, 1)
bevel = bevel * (2 - bevel)   # 角を丸く

# 石ごとの明るさ
block_id = row * COLUMNS + col
block_tone = rng.uniform(-1, 1, ROWS * COLUMNS)[block_id]

cloud = fbm(3.0)
grain = fbm(1.6, lo=60.0)
pits = np.clip(fbm(2.4, lo=20.0) - 2.2, 0, 1)   # まばらな小さな欠け

# ---- 色 ----
tone = 1.0 + STONE_VARIATION * block_tone + CLOUD_STRENGTH * cloud
stone = STONE_COLOR[None, None, :] * tone[..., None]
stone = stone * (1 - pits[..., None]) + PIT_COLOR * pits[..., None]
# 角が欠けて暗くなったように、縁の近くを少し暗く
stone *= (0.75 + 0.25 * bevel)[..., None]
albedo = stone * (1 - mortar[..., None]) + MORTAR_COLOR * mortar[..., None]
albedo = np.clip(albedo, 0, 1)

# ---- 高さ → 法線（端がつながるよう、差分は周期的に取る） ----
h = bevel * STONE_HEIGHT + SURFACE_BUMP * (0.15 * grain + 0.35 * cloud) * bevel - pits * 1.5
hx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) * 0.5
hy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) * 0.5
n = np.stack([-hx, hy, np.ones_like(h)], axis=-1)   # 画像の上を +Y にする
n /= np.linalg.norm(n, axis=-1, keepdims=True)
normal = n * 0.5 + 0.5

# ---- 金属度・滑らかさ ----
smooth = SMOOTH_STONE + 0.05 * cloud - 0.15 * pits
smooth = smooth * (1 - mortar) + SMOOTH_MORTAR * mortar
zeros = np.zeros_like(h)
spec = np.stack([zeros, zeros, zeros, np.clip(smooth, 0, 1)], axis=-1)


def save(name, rgb_or_rgba, colorspace, outdir, size=None):
    """size を渡すと、その1辺に縮めて保存する。"""
    arr = rgb_or_rgba
    if arr.shape[-1] == 3:
        arr = np.concatenate([arr, np.ones(arr.shape[:2] + (1,))], axis=-1)
    hgt, wid = arr.shape[:2]
    img = bpy.data.images.new(name, wid, hgt, alpha=True)
    img.colorspace_settings.name = colorspace
    img.pixels.foreach_set(arr[::-1].astype(np.float32).ravel())   # Blender は下の行から
    if size and (wid, hgt) != (size, size):
        img.scale(size, size)
    img.filepath_raw = os.path.join(outdir, name + ".png")
    img.file_format = "PNG"
    img.save()
    print("SAVED", img.filepath_raw)


outdir = sys.argv[sys.argv.index("--") + 1]
os.makedirs(outdir, exist_ok=True)
save("BossWall_albedo", albedo, "sRGB", outdir, OUT_SIZE)
save("BossWall_normal", normal, "Non-Color", outdir, OUT_SIZE)
save("BossWall_spec", spec, "Non-Color", outdir, OUT_SIZE)

# 確かめ用: 2×2 に並べ、半分に縮める（つなぎ目が見える位置に来る）
preview_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview")   # ゲームのフォルダーには置かない
os.makedirs(preview_dir, exist_ok=True)
tiled = np.tile(albedo, (2, 2, 1))[::2, ::2]
save("BossWall_preview", tiled, "sRGB", preview_dir)
