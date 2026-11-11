#pragma once

#include "EnemyPoseKeys.h"

class CharacterModel;

// 黒騎士のモーションを、F1 の開発用の画面で直す係（窓の名前は「黒騎士のモーション」）。
//   - 「編集する」にすると、AI を止めてその場に立たせ、選んだクリップをくり返し流す。止める・1コマずつ送るができる。
//   - 止めたコマで、関節（肩・上腕・ひじ・太もも・ひざ）と剣の角度・位置を変えると、そのコマにキーが打たれる。
//   - 「保存」で Assets/data/black_knight_pose_keys.json に書き、「FBX に焼く」で retarget_black_knight.py を流す。
//     焼いた FBX は、ゲームを起動し直すと読み込まれる。
// 編集中は、キーを焼いていない元のクリップ（Animation/Base/）に、打ったキーをその場で足して見せる。
class EnemyMotionEditor
{
public:
	struct ClipEntry
	{
		const char* name;  // black_knight_anims.json のクリップの名前（Attack1 など）
		int clipNo;        // ふだん流すクリップ（キーを焼いたもの）
		int baseClipNo;    // 編集中に流すクリップ（キーを焼く前のもの）
	};

	void Init(CharacterModel* model, const std::vector<ClipEntry>& clips);
	~EnemyMotionEditor();

	bool IsEditing() const
	{
		return m_isEditing;
	}
	// 編集をやめたフレームだけ true（ふだんの動きに戻すのに使う）。
	bool ConsumeStopped()
	{
		const bool stopped = m_isStopped;
		m_isStopped = false;
		return stopped;
	}
	// 編集中に毎フレーム呼ぶ。再生の時間を進め、そのコマの姿勢を置く。
	void UpdatePlayback(float deltaTime);
	// F1 の開発用の画面を出しているときに毎フレーム呼ぶ。
	void DrawUI();
	// アニメーションの姿勢ができたあとに呼ぶ。編集中だけ、打ったキーの角度を骨に足す。
	void ApplyToSkeleton(Skeleton& skeleton, const Quaternion& modelRotation, float modelScale) const;

private:
	int GetFrameCount() const;
	float GetCurrentFrame() const;  // 1 から数えたコマ（再生中は小数）
	const ClipEntry& GetClip() const
	{
		return m_clips[m_clipIndex];
	}
	void SelectClip(int index);
	bool DrawPoseEditor(EnemyPoseKeys::Pose& pose);  // 変えたら true
	void StartBake();
	void UpdateBake();

	CharacterModel* m_model = nullptr;
	std::vector<ClipEntry> m_clips;
	EnemyPoseKeys m_keys;
	bool m_isEditing = false;
	bool m_isStopped = false;
	bool m_isPlaying = true;
	int m_clipIndex = 0;
	float m_time = 0.0f;       // クリップの始まりからの秒
	float m_playSpeed = 0.5f;  // 再生の速さ（ゆっくり見る）
	bool m_isDirty = false;    // 保存していない変更がある
	std::string m_message;     // 窓に出す知らせ
	int m_boneNos[EnemyPoseKeys::enBone_Num] = {};
	int m_swordBoneNo = -1;
	int m_legBoneL = -1;
	int m_legBoneR = -1;
	// FBX に焼いている Blender
	void* m_bakeProcess = nullptr;
	std::string m_bakeClip;
};
