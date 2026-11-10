#pragma once

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
	// 上腕の後ろへの振り（真下を 0°）が armBackStartDeg を超えた分を、何倍に縮めるか。1 なら縮めない。前への振りは変えない。
	float armBackStartDeg = 20.0f;
	float armBackScale = 1.0f;
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
	void Update(Skeleton& skeleton);

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
		enBone_Num,
	};

	Vector3 GetPosition(Skeleton& skeleton, EnBone bone) const;
	// bone とその子孫を、bone の位置を中心に axis まわりへ angle（ラジアン）回す。
	void RotateAround(Skeleton& skeleton, EnBone bone, const Vector3& axis, float angle) const;
	// axis まわりに正の角度で回したとき、bone の位置が direction へ動くなら 1、逆なら -1。
	float SignToward(Skeleton& skeleton, EnBone bone, EnBone pivot, const Vector3& axis, const Vector3& direction) const;
	// left → right の線を、キャラクターの左右の向き（水平で、前の向きに直角）にそろえる回転を求める。回す必要があれば true。
	bool MakeLevelRotation(Skeleton& skeleton, EnBone left, EnBone right, const Vector3& forward, Vector3& axis, float& angle) const;
	// 上腕が後ろへ振れているとき、その角度を armBackScale 倍にする。
	void LimitArmBackSwing(Skeleton& skeleton, EnBone upperArm, EnBone lowerArm, const Vector3& forward) const;

	BodyLinkSettings m_settings;
	int m_boneNos[enBone_Num] = {};
	bool m_isActive = false;
	Quaternion m_rotation = Quaternion::Identity;
};
