# 黒騎士のモーションの見本帳（ゲームの F1 の画面「黒騎士のモーション」）に入れるモーションの一覧を作る。
#
# 使い方（bpy は使わないので、Blender でもふつうの Python でも動く）:
#   blender -b --python make_anim_catalog.py -- [--sns <フォルダ>] [--meshy <フォルダ>]
#
#   --sns    SnSAnimsetPro の InPlace の FBX のフォルダ（SourceFiles.zip の EXPORT/InPlace を展開したところ）。
#            _fast（同じ動きの速い版）と _Add（足し込み用のポーズ）は入れない。骨格は UE4 のマネキン（60fps）
#   --meshy  Meshy から書き出した FBX のフォルダ（下のフォルダも探す）。スケルトンテンプレートは Mixamo にしておく。
#            Rig_TPose.fbx（骨を入れただけの T ポーズ）があれば、基準の姿勢に使う
#
# 書くもの（どちらも Game/Assets/modelData/enemy/BlackKnight/AnimCatalog/。リポジトリには入れない）
#   catalog_config.json  retarget_black_knight.py の設定。これで全部を黒騎士へ移す:
#       blender -b --python retarget_black_knight.py -- <AnimCatalog>/catalog_config.json
#   catalog.json         ゲームが読む一覧（名前・分類・FBX のパス・ループするか）
import json
import os
import re
import sys

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]


def arg(name):
    return os.path.abspath(argv[argv.index(name) + 1]) if name in argv else None


SNS_DIR = arg("--sns")
MESHY_DIR = arg("--meshy")
HERE = os.path.dirname(os.path.abspath(__file__))
CATALOG_DIR = os.path.normpath(os.path.join(HERE, "../../Game/Assets/modelData/enemy/BlackKnight/AnimCatalog"))
GAME_DIR = "Assets/modelData/enemy/BlackKnight/AnimCatalog"   # ゲームから見たパス

# 名前に含まれる言葉で分類する（上から順に見て、最初に当たったもの）
CATEGORIES = [
    (u"攻撃", ("Attack", "Counter", "Kick")),
    (u"防ぐ", ("Block",)),
    (u"被弾", ("Hit",)),
    (u"倒れる", ("Death", "Defeat", "Knockdown")),
    (u"移動", ("Walk", "Run", "Strafe", "Turn", "Dodge", "Dodgle", "Jump", "Fall", "Land", "Climb")),
]


def category_of(name):
    for cat, words in CATEGORIES:
        if any(w.lower() in name.lower() for w in words):
            return cat
    return u"その他"


def is_loop(name):
    """ループさせるもの。待機・歩き・走り・横歩きの続く部分と、名前に Loop が付くもの。"""
    if re.search(r"(Start|Stop|Land|Turn|Dodg|Jump|Climb|Fall)", name) and "Loop" not in name:
        return False
    return bool(re.search(r"(Idle|Loop|Walk|Run|Strafe)", name))


clips = []
if SNS_DIR:
    for f in sorted(os.listdir(SNS_DIR)):
        base, ext = os.path.splitext(f)
        if ext.lower() != ".fbx" or base.lower().endswith("_fast") or base.endswith("_Add"):
            continue
        clips.append({
            "name": base, "source": os.path.join(SNS_DIR, f).replace("\\", "/"), "rig": "ue4",
            "loop": is_loop(base), "category": category_of(base),
        })

if MESHY_DIR:
    rest = os.path.join(MESHY_DIR, "Rig_TPose.fbx")
    for root, _, files in os.walk(MESHY_DIR):
        for f in sorted(files):
            base, ext = os.path.splitext(f)
            if ext.lower() != ".fbx" or f == "Rig_TPose.fbx":
                continue
            # Meshy の名前は「Meshy_AI_<モデル>_animated_mixamo_<モーション>」。モーションの名前だけ残す
            motion = base.split("_animated_mixamo_")[-1] if "_animated_mixamo_" in base else base
            clip = {
                "name": "Meshy_" + re.sub(r"[^0-9A-Za-z_]+", "_", motion), "source": os.path.join(root, f).replace("\\", "/"),
                "rig": "mixamo", "loop": is_loop(motion), "category": "Meshy",
            }
            if os.path.exists(rest):
                clip["rest"] = rest.replace("\\", "/")
            clips.append(clip)

os.makedirs(CATALOG_DIR, exist_ok=True)
config = {
    "target": "../BlackKnight.fbx",
    "out_dir": ".",
    "clips": [{k: v for k, v in c.items() if k != "category"} for c in clips],
}
with open(os.path.join(CATALOG_DIR, "catalog_config.json"), "w", encoding="utf-8") as fp:
    json.dump(config, fp, ensure_ascii=False, indent=2)

game = {"clips": [{"name": c["name"], "category": c["category"], "path": f"{GAME_DIR}/{c['name']}.fbx", "loop": c["loop"]}
                  for c in clips]}
with open(os.path.join(CATALOG_DIR, "catalog.json"), "w", encoding="utf-8") as fp:
    json.dump(game, fp, ensure_ascii=False, indent=2)

counts = {}
for c in clips:
    counts[c["category"]] = counts.get(c["category"], 0) + 1
print("CATALOG", len(clips), "本", counts)
print("WROTE", CATALOG_DIR)
