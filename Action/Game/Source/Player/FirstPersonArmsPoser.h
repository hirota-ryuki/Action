#pragma once

// 一人称の腕の形（拳の握り方・腕の置き方・手の向き・指の形）を、骨を回して作る。開発用（Debug ビルドだけ）。
// ゲームの中では使わない。焼き込み（Debug ビルドのゲームを -bakeFirstPersonArms を付けて起動する）のときだけ、
// もとのクリップ（Animation/FirstPerson/）にこの形を重ねて JSON に書き、Blender で FBX（Animation/FirstPerson/Baked/）にする。
// ふだんの FirstPersonArms は、焼き込んだクリップを流すだけ。形はここのコードを正とし、直すときはここを直して焼き直す
// （Blender で FBX を直しても、焼き直すと消える）。
class FirstPersonArmsPoser
{
public:
	// 左右の腕（0 が左の骨 J_Bip_L、画面の右に映る）を、どの形にするかの混ぜ具合（0〜1）。
	struct Weights
	{
		float cast[2] = { 0.0f, 0.0f };     // 氷を撃つときの置き方（手を開いて前へ突き出す）。
		float charge[2] = { 0.0f, 0.0f };   // 炎を溜めるときの置き方（火の玉の左右に手を添える）。
		float release[2] = { 0.0f, 0.0f };  // 炎を放つときの置き方（両腕を前へ伸ばす）。
		float grip[2] = { 1.0f, 1.0f };     // 握り具合（1 で握りこぶし、0 で開く）。
	};

	// 骨の番号を探す。skeleton は一人称の腕のモデル（SK_PlayerFp）のもの。
	void Init(const Skeleton& skeleton);
	void SetWeights(const Weights& weights);
	// 骨のワールド行列を計算したあとに呼ぶ。指を握る → 腕を置く → 手の向きを決める、の順に形を作る。
	void Apply(Skeleton& skeleton);

	// 焼き込む6本（FpIdle・FpCastL・FpCastR・FpChargeStart・FpCharge・FpRelease）を JSON に書く。
	// 書いたものと、うまくいかなかったものを logPath に書く。全部書けたら true。
	static bool BakeAll(const char* logPath);
	// Blender で作った FBX を読み、JSON の姿勢と比べる（骨の位置のずれのいちばん大きいもの）。結果は logPath に書く。
	static bool VerifyAll(const char* logPath);

private:
	// 両腕を、肩を中心に外へ開いて下げる（手を画面の左下・右下に置く）。
	void SpreadArms(Skeleton& skeleton);
	// 手首から先の向きを決める。SpreadArms のあとに呼ぶ。
	// - 構えの拳（m_grip）は、手のひらを内側へ向け、親指の側をカメラへ向ける。
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
	// 親指を、握った人差し指と中指の中節の外側に、指を横切るように重ねる（親指の第一関節と先を目標へ合わせる IK）。CurlFingers から呼ぶ。
	void PlaceThumb(Skeleton& skeleton, int side, float grip);
	static Vector3 BonePosition(Skeleton& skeleton, int boneNo);
	// chain[from] の関節を中心に、chain[from] から指先（chain[2]）までの骨をまとめて回す。
	static void RotateChain(Skeleton& skeleton, const int* chain, int from, Vector3 axis, float deg);

	static constexpr int FINGER_COUNT = 5;  // 人差し指・中指・薬指・小指・親指。
	static constexpr int THUMB = 4;         // m_fingerBones の親指の番号。
	static constexpr int LITTLE = 3;        // m_fingerBones の小指の番号。

	int m_fingerBones[2][FINGER_COUNT][3] = {};  // 左右・指ごとの、付け根から指先への3本の骨の番号（無ければ -1）。
	int m_handBone[2] = { -1, -1 };              // 左右の手の骨の番号。
	std::vector<int> m_armBones[2];       // 左腕・右腕の骨（上腕から先）の番号。
	std::vector<int> m_handTreeBones[2];  // 左右の、手首から先の骨（手と指）の番号。
	int m_upperArmBone[2] = { -1, -1 };   // 左右の上腕の骨の番号（腕を開くときの中心）。
	float m_castWeight[2] = { 0.0f, 0.0f };
	float m_chargeWeight[2] = { 0.0f, 0.0f };
	float m_releaseWeight[2] = { 0.0f, 0.0f };
	float m_grip[2] = { 1.0f, 1.0f };
};
