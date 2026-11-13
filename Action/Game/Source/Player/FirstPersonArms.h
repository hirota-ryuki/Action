#pragma once

#include "CharacterModel.h"

class Player;
class MagicBase;

// 一人称カメラのときに画面に映す、プレイヤーの腕と手。
// プレイヤーの腕だけのモデル（SK_PlayerArms.fbx）をカメラの前に置き、魔法に合わせて腕のアニメーションを流す。
// - メインの魔法（氷）を撃つと、右手を前へ突き出して戻す。
// - サブの魔法（炎）を溜めている間は、両手を前に上げて溜め、撃つと両手を下ろす。
// GameCamera が持ち、一人称のときだけ Update・Draw を呼ぶ。
class FirstPersonArms
{
public:
	void Init();
	// 一人称に切り替えたときに呼ぶ。構えの姿勢から始める。
	void Reset(const Player& player);
	// 毎フレーム、カメラを動かしたあとに呼ぶ。
	void Update(const Player& player);
	void Draw(RenderContext& rc);

private:
	enum EnClip
	{
		enClip_Idle,         // 両手を胸の下で構える。
		enClip_CastRight,    // 右手を突き出して戻す（氷）。
		enClip_ChargeStart,  // 両手を前へ上げる（炎を溜め始める）。
		enClip_Charge,       // 両手を上げたまま溜める。
		enClip_Release,      // 炎を放って、両手を構えの高さまで下ろす。
		enClip_Num,
	};

	// 腕以外の骨を見えなくする設定。
	void InitHiddenBones();
	void UpdateClip(const Player& player);
	void Play(EnClip clip, float interpolateTime);
	void UpdateTransform();
	// 前のフレームから撃った回数が増えたか。見ている魔法が変わったときは数え直す（撃ったことにしない）。
	static bool IsCastThisFrame(const MagicBase* magic, const MagicBase*& lastMagic, int& lastCount);

	CharacterModel m_model;
	std::vector<int> m_hiddenBones;    // 大きさ 0 にする骨（腕以外）の番号。
	std::vector<int> m_lowerArmBones;  // 袖口を細くする前腕の骨の番号。
	EnClip m_clip = enClip_Idle;
	const MagicBase* m_lastMainMagic = nullptr;
	const MagicBase* m_lastSubMagic = nullptr;
	int m_lastMainCastCount = 0;
	int m_lastSubCastCount = 0;
};
