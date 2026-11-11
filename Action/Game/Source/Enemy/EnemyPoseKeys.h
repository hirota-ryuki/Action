#pragma once

#include <map>
#include <string>
#include <vector>

// 黒騎士のモーションに足す、関節と剣の角度のキー（開発用の画面で打つ）。
// クリップごとに「何コマ目に、どの関節を何度回すか」を持ち、打ったコマの間はなめらかにつなぐ。
// 保存先は Assets/data/black_knight_pose_keys.json。tools/BlenderScripts/retarget_black_knight.py が同じ JSON を読み、
// 同じ決まりで FBX に焼き込む（ここを変えたら、スクリプトの apply_pose_keys も同じように変える）。
//
// 角度の決まり（ゲームとスクリプトで同じ）
//   - 回す軸は、キャラクターの体の向きから見た 左右の軸・前後の軸・縦の軸（腰ではなく、キャラクター全体の向き）。
//   - 3つの値は [左右の軸まわり, 前後の軸まわり, 縦の軸まわり]（度）。この順に回す。
//     左右の軸 + : 前を向いている向きが下へ倒れる向き（お辞儀の向き）
//     前後の軸 + : 上を向いている向きが体の右へ倒れる向き
//     縦の軸   + : 前を向いている向きが体の左へ回る向き
//   - 骨の付け根を中心に、その骨から先をまとめて回す。肩 → 上腕 → 前腕、太もも → すねの順に回す。
//   - 剣は、剣の骨の付け根を中心に回したあと、体の [左, 前, 上] へ動かす（cm。モデルの大きさを掛ける前の値）。
class EnemyPoseKeys
{
public:
	// 角度を足す関節。この並び（親から子の順）で回す。
	enum EnBone
	{
		enBone_ShoulderL,
		enBone_UpperArmL,
		enBone_LowerArmL,
		enBone_ShoulderR,
		enBone_UpperArmR,
		enBone_LowerArmR,
		enBone_UpperLegL,
		enBone_LowerLegL,
		enBone_UpperLegR,
		enBone_LowerLegR,
		enBone_Num,
	};
	static const char* GetBoneName(int bone);         // 骨の名前（J_Bip_L_Shoulder など）
	static const char* GetBoneLabel(int bone);        // 画面に出す名前（左肩 など）
	static constexpr const char* SWORD_BONE_NAME = "J_Sec_Sword";

	// あるコマでの、足す角度と剣の動き。
	struct Pose
	{
		Vector3 boneDeg[enBone_Num];
		Vector3 swordDeg = Vector3::Zero;
		Vector3 swordPos = Vector3::Zero;

		Pose()
		{
			for (auto& deg : boneDeg) {
				deg = Vector3::Zero;
			}
		}
		bool IsZero() const;
	};
	struct Key
	{
		int frame = 1;  // 何コマ目か（1 から数える。1コマ目がアニメーションの始まり）
		Pose pose;
	};

	// JSON を読む。ないときは、キーがない状態にする。
	void Load(const char* path);
	// JSON に書く。書けたら true。
	bool Save(const char* path) const;

	// frame コマ目（小数でよい）の値。キーがないクリップは全部 0。
	Pose Evaluate(const std::string& clip, float frame) const;
	// frame コマ目にキーがあればそれを返す（なければ nullptr）。
	Key* FindKey(const std::string& clip, int frame);
	// frame コマ目にキーを打つ（あれば上書き）。
	void SetKey(const std::string& clip, int frame, const Pose& pose);
	void RemoveKey(const std::string& clip, int frame);
	const std::vector<Key>& GetKeys(const std::string& clip) const;

private:
	std::map<std::string, std::vector<Key>> m_clips;  // コマの順に並べておく
};
