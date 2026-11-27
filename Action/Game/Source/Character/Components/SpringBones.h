#pragma once

// 揺れ物の骨のまとまり1つ分の調整値（髪・スカートなど）。
struct SpringBoneGroupSettings
{
	std::wstring match;           // 骨の名前にこの文字が入っていれば、このまとまりにする。いくつも当てはまるときは長い方。
	float frequency = 3.0f;       // 揺れの速さ（1秒に何回揺れるか）。大きいほど硬く、アニメーションの形にすぐ戻る。
	float dampingRatio = 0.5f;    // 揺れの止まりやすさ。1 で揺り返さずに戻る。小さいほど何度も揺れる。
	float maxAngleDeg = 30.0f;    // アニメーションの形から曲がってよい角度の上限（親の骨が揺れたあとの形から、骨1本ごと）。
	// 根元から数えて、アニメーションの形（揺らす前）から曲がってよい角度の上限。0 なら抑えない。
	// maxAngleDeg は骨1本ごとなので、3段の骨が全部上限まで曲がると 3倍になり、裾が丸まって折れて見える。
	// こちらで抑えると、上限に当たっても、どの骨も同じ向きにそろい、まっすぐ倒れた形になる。
	float maxTotalAngleDeg = 0.0f;
	// 速く動いている間（SpringBonesSettings::fastMoveStartSpeed より速い間）に、骨1本ごとに後ろへ倒す角度の上限。0 なら倒さない。
	float fastLeanDeg = 0.0f;
	// ジャンプ中は、揺れ（体の動きからの遅れ）でアニメーションの形より上へ上がらず、
	// 真下から SpringBonesSettings::jumpMaxAngleFromDownDeg より上へも上がらない。スカートの中が見えるのを防ぐ。
	// 地面にいる間も、真下から SpringBonesSettings::maxAngleFromDownDeg より上へ上がらない。
	bool noLiftInJump = false;
};

// 揺れ物全体の調整値。
struct SpringBonesSettings
{
	bool isEnabled = false;
	std::wstring bonePrefix = L"J_Sec_";  // 揺れ物の骨の名前の頭（VRoid のモデルは J_Sec_）。
	std::vector<SpringBoneGroupSettings> groups;
	// 脚の当たり判定（太もも・すね）の太さ。揺れ物の先がこれより脚に近づかないようにする。0 なら当たらない。
	float legRadius = 0.0f;
	// ジャンプ中に、noLiftInJump のまとまり（スカート）を、真下からこの角度より上へ上げない。0 なら抑えない。
	float jumpMaxAngleFromDownDeg = 0.0f;
	// 地面にいる間も、noLiftInJump のまとまりを、真下からこの角度より上へ上げない。0 なら抑えない。
	// スカートの付け根の骨の一部は太もも・すねの子で、走ると太ももと一緒に水平より上まで上がるため。
	float maxAngleFromDownDeg = 0.0f;
	// アニメーションでの骨の動きのうち、揺れ（遅れ）として残す割合（0〜1）。
	// 0 で、揺れ物の骨はアニメーションでの自分の骨の動き（腰の揺さぶり・太ももに振られる裾など）に完全に付いていき、
	// キャラクターそのものの動き（走り出し・止まる・向きを変える）だけで揺れる。1 でアニメーションの動きも全部揺れになる。
	float animInertia = 1.0f;
	// 速く動いている間（ダッシュなど）の揺れ方。キャラクターの水平の速さが fastMoveStartSpeed から fastMoveFullSpeed（cm/秒）
	// になるまでに、「速く動いている度合い」を 0 から 1 にする。速くなったらすぐ上げ、遅くなったら fastMoveReleaseSeconds かけて下げる。
	// 度合いが 1 のとき、キャラクターが動いた分の fastMoveFollow（0〜1）だけ、揺れの先も一緒に運ぶ（ばねが追いつけずに丸まらない）。
	// 代わりに、まとまりごとの fastLeanDeg だけ、進む向きと逆へ倒した形へ引き戻す（後ろへなびく）。
	// 走り（960）より速いときだけ効くようにする。0 以下なら使わない。
	float fastMoveStartSpeed = 0.0f;
	float fastMoveFullSpeed = 0.0f;
	float fastMoveFollow = 0.0f;
	float fastMoveReleaseSeconds = 0.3f;
};

