#pragma once

// 揺れ物の骨のまとまり1つ分の調整値（髪・スカートなど）。
struct SpringBoneGroupSettings
{
	std::wstring match;           // 骨の名前にこの文字が入っていれば、このまとまりにする。いくつも当てはまるときは長い方。
	float frequency = 3.0f;       // 揺れの速さ（1秒に何回揺れるか）。大きいほど硬く、アニメーションの形にすぐ戻る。
	float dampingRatio = 0.5f;    // 揺れの止まりやすさ。1 で揺り返さずに戻る。小さいほど何度も揺れる。
	float maxAngleDeg = 30.0f;    // アニメーションの形から曲がってよい角度の上限。
	// ジャンプ中は、揺れ（体の動きからの遅れ）でアニメーションの形より上へ上がらず、
	// 真下から SpringBonesSettings::jumpMaxAngleFromDownDeg より上へも上がらない。スカートの中が見えるのを防ぐ。
	bool noLiftInJump = false;
	// 持ち主が動くと、動く向きの逆（後ろ）へ流される強さ（1/秒。速さ × この値が、先を押す加速になる）。マントの翻りに使う。0 なら流されない。
	float windDrag = 0.0f;
	// 止まっていても吹く、ゆるい風の強さ（加速。cm/秒²）。向きと強さが時間とともに変わり、骨ごとに少しずれる。0 なら吹かない。
	float breeze = 0.0f;
	// 持ち主が急に速くなったとき（踏み込み）に、先へ足す速さ（cm/秒）。後ろへ burstBack、上へ burstLift。マントがバサッと浮く。0 なら煽らない。
	float burstBack = 0.0f;
	float burstLift = 0.0f;
	// 煽った直後（約 0.8 秒かけて元に戻る間）だけ広げる、曲がってよい角度の上限。maxAngleDeg より大きいときだけ効く。水平（90°）より上へ舞い上がらせるのに使う。
	float burstMaxAngleDeg = 0.0f;
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
};

// 髪・スカートなどの揺れ物の骨を、体の動きから遅れて付いてくるように揺らす部品。
// アニメーションの姿勢を正として、その形へばねで引き戻す（体が動くと先が遅れ、止まると揺り返す）。
// 持ち主が、Skeleton のワールド行列を計算したあとの処理（SetOnPostUpdateWorldMatrix）から Update() を呼ぶ。
class SpringBones
{
public:
	// 煽ったあと、角度の上限（burstMaxAngleDeg）を広げておく秒数。この間に、広げた上限から元の上限へ少しずつ戻る。
	static constexpr float BURST_WIDE_SECONDS = 0.8f;

	// model はスケルトンを読み込んだあとのもの。
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
	// 次のフレームに、揺れ物を backDirection（水平）の向きへ、後ろ・上へバサッと煽る。strength は burstBack・burstLift に掛ける倍率。
	void Burst(const Vector3& backDirection, float strength = 1.0f)
	{
		Vector3 horizontal(backDirection.x, 0.0f, backDirection.z);
		if (horizontal.LengthSq() > FLT_EPSILON) {
			horizontal.Normalize();
		}
		m_burstDirection = horizontal;
		m_burstStrength = strength;
		m_isBurstPending = true;
		m_burstWideLeft = BURST_WIDE_SECONDS;
	}
	// 揺れを止める（開発用。止めている間は、アニメーションの姿勢のまま）。戻すと、その姿勢から揺れ始める。
	void SetSuspended(bool isSuspended)
	{
		if (m_isSuspended && !isSuspended) {
			m_isFirstFrame = true;
		}
		m_isSuspended = isSuspended;
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
	};
	struct Capsule
	{
		int startBoneNo = -1;
		int endBoneNo = -1;
	};

	// 先を脚から押し出す。animTail がもとから近いときは、その近さまでは許す。
	Vector3 PushOutFromLegs(Skeleton& skeleton, const Vector3& tail, const Vector3& animTail) const;

	SpringBonesSettings m_settings;
	std::vector<Joint> m_joints;
	std::vector<Capsule> m_legCapsules;
	bool m_isFirstFrame = true;
	bool m_isSuspended = false;
	float m_jumpWeight = 0.0f;
	// 持ち主の動き（腰の骨の動きから出す）。windDrag に使う。
	int m_ownerBoneNo = -1;
	Vector3 m_prevOwnerPosition = Vector3::Zero;
	Vector3 m_ownerVelocity = Vector3::Zero;
	float m_time = 0.0f;  // 風の揺らぎに使う時間（秒）。
	Vector3 m_prevRawVelocity = Vector3::Zero;  // 前のフレームの、ならす前の持ち主の速さ（急に速くなったかを見る）。
	float m_burstCooldown = 0.0f;
	Vector3 m_burstDirection = Vector3::Zero;   // 煽る向き（持ち主が動く向きの逆の水平方向）。
	bool m_isBurstPending = false;              // このフレームに煽る。
	float m_burstStrength = 1.0f;               // 煽る強さ（burstBack・burstLift に掛ける倍率）。
	float m_burstWideLeft = 0.0f;               // 角度の上限を広げている残りの秒数（burstMaxAngleDeg）。
};
