#pragma once

#include <deque>

// 体の連動の調整値。
struct BodyLinkSettings
{
	bool isEnabled = false;
	// 脚の振り（前に出た膝と後ろの膝の前後の差 ÷ 太もも2本分の長さ。走りで 0.5〜0.6 くらい）が 1 のときの角度。
	float pelvisYawDeg = 18.0f;     // 骨盤のひねり。前に出た脚の側の腰が前へ出る。
	float shoulderYawDeg = 22.0f;   // 肩のひねり。骨盤と逆向き（前に出た腕の側の肩が前へ出る）。
	// 頭がどれだけ肩のひねりについていくか。0 なら頭は前を向いたまま、1 なら肩と一緒に回る。
	float headFollow = 0.2f;
	// 浮いている脚の側の骨盤が下がる角度（左右の足の高さの差が脚の長さの 15% 以上のとき）。
	float pelvisRollDeg = 5.0f;
	// 肩の左右の傾きを、骨盤の傾きの何倍の逆向きにするか（0 なら背骨で戻して水平、0.5 なら半分だけ逆へ傾く）。
	float chestRollRatio = 0.5f;
	// アニメーションの骨盤・肩の線の傾き・ひねりを、まっすぐに戻してから足すか。
	// 走り・ダッシュ・止まるは、もとのクリップの1コマの胴の姿勢（骨盤が約 10° 傾き、約 14° ひねれている）を全コマに残しているため。
	bool levelTorso = true;
	// 上腕を前まで出す・後ろまで引く角度（真下を 0°）。この角度に近づくほど進まなくなり、超えない。0 なら抑えない。
	// 最大の角度の半分までは、アニメーションのまま。
	float armFrontMaxDeg = 0.0f;
	float armBackMaxDeg = 0.0f;
	// 走っているときに、上腕の前後の振り（真下を 0°）を何倍にするか。1 ならアニメーションのまま。前・後ろの最大の角度で抑える前にかける。
	float armSwingScale = 1.0f;
	// 走っているときに、肩の関節（上腕のつけ根）・ひじの関節を、アニメーションから前後へ回しておく角度。正なら前、負なら後ろ。
	// 歩く速さから走る速さに近づくにつれて効かせる（立ち止まり・歩きは、アニメーションの角度のまま）。
	float shoulderJointOffsetDeg = 0.0f;
	float elbowJointOffsetDeg = 0.0f;
	// 走っているときに、上腕を体の外へ開く角度。負なら内へ閉じる。
	float upperArmSpreadDeg = 0.0f;
	// ジャンプ中（跳ぶ・空中・着地）に、太ももを股関節から体の外へ開く角度。負なら内へ閉じる。
	// ジャンプのクリップはもとのマネキンが脚を開き気味に立つ姿勢のままなので、股の開きをここで直す。
	float jumpLegSpreadDeg = 0.0f;
	// 空中（跳ぶ・空中のクリップ）で、太ももを上げる角度（真下からの角度）・膝の曲がりを何倍にするか。1 ならアニメーションのまま。
	// 着地では変えない（足が地面から浮く・地面に入るため）。
	float jumpHipFlexScale = 1.0f;
	float jumpKneeBendScale = 1.0f;
	// 空中で、太ももを股関節から体の左右の軸まわりに回し、膝を前へ出す角度。負なら後ろへ。着地では変えない。
	float jumpKneeForwardDeg = 0.0f;
	// 走っているときに、左右の腕・脚の形をそろえ、頭を前に向ける。
	// 走りのクリップは左右がそろっておらず、後ろから見ると、左腕が右腕より約 11° 外へ開いて約 9° 後ろへ振れ、右足だけが内へ入り、頭が約 5° 横を向いている。
	bool symmetrize = true;
};

