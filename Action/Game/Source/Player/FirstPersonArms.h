#pragma once

#include "CharacterModel.h"
#include "CharacterInput.h"
#include "PoseLayers.h"

class Player;
class SkillController;
class FirstPersonBob;

// 一人称カメラのときに画面に映す、プレイヤーの腕と手。
// プレイヤーの体（袖の肘から先を消したもの）の腕だけを映してカメラの前に置き、魔法に合わせて腕のアニメーションを流す。
// - 構えでは、両手を握りこぶしにして画面の下の左右に置き、ファイティングポーズのように、手のひらを内側へ向け、親指の側をカメラへ向ける。
// - メインの魔法（氷）を撃つと、右手と左手で交互に、手を開いて前へ突き出し、構えに戻す。
// - サブの魔法（炎）を溜めている間は、開いた両手を画面の下の火の玉の左右に添え、撃つと両腕を前へ伸ばして、最初の構えに戻す。
// 腕と手の形は、どれもクリップ（Animation/FirstPerson/Baked/）に焼き込んである（作り方は FirstPersonArmsPoser）。
// 氷を撃つ腕は、左右の腕の骨だけに重みを付けたアニメーションの層で流し、構えの上に重ねる。
// コードで足すのは、腕以外の骨を見えなくすることと、歩き・走りの腕の揺れだけ。
// GameCamera が持ち、一人称のときだけ Update・Draw を呼ぶ。
class FirstPersonArms
{
public:
	void Init();
	// 一人称に切り替えたときに呼ぶ。構えの姿勢から始める。
	void Reset(const Player& player);
	// 毎フレーム、カメラを動かしたあとに呼ぶ。bob の腕の揺れを、撃つ形になっていない腕にだけ足す。
	void Update(const Player& player, const FirstPersonBob& bob);
	void Draw(RenderContext& rc);

private:
	enum EnClip
	{
		enClip_Idle,         // 構え（両手を握りこぶしにして、画面の下の左右に置く）。
		enClip_CastL,        // 左の骨の腕（画面の右の手）で氷を撃つ形。層で流す。
		enClip_CastR,        // 右の骨の腕（画面の左の手）で氷を撃つ形。層で流す。
		enClip_ChargeStart,  // 両手を前へ上げる（炎を溜め始める）。
		enClip_Charge,       // 両手を火の玉の左右に添えて溜める。
		enClip_Release,      // 両腕を前へ伸ばして炎を放つ。
		enClip_ReleaseToIdle,  // 炎を放った形から、腕を下ろしながら握り直して構えに戻る（繰り返さない）。
		enClip_Num,
	};

	// 腕の骨と、見えなくする骨（腕以外）の番号を探す。
	void InitBones();
	// 腕以外の骨を、その骨の位置で大きさ 0 にする（骨のワールド行列を計算したあとに呼ぶ）。
	void HideBones(Skeleton& skeleton);
	// 歩き・走りの腕の振りと揺れ（m_bobSwing・m_bobOffset）を、肩を中心に足す。
	void SwingArms(Skeleton& skeleton);
	void UpdateClip(const Player& player, float deltaTime);
	void Play(EnClip clip, float interpolateTime);
	void UpdateTransform();
	// 炎の溜めのクリップ（溜め始め・溜め）を流しているか。
	bool IsChargeClip() const;
	// そのボタンのスキルを、前のフレームから撃ったか（撃った回数が増えたか）。
	static bool IsFiredThisFrame(const SkillController& skills, ActionButton button, int& lastCount);

	static constexpr ActionButton MAIN_BUTTON = ActionButton::Action1;  // 氷（左クリック）の魔法のボタン。Player が登録する。
	static constexpr ActionButton SUB_BUTTON = ActionButton::Action2;   // 炎（右クリック、溜め撃ち）の魔法のボタン。

	CharacterModel m_model;
	PoseLayers m_poseLayers;
	std::vector<int> m_hiddenBones;    // 大きさ 0 にする骨（腕以外）の番号。
	std::vector<int> m_armBones[2];    // 左腕・右腕の骨（上腕から先）の番号。
	int m_upperArmBone[2] = { -1, -1 };  // 左右の上腕の骨の番号（腕を振るときの中心）。
	EnClip m_clip = enClip_Idle;
	float m_castWeight[2] = { 0.0f, 0.0f };  // 左右の腕の、氷を撃つ形の層の重み（0 で構え、1 で撃つ形）。
	float m_holdWeight[2] = { 0.0f, 0.0f };  // 左右の腕を、撃つ形（氷・炎）にしている度合い。揺れを弱めるのに使う。
	float m_releaseTimer = 0.0f;             // 炎を撃ったあと、両手を開いて見せている残りの秒数。
	float m_castTimer[2] = { 0.0f, 0.0f };   // 左右の手の、氷を撃ったあと、手を開いて突き出している残りの秒数。
	// 左右の腕の、歩き・走りの揺れと着地の沈み（x: カメラの右、y: 上、z: 前。cm）。撃つ形の腕は弱めてある。
	Vector3 m_bobOffset[2] = { Vector3::Zero, Vector3::Zero };
	// 左右の腕を、歩き・走りで前後に振る量（+ で前、- で後ろ）。撃つ形の腕は弱めてある。
	float m_bobSwing[2] = { 0.0f, 0.0f };
	int m_lastMainFireCount = 0;  // 前のフレームまでに、氷を撃った回数。
	int m_lastSubFireCount = 0;   // 前のフレームまでに、炎を撃った回数。
};
