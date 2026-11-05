"""UE 5.8 コマンドライン用: 任意のアニメーションを SK_Player (VRM) にリターゲットして FBX で書き出す。

使い方:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="retarget_to_player.py <config.json>"

config.json:
{
  "work_dir": "/Game/04_Retarget/Mixamo",        # 作る資産の置き場
  "source_mesh_fbx": "D:/.../YBot.fbx",           # これか source_mesh のどちらか
  "source_mesh": "/Game/.../SK_Mannequin",        # 既存のスケルタルメッシュを使う場合
  "align_source_to_target": false,                # 元が A ポーズなら true（T ポーズの VRM に合わせる）
  "clips": [
    {"name": "Idle", "fbx": "D:/.../Idle.fbx"},   # FBX から読み込む
    {"name": "Walk", "asset": "/Game/.../Walk"}   # 既存の AnimSequence を使う
  ],
  "export_dir": "D:/.../out",
  "log": "D:/.../result.txt"
}
"""
import json
import os
import sys

import unreal

TARGET_MESH = "/Game/01_PlayerVRM/SK_Player"
TARGET_RIG = "/Game/01_PlayerVRM/IK_Player_Mannequin"

lines = []


def log(*args):
    msg = " ".join(str(a) for a in args)
    lines.append(msg)
    unreal.log("[retarget] " + msg)


def asset_tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


def import_fbx(fbx_path, dest_dir, dest_name, skeleton=None):
    """skeleton が None ならスケルタルメッシュとして、あればアニメーションだけを読み込む。"""
    ui = unreal.FbxImportUI()
    ui.automated_import_should_detect_type = False
    ui.import_materials = False
    ui.import_textures = False
    if skeleton is None:
        ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
        ui.import_mesh = True
        ui.import_as_skeletal = True
        ui.import_animations = False
        ui.create_physics_asset = False
    else:
        ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
        ui.import_mesh = False
        ui.import_animations = True
        ui.skeleton = skeleton
        ui.anim_sequence_import_data.set_editor_property(
            "animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        ui.anim_sequence_import_data.set_editor_property("remove_redundant_keys", False)

    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = dest_dir
    task.destination_name = dest_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.options = ui
    asset_tools().import_asset_tasks([task])

    paths = list(task.imported_object_paths)
    log("imported", fbx_path, "->", paths)
    for p in paths:
        obj = unreal.load_asset(p)
        if skeleton is None and isinstance(obj, unreal.SkeletalMesh):
            return obj
        if skeleton is not None and isinstance(obj, unreal.AnimSequence):
            return obj
    raise RuntimeError("import failed: " + fbx_path)


def recreate(path, asset_class, factory):
    """あれば読み込んで使い回す（消して作り直すと、読み込み済みの資産が残って作れないため）。"""
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path), False
    d, n = path.rsplit("/", 1)
    return asset_tools().create_asset(n, d, asset_class, factory), True