// 走る・歩くときに、脚の動きに合わせて骨盤・背骨・肩・首を連動させる部品。
// アニメーションは横から見た角度だけで作ってあり、背中が固まって見えるため、再生中の脚の位置から毎フレーム求めて足す。
//   前に出た脚の側の骨盤が前へ回り、肩は逆向きにひねり返す。浮いている脚の側の骨盤が下がる。頭は前を向いたまま。
// 骨盤を回した分だけ脚を戻すので、足の位置はほとんど変わらない。
// 持ち主が、Skeleton のワールド行列を計算したあとの処理（SetOnPostUpdateWorldMatrix）から Update() を呼ぶ。
class BodyLink
{
public:
	// model はスケルトンを読み込んだあとのもの。骨が見つからなければ何もしない。
	void Init(ModelRender& model, const BodyLinkSettings& settings);
	bool IsActive() const
	{
		return m_isActive;
	}
	// キャラクターの見た目の向き。モデルを Update する前に毎フレーム渡す。
	void SetRotation(const Quaternion& rotation)
	{
		m_rotation = rotation;
	}
	// 走っている度合い（歩く速さで 0、走る速さで 1）。モデルを Update する前に毎フレーム渡す。
	void SetRunWeight(float runWeight)
	{
		m_runWeight = runWeight;
	}
	// ジャンプ中（跳ぶ・空中・着地）の度合いと、空中（着地を除く）の度合い（どちらも 0〜1）。
	// 持ち主が、入る・出るときに少しずつ変えて渡す。
	void SetJumpWeight(float jumpWeight, float airWeight)
	{
		m_jumpWeight = jumpWeight;
		m_airWeight = airWeight;
	}
	void Update(Skeleton& skeleton);
	// 調整値。開発用の画面（ImGui）で変えるときに使う。変えた値は次のフレームから効く。
	BodyLinkSettings& GetSettings()
	{
		return m_settings;
	}
	// 調べる用: 脚を変える前と後の、股関節から見た膝の位置（キャラクターの横・上・前）。顔の向きと前の向きの角度。
	struct DebugInfo
	{
		Vector3 kneeBefore[2];
		Vector3 kneeAfterForward[2];
		Vector3 kneeAfter[2];
		float faceYawDeg = 0.0f;
	};
	const DebugInfo& GetDebugInfo() const
	{
		return m_debug;
	}
	// 調整値を、Init で受け取った値（player.json の値）に戻す。
	void ResetSettings()
	{
		m_settings = m_initialSettings;
	}

private:
	enum EnBone
	{
		enBone_Hips,
		enBone_Spine,
		enBone_Chest,
		enBone_UpperChest,
		enBone_Neck,
		enBone_Head,
		enBone_UpperLegL,
		enBone_LowerLegL,
		enBone_FootL,
		enBone_UpperLegR,
		enBone_LowerLegR,
		enBone_FootR,
		enBone_UpperArmL,
		enBone_UpperArmR,
		enBone_LowerArmL,
		enBone_LowerArmR,
		enBone_HandL,
		enBone_HandR,
		enBone_Num,
	};