// 髪・スカートなどの揺れ物の骨を、体の動きから遅れて付いてくるように揺らす部品。
// アニメーションの姿勢を正として、その形へばねで引き戻す（体が動くと先が遅れ、止まると揺り返す）。
// 持ち主が、Skeleton のワールド行列を計算したあとの処理（SetOnPostUpdateWorldMatrix）から Update() を呼ぶ。
class SpringBones
{
public:
	// model はスケルトンを読み込んだあとのもの。Update() の中で、model のワールド行列を読む。
	void Init(ModelRender& model, const SpringBonesSettings& settings);
	// 揺らす骨があるか。
	bool IsActive() const
	{
		return !m_joints.empty();
	}
	// 調整値。開発用の画面（ImGui）で変えるときに使う。変えた値は次のフレームから効く。
	SpringBonesSettings& GetSettings()
	{
		return m_settings;
	}
	// ジャンプ中の度合い（0〜1）。持ち主が、ジャンプに入る・出るときに少しずつ変えて渡す。
	void SetJumpWeight(float jumpWeight)
	{
		m_jumpWeight = jumpWeight;
	}
	// アニメーションの姿勢（ワールド行列）を揺らす。体の連動（BodyLink）のあとに呼ぶ。
	void Update(Skeleton& skeleton);

private:
	struct Joint
	{
		int boneNo = -1;
		int childBoneNo = -1;     // 先の位置を決める子の骨。
		int depth = 0;            // 根元からの段数。親から先に動かすために並べる。
		const SpringBoneGroupSettings* group = nullptr;
		Vector3 tail = Vector3::Zero;          // 揺れている先の位置（ワールド）。
		Vector3 velocity = Vector3::Zero;      // 先の速さ。
		Vector3 prevAnimTail = Vector3::Zero;  // 前のフレームの、アニメーションどおりの先の位置。
		Matrix prevAnimWorld;                  // 前のフレームの、揺らす前の骨のワールド行列。
		Vector3 baseDirection = Vector3::Zero; // このフレームの、どの骨も揺らす前の、アニメーションの向き（maxTotalAngleDeg に使う）。
	};
	struct Capsule
	{
		int startBoneNo = -1;
		int endBoneNo = -1;
	};

	// 骨がアニメーションで動いた分（キャラクターそのものの動きは除く）だけ、揺れの先・速さを運ぶ。
	// 揺れ物の骨を動かす前（アニメーションと体の連動だけの姿勢）に呼ぶ。
	void CarryWithAnimation(Skeleton& skeleton);
	// キャラクターの水平の速さから、速く動いている度合い（m_fastWeight）と進む向きを決め、
	// 動いた分の fastMoveFollow × 度合いだけ、揺れの先を一緒に運ぶ。CarryWithAnimation のあとに呼ぶ。
	void FollowFastMove(float deltaTime);
	// 先を脚から押し出す。animTail がもとから近いときは、その近さまでは許す。
	Vector3 PushOutFromLegs(Skeleton& skeleton, const Vector3& tail, const Vector3& animTail) const;
	// 付け根 head から先 tail までの線分が、脚（太もも・すね）に入り込むか。
	// アニメーションの形（head から animTail）がもとから脚に近いときは、その近さまでは入り込みとみなさない。
	bool HitsLegs(Skeleton& skeleton, const Vector3& head, const Vector3& tail, const Vector3& animTail) const;

	SpringBonesSettings m_settings;
	ModelRender* m_modelRender = nullptr;
	Matrix m_prevModelWorld;   // 前のフレームの、モデルのワールド行列（キャラクターの位置・向き）。
	std::vector<Joint> m_joints;
	std::vector<Capsule> m_legCapsules;
	bool m_isFirstFrame = true;
	float m_jumpWeight = 0.0f;
	Vector3 m_prevModelPosition = Vector3::Zero;  // 前のフレームの、モデルの位置（速さを求める）。
	float m_fastWeight = 0.0f;                    // 速く動いている度合い（0〜1）。
	Vector3 m_fastDirection = Vector3::AxisZ;     // 速く動いている向き（水平・正規化済み）。遅くなっても最後の向きを残す。
};