def make_source_rig(work_dir, source_mesh):
    rig, is_new = recreate(work_dir + "/IK_Source", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    if not is_new:
        log("reuse source rig")
        return rig
    c.set_skeletal_mesh(source_mesh)
    ok = c.apply_auto_generated_retarget_definition()
    c.apply_auto_fbik()
    unreal.EditorAssetLibrary.save_loaded_asset(rig)
    log("source rig auto:", ok, "root=", c.get_retarget_root())
    for ch in c.get_retarget_chains():
        log("  src chain", ch.get_editor_property("chain_name"))
    return rig


def make_retargeter(work_dir, source_rig, source_mesh, align):
    rtg, is_new = recreate(work_dir + "/RTG_SourceToPlayer", unreal.IKRetargeter, unreal.IKRetargetFactory())
    c = unreal.IKRetargeterController.get_controller(rtg)
    if not is_new:
        log("reuse retargeter")
        return rtg
    S = unreal.RetargetSourceOrTarget
    target_rig = unreal.load_asset(TARGET_RIG)
    target_mesh = unreal.load_asset(TARGET_MESH)
    c.set_ik_rig(S.SOURCE, source_rig)
    c.set_ik_rig(S.TARGET, target_rig)
    c.set_preview_mesh(S.SOURCE, source_mesh)
    c.set_preview_mesh(S.TARGET, target_mesh)
    c.add_default_ops()
    c.assign_ik_rig_to_all_ops(S.SOURCE, source_rig)
    c.assign_ik_rig_to_all_ops(S.TARGET, target_rig)
    c.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    for i in range(c.get_num_retarget_ops()):
        log("  op", i, c.get_op_name(i))
    if align:
        # 元のポーズ（A ポーズなど）を、ターゲット（VRM の T ポーズ）に合わせる。
        c.create_retarget_pose("AlignedToPlayer", S.SOURCE)
        c.auto_align_all_bones(S.SOURCE)
        log("aligned source retarget pose")
    unreal.EditorAssetLibrary.save_loaded_asset(rtg)
    return rtg


def retarget(work_dir, rtg, source_mesh, anims):
    inputs = unreal.IKRetargetBatchOperationInputs()
    inputs.assets_to_retarget = [unreal.EditorAssetLibrary.find_asset_data(a.get_path_name()) for a in anims]
    inputs.source_mesh = source_mesh
    inputs.target_mesh = unreal.load_asset(TARGET_MESH)
    inputs.ik_retarget_asset = rtg
    inputs.prefix = "Player_"
    inputs.target_path = work_dir + "/Out"
    inputs.overwrite_existing_files = True
    inputs.include_referenced_assets = False
    result = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
    out = []
    for ad in result:
        obj = unreal.load_asset(str(ad.package_name))
        if isinstance(obj, unreal.AnimSequence):
            unreal.EditorAssetLibrary.save_loaded_asset(obj)
            out.append(obj)
    log("retargeted:", [o.get_path_name() for o in out])
    return out


def export_fbx(anim, path):
    opt = unreal.FbxExportOption()
    opt.ascii = False
    opt.export_morph_targets = False
    opt.export_preview_mesh = False
    opt.map_skeletal_motion_to_root = False
    opt.export_local_time = True
    task = unreal.AssetExportTask()
    task.object = anim
    task.filename = path
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    task.exporter = unreal.AnimSequenceExporterFBX()
    task.options = opt
    ok = unreal.Exporter.run_asset_export_task(task)
    log("export", anim.get_name(), "->", path, ok)
    return ok


def main():
    cfg = json.load(open(sys.argv[-1], encoding="utf-8"))
    work = cfg["work_dir"]
    try:
        if cfg.get("source_mesh_fbx"):
            source_mesh = import_fbx(cfg["source_mesh_fbx"], work, "SK_Source")
        else:
            source_mesh = unreal.load_asset(cfg["source_mesh"])
        skeleton = source_mesh.skeleton
        log("source mesh", source_mesh.get_path_name(), "skeleton", skeleton.get_path_name())

        anims = []
        names = []
        for clip in cfg["clips"]:
            if clip.get("fbx"):
                a = import_fbx(clip["fbx"], work + "/Src", "A_" + clip["name"], skeleton)
            else:
                a = unreal.load_asset(clip["asset"])
            log("clip", clip["name"], a.get_path_name(), "len=", a.get_play_length())
            anims.append(a)
            names.append(clip["name"])

        rig = make_source_rig(work, source_mesh)
        rtg = make_retargeter(work, rig, source_mesh, cfg.get("align_source_to_target", False))
        outs = retarget(work, rtg, source_mesh, anims)

        os.makedirs(cfg["export_dir"], exist_ok=True)
        for name, src in zip(names, anims):
            match = [o for o in outs if o.get_name().endswith(src.get_name())]
            if not match:
                log("NO OUTPUT for", name)
                continue
            export_fbx(match[0], os.path.join(cfg["export_dir"], name + ".fbx"))
        log("DONE")
    except Exception as e:
        import traceback
        log("ERROR", e, traceback.format_exc())
    finally:
        open(cfg["log"], "w", encoding="utf-8").write("\n".join(lines))


main()