	Vector3 GetPosition(Skeleton& skeleton, EnBone bone) const;
	// bone とその子孫を、bone の位置を中心に axis まわりへ angle（ラジアン）回す。
	void RotateAround(Skeleton& skeleton, EnBone bone, const Vector3& axis, float angle) const;
	// axis まわりに正の角度で回したとき、bone の位置が direction へ動くなら 1、逆なら -1。
	float SignToward(Skeleton& skeleton, EnBone bone, EnBone pivot, const Vector3& axis, const Vector3& direction) const;
	// left → right の線を、キャラクターの左右の向き（水平で、前の向きに直角）にそろえる回転を求める。回す必要があれば true。
	bool MakeLevelRotation(Skeleton& skeleton, EnBone left, EnBone right, const Vector3& forward, Vector3& axis, float& angle) const;
	// 上腕の前後の振りを、armFrontMaxDeg・armBackMaxDeg までに抑える。
	void LimitArmSwing(Skeleton& skeleton, EnBone upperArm, EnBone lowerArm, const Vector3& forward) const;
	// joint（関節）から先を、キャラクターの左右の軸まわりに回し、tip（その先の骨）を前へ angleDeg 動かす。負なら後ろ。
	// 上腕の前後の振り（真下を 0°）を scale 倍にする。
	void ScaleArmSwing(Skeleton& skeleton, EnBone upperArm, EnBone lowerArm, const Vector3& forward, float scale) const;
	void RotateJointForward(Skeleton& skeleton, EnBone joint, EnBone tip, const Vector3& forward, float angleDeg) const;
	// joint（関節）から先を、キャラクターの前後の軸まわりに回し、tip（その先の骨）を体の外へ angleDeg 動かす。負なら内へ。
	void RotateJointOutward(Skeleton& skeleton, EnBone joint, EnBone tip, const Vector3& forward, float angleDeg) const;
	// 太もも（upperLeg から先）を股関節を中心に回し、膝を体の外へ angleDeg 動かす。負なら内へ。
	// 軸は太ももの向きと横の向きの両方に直角にするので、太ももが前へ上がっていても、ねじれずに開く・閉じる。
	void SpreadLeg(Skeleton& skeleton, EnBone upperLeg, EnBone lowerLeg, const Vector3& forward, float angleDeg) const;
	// joint から tip への向きと、base の向きとの角度を scale 倍にする（joint から先を回す）。
	// 太ももを上げる量（base は真下）・膝の曲がり（base は太ももの向き）に使う。
	void ScaleJointAngle(Skeleton& skeleton, EnBone joint, EnBone tip, const Vector3& base, float scale) const;
	// joint（関節）から先を回し、joint から tip（その先の骨）への向きを direction にそろえる。
	void RotateToward(Skeleton& skeleton, EnBone joint, EnBone tip, const Vector3& direction) const;
	// 頭の左右の向きを、キャラクターの前の向きにそろえる。
	void LevelHeadYaw(Skeleton& skeleton, const Vector3& forward) const;
	// 左右の足の、股関節からの横の位置を、両足の平均にそろえる。
	void SymmetrizeLegs(Skeleton& skeleton, const Vector3& forward) const;
	// 腕の形を、1歩前（脚の前後が今と逆だったとき）の反対の腕を左右に映した形と、半分ずつ混ぜる。
	// 走りは1歩ごとに左右が入れ替わるので、こうすると左右の腕が同じ形で振れる。
	void SymmetrizeArms(Skeleton& skeleton, const Vector3& forward, float swing);

	// 腕の向き（キャラクターの左右・上・前の座標）の記録。
	struct ArmSample
	{
		float time = 0.0f;
		Vector3 directions[4];  // 左の上腕・左の前腕・右の上腕・右の前腕。
	};

	BodyLinkSettings m_settings;
	BodyLinkSettings m_initialSettings;  // Init で受け取った値。ResetSettings() で戻す。
	int m_boneNos[enBone_Num] = {};
	bool m_isActive = false;
	Quaternion m_rotation = Quaternion::Identity;
	float m_runWeight = 0.0f;
	float m_jumpWeight = 0.0f;
	float m_airWeight = 0.0f;
	DebugInfo m_debug;
	// 腕をそろえるための記録。
	std::deque<ArmSample> m_armHistory;
	float m_time = 0.0f;
	float m_stepDuration = 0.0f;     // 1歩（脚の前後が入れ替わるまで）の時間。0 ならまだわからない。
	float m_lastStepTime = -1.0f;    // 最後に脚の前後が入れ替わった時刻。
	float m_lastSwingSign = 0.0f;    // 右の膝が前なら 1、左なら -1。
	int m_longStepCount = 0;         // 1歩の時間より長すぎる入れ替わりが続いた回数。
	float m_armSymmetryWeight = 0.0f;  // 腕をそろえる強さ（0〜1）。急に変わらないように少しずつ動かす。
};
