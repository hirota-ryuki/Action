#pragma once

#include "CharacterModel.h"
#include "CharacterInput.h"

class Player;
class SkillController;

// 一人称カメラのときに画面に映す、プレイヤーの腕と手。
// プレイヤーの体（袖の肘から先を消したもの）の腕だけを映してカメラの前に置き、魔法に合わせて腕のアニメーションを流す。
// - 構えでは、両手を握りこぶしにして画面の下の左右に置き、ファイティングポーズのように拳の握った指の側をカメラへ向ける。
// - メインの魔法（氷）を撃つと、右手と左手で交互に、手を開いて前へ突き出し、構えに戻す。
// - サブの魔法（炎）を溜めている間は、開いた両手を画面の下の火の玉の左右に添え、撃つと両腕を前へ伸ばして、最初の構えに戻す。
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
		enClip_Idle,         // 両手を胸の下で構える（氷を撃つときも、このクリップのまま、腕の置き方だけで突き出す）。
		enClip_ChargeStart,  // 両手を前へ上げる（炎を溜め始める）。
		enClip_Charge,       // 両手を上げたまま溜める。
		enClip_Num,
	};

	// 腕以外の骨を見えなくする設定。
	void InitHiddenBones();
	// 両腕を、肩を中心に外へ開いて下げる（手を画面の左下・右下に置く）。骨のワールド行列を計算したあとに呼ぶ。
	void SpreadArms(Skeleton& skeleton);
	// 手首から先の向きを決める。SpreadArms のあとに呼ぶ。
	// - 構えの拳（m_grip）は、握った指の側をカメラへ向ける。
	// - 氷・炎を撃つとき（m_castWeight・m_releaseWeight）は、手のひらを前へ向け、指の形を作り直す。
	//   炎を放つときは、両手の間を RELEASE_HAND_GAP にそろえる。
	void AimHands(Skeleton& skeleton);
	// 手首を中心に手を回して、手のひらを palmTarget へ、手首から中指の付け根への向きを fingersTarget へ、w の割合だけ向ける。
	// 回したあとの手のひらの向きを返す。
	Vector3 AimHand(Skeleton& skeleton, int side, const Vector3& palmTarget, const Vector3& fingersTarget, float w);
	// 手を開いて撃つときの指を、反らさず開きすぎない形にする（w で混ぜる）。palmNormal は手のひらの向き。AimHands から呼ぶ。
	void ShapeReleaseFingers(Skeleton& skeleton, int side, Vector3 palmNormal, float w);
	// 指の chain[j] の骨の向き（付け根側から指先へ）。
	static Vector3 FingerSegmentDirection(Skeleton& skeleton, const int* chain, int j);
	// 握り具合（m_grip）に合わせて指を握り込む。SpreadArms の前に呼ぶ。
	void CurlFingers(Skeleton& skeleton);
	// 親指を、握った人差し指と中指の外側に重ねる（親指の先を目標へ合わせる IK）。CurlFingers から呼ぶ。
	void PlaceThumb(Skeleton& skeleton, int side, float grip);
	static Vector3 BonePosition(Skeleton& skeleton, int boneNo);
	// chain[from] の関節を中心に、chain[from] から指先（chain[2]）までの骨をまとめて回す。
	static void RotateChain(Skeleton& skeleton, const int* chain, int from, Vector3 axis, float deg);
	void UpdateClip(const Player& player, float deltaTime);
	void Play(EnClip clip, float interpolateTime);
	void UpdateTransform();
	// その腕（side）を「氷を撃つときの置き方」にする割合（0〜1）。
	float GetCastWeightTarget(int side) const;
	// 今流しているクリップで、両腕を「炎を溜めるときの置き方」にする割合（0〜1）。
	float GetChargeWeightTarget() const;
	// 炎の溜めのクリップ（溜め始め・溜め）を流しているか。
	bool IsChargeClip() const;
	// その手（side）の握り具合の目標（1 で握りこぶし、0 で開く）。
	float GetGripTarget(int side) const;
	// そのボタンのスキルを、前のフレームから撃ったか（撃った回数が増えたか）。
	static bool IsFiredThisFrame(const SkillController& skills, ActionButton button, int& lastCount);

	static constexpr int FINGER_COUNT = 5;  // 人差し指・中指・薬指・小指・親指。
	static constexpr int THUMB = 4;         // m_fingerBones の親指の番号。
	static constexpr int LITTLE = 3;        // m_fingerBones の小指の番号。
	static constexpr ActionButton MAIN_BUTTON = ActionButton::Action1;  // 氷（左クリック）の魔法のボタン。Player が登録する。
	static constexpr ActionButton SUB_BUTTON = ActionButton::Action2;   // 炎（右クリック、溜め撃ち）の魔法のボタン。

	CharacterModel m_model;
	std::vector<int> m_hiddenBones;    // 大きさ 0 にする骨（腕以外）の番号。
	int m_fingerBones[2][FINGER_COUNT][3] = {};  // 左右・指ごとの、付け根から指先への3本の骨の番号（無ければ -1）。
	int m_handBone[2] = { -1, -1 };              // 左右の手の骨の番号（指の付け根の関節の曲がりを測るのに使う）。
	std::vector<int> m_armBones[2];    // 左腕・右腕の骨（上腕から先）の番号。
	std::vector<int> m_handTreeBones[2];  // 左右の、手首から先の骨（手と指）の番号。
	int m_upperArmBone[2] = { -1, -1 };  // 左右の上腕の骨の番号（腕を開くときの中心）。
	EnClip m_clip = enClip_Idle;
	float m_castWeight[2] = { 0.0f, 0.0f };  // 左右の腕の、構え（0）と氷を撃つとき（1）の置き方の混ぜ具合。
	float m_chargeWeight[2] = { 0.0f, 0.0f };  // 左右の腕の、炎を溜めるときの置き方の混ぜ具合（1 で溜めるときの置き方）。
	float m_releaseWeight[2] = { 0.0f, 0.0f }; // 左右の腕の、炎を放つときの置き方（前へ伸ばす）の混ぜ具合。
	float m_grip[2] = { 1.0f, 1.0f };        // 左右の手の握り具合（1 で握りこぶし、0 で開く）。
	float m_releaseTimer = 0.0f;             // 炎を撃ったあと、両手を開いて見せている残りの秒数。
	float m_castTimer[2] = { 0.0f, 0.0f };   // 左右の手の、氷を撃ったあと、手を開いて突き出している残りの秒数。
	int m_lastMainFireCount = 0;  // 前のフレームまでに、氷を撃った回数。
	int m_lastSubFireCount = 0;   // 前のフレームまでに、炎を撃った回数。
};
