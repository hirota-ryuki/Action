# 一人称で映す腕のモデル（SK_PlayerFp.tkm / .tks）を、プレイヤーのモデル（SK_Player.tkm / .tks）から作る。
#
# 使い方:
#   python make_first_person_arms_tkm.py <SK_Player.tkm> <出力の .tkm> [--report]
#
# - 袖を肘までにする: 服（袖）のマテリアルのうち、前腕から先（LowerArm・Hand・指）の骨に重みが寄っている三角形を消す。
#   一人称では、広がった袖口が手より大きく、目の位置から見ると手が隠れるため。
# - 頂点・骨・マテリアルは変えない（三角形の並び＝インデックスだけを減らす）。骨の番号がそのままなので、
#   SK_Player.tks をコピーして同じ名前の .tks にすれば、SK_Player 用に変換したモーションがそのまま使える。
# - Blender を通さないのは、Blender から書き出した腕だけのモデルに、ゲームでモーションを付けると映らなかったため
#   （原因は未解明。経緯を参照）。
# - --report で、マテリアルごとの三角形の数と、消す数を出す（どのマテリアルが袖かを確かめる用）。
import shutil
import struct
import sys

SLEEVE_TEXTURES = None  # main で決める


def read_tkm(path):
    b = open(path, "rb").read()
    o = 0
    version, flat, nparts = struct.unpack_from("<BBH", b, o)
    o += 4
    parts = []
    for _ in range(nparts):
        nmat, nvert, isz = struct.unpack_from("<IIB", b, o)
        o += 12
        mats = []
        for _m in range(nmat):
            start = o
            names = []
            for _t in range(5):
                n, = struct.unpack_from("<I", b, o)
                o += 4
                if n:
                    names.append(b[o:o + n].decode("utf-8", "replace"))
                    o += n + 1
                else:
                    names.append("")
            mats.append({"raw": b[start:o], "textures": names})
        vstart = o
        verts = [struct.unpack_from("<12f4h", b, o + i * 56) for i in range(nvert)]
        o += 56 * nvert
        vraw = b[vstart:o]
        ibs = []
        for _m in range(nmat):
            npoly, = struct.unpack_from("<I", b, o)
            o += 4
            fmt = "<%d%s" % (npoly * 3, "I" if isz == 4 else "H")
            idx = list(struct.unpack_from(fmt, b, o))
            o += npoly * 3 * isz
            ibs.append(idx)
        parts.append({"isz": isz, "mats": mats, "vraw": vraw, "verts": verts, "ibs": ibs})
    assert o == len(b), (o, len(b))
    return {"version": version, "flat": flat, "parts": parts}


def write_tkm(path, tkm):
    out = bytearray(struct.pack("<BBH", tkm["version"], tkm["flat"], len(tkm["parts"])))
    for p in tkm["parts"]:
        out += struct.pack("<IIB3x", len(p["mats"]), len(p["verts"]), p["isz"])
        for m in p["mats"]:
            out += m["raw"]
        out += p["vraw"]
        for idx in p["ibs"]:
            out += struct.pack("<I", len(idx) // 3)
            out += struct.pack("<%d%s" % (len(idx), "I" if p["isz"] == 4 else "H"), *idx)
    open(path, "wb").write(out)


def read_tks_names(path):
    b = open(path, "rb").read()
    o = 0
    n, = struct.unpack_from("<i", b, o)
    o += 4
    names = []
    parents = []
    for _ in range(n):
        c = b[o]
        o += 1
        names.append(b[o:o + c].decode())
        o += c + 1
        p, = struct.unpack_from("<i", b, o)
        parents.append(p)
        o += 4 + 96
    return names, parents


def main():
    src_tkm, out_tkm = sys.argv[1], sys.argv[2]
    report = "--report" in sys.argv
    src_tks = src_tkm[:-4] + ".tks"
    names, parents = read_tks_names(src_tks)

    def under(i, root):
        while i >= 0:
            if names[i] == root:
                return True
            i = parents[i]
        return False

    # 前腕から先の骨
    forearm = {i for i in range(len(names)) if under(i, "J_Bip_L_LowerArm") or under(i, "J_Bip_R_LowerArm")}

    tkm = read_tkm(src_tkm)
    removed_total = 0
    for p in tkm["parts"]:
        # tkm の頂点の重みは3つ書き、4つ目は残り（1 - 合計）として使われる（ComputeAnimationVertexBuffer.fx と同じ）
        def forearm_weight(v):
            w = v[8:11]
            idx = v[12:16]
            total = sum(w[k] for k in range(3) if idx[k] in forearm)
            if idx[3] in forearm:
                total += max(0.0, 1.0 - sum(w))
            return total

        fw = [forearm_weight(v) for v in p["verts"]]
        for m, (mat, idx) in enumerate(zip(p["mats"], p["ibs"])):
            tex = mat["textures"][0]
            is_sleeve = tex in SLEEVE_TEXTURES
            keep = []
            cut = 0
            for t in range(0, len(idx), 3):
                tri = idx[t:t + 3]
                # 書き出しで +1 されている（TkmFile::Save）ので、頂点の番号は -1 したもの
                if is_sleeve and all(fw[i - 1] >= 0.5 for i in tri):
                    cut += 1
                    continue
                keep += tri
            if report:
                touches = sum(1 for t in range(0, len(idx), 3) if all(fw[i - 1] >= 0.5 for i in idx[t:t + 3]))
                print(f"material {m:2d} albedo={tex:24s} 三角形 {len(idx)//3:6d}  前腕から先 {touches:5d}  消す {cut:5d}")
            p["ibs"][m] = keep
            removed_total += cut
    write_tkm(out_tkm, tkm)
    shutil.copyfile(src_tks, out_tkm[:-4] + ".tks")
    print(f"WROTE {out_tkm}（消した三角形 {removed_total}）")


# 袖（服）のマテリアルのアルベドの画像。SK_Player の MaterialTextureTable.txt で Tops の CLOTH に当たるもの。
SLEEVE_TEXTURES = set(sys.argv[sys.argv.index("--sleeve") + 1].split(",")) if "--sleeve" in sys.argv else set()

if __name__ == "__main__":
    main()
