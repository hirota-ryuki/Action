"""動画の決まった範囲のコマから、MediaPipe の PoseLandmarker で関節の位置（33点、画面の座標）を読み取る。

使い方:
  python detect_pose.py <動画.mp4> <開始秒> <コマ数> <出力.json> <x0> <y0> <x1> <y1> <モデル.task> [<確かめ用の画像.jpg>]
  x0..y1 はキャラクターが入る切り出し範囲（ピクセル）。モデルは pose_landmarker_heavy.task
  （https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_heavy/float16/latest/pose_landmarker_heavy.task）。
  必要なもの: pip install mediapipe opencv-python-headless
出力: [{"t": 秒, "pts": [[x, y, visibility] × 33] または null}, ...]
"""
import json
import sys

import cv2
import mediapipe as mp
import numpy as np
from mediapipe.tasks.python import BaseOptions, vision

video, start, count, out, x0, y0, x1, y1, model = sys.argv[1:10]
sheet = sys.argv[10] if len(sys.argv) > 10 else None
start, count, x0, y0, x1, y1 = float(start), int(count), int(x0), int(y0), int(x1), int(y1)
opt = vision.PoseLandmarkerOptions(base_options=BaseOptions(model_asset_path=model),
                                   running_mode=vision.RunningMode.IMAGE, num_poses=1,
                                   min_pose_detection_confidence=0.3, min_pose_presence_confidence=0.3)
det = vision.PoseLandmarker.create_from_options(opt)
cap = cv2.VideoCapture(video)
fps = cap.get(cv2.CAP_PROP_FPS)
first = int(start * fps)
cap.set(cv2.CAP_PROP_POS_FRAMES, first)
CON = [(11, 13), (13, 15), (12, 14), (14, 16), (11, 23), (12, 24), (23, 25), (25, 27), (27, 31), (24, 26), (26, 28), (28, 32)]
res, tiles = [], []
for i in range(count):
    ok, fr = cap.read()
    if not ok:
        break
    crop = fr[y0:y1, x0:x1].copy()
    r = det.detect(mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(crop, cv2.COLOR_BGR2RGB)))
    pts = None
    if r.pose_landmarks:
        h, w = crop.shape[:2]
        pts = [(l.x * w, l.y * h, l.visibility) for l in r.pose_landmarks[0]]
        for a, b in CON:
            cv2.line(crop, (int(pts[a][0]), int(pts[a][1])), (int(pts[b][0]), int(pts[b][1])), (0, 0, 255) if a % 2 else (255, 0, 0), 3)
    res.append({"t": (first + i) / fps, "pts": pts})
    c = cv2.resize(crop, (300, 230))
    cv2.putText(c, "%.2f" % ((first + i) / fps), (5, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 2)
    tiles.append(c)
json.dump(res, open(out, "w"))
if sheet:
    while len(tiles) % 8:
        tiles.append(np.zeros_like(tiles[0]))
    cv2.imwrite(sheet, np.vstack([np.hstack(tiles[j:j + 8]) for j in range(0, len(tiles), 8)]))
print("detected", sum(1 for r in res if r["pts"]), "of", len(res))
