#include "stdafx.h"
#include "BodyLink.h"
#include "SpringBones.h"

namespace
{
	// VRoid のモデルの骨の名前（EnBone の順）。
	const wchar_t* const BONE_NAMES[] = {
		L"J_Bip_C_Hips",
		L"J_Bip_C_Spine",
		L"J_Bip_C_Chest",
		L"J_Bip_C_UpperChest",
		L"J_Bip_C_Neck",
		L"J_Bip_C_Head",
		L"J_Bip_L_UpperLeg",
		L"J_Bip_L_LowerLeg",
		L"J_Bip_L_Foot",
		L"J_Bip_R_UpperLeg",
		L"J_Bip_R_LowerLeg",
		L"J_Bip_R_Foot",
		L"J_Bip_L_UpperArm",
		L"J_Bip_R_UpperArm",
		L"J_Bip_L_LowerArm",
		L"J_Bip_R_LowerArm",
		L"J_Bip_L_Hand",
		L"J_Bip_R_Hand",
	};
	// 背骨のひねり・傾きを、Spine・Chest・UpperChest に分ける割合。
	constexpr float SPINE_SHARE = 0.4f;
	constexpr float CHEST_SHARE = 0.3f;
	constexpr float UPPER_CHEST_SHARE = 0.3f;
	// 首のひねり・傾きを、Neck・Head に分ける割合。
	constexpr float NECK_SHARE = 0.5f;
	constexpr float HEAD_SHARE = 0.5f;
	// 左右の足の高さの差が、脚の長さのこの割合になったら、骨盤の傾きをいっぱいにする。
	// 走りでは足の高さの差がすぐこの割合を超えるので、小さいと傾きが左右に切り替わるだけになる（0.15 では切り替わるだけだった）。
	constexpr float FULL_ROLL_FOOT_HEIGHT_RATIO = 0.4f;
	// 腕の振りを抑えるとき、最大の角度のこの割合までは、そのままにする。
	constexpr float ARM_LIMIT_KNEE_RATIO = 0.5f;
	// 向きを調べるために試しに回す角度（ラジアン）。
	constexpr float PROBE_ANGLE = 0.1f;
	// 脚の振りがこの大きさを超えて逆になったら、1歩進んだとみなす（0 の近くの揺れで数えないため）。
	constexpr float STEP_SWING_THRESHOLD = 0.1f;
	// 1歩の時間として受け付ける範囲（秒）。
	constexpr float MIN_STEP_SECONDS = 0.1f;
	constexpr float MAX_STEP_SECONDS = 0.8f;
	// 1歩の時間として受け付けるのは、今の1歩の時間のこの倍まで。
	constexpr float STEP_ACCEPT_RATIO = 1.5f;
	// 最後に脚の前後が入れ替わってから、1歩の時間のこの倍たっても入れ替わらなければ、走っていないとみなす。
	constexpr float STEP_TIMEOUT_RATIO = 3.0f;
	// 腕の向きを残しておく時間（秒）。1歩より長くする。
	constexpr float ARM_HISTORY_SECONDS = 1.0f;
	// 腕をそろえる強さが 0 から 1 まで変わるのにかかる時間（秒）。
	constexpr float ARM_SYMMETRY_FADE_SECONDS = 0.25f;

	// キャラクターの左右・上・前の座標にする（x が side、y が上、z が forward）。
	Vector3 ToLocal(const Vector3& v, const Vector3& side, const Vector3& forward)
	{
		return Vector3(v.Dot(side), v.y, v.Dot(forward));
	}

	Vector3 ToWorld(const Vector3& v, const Vector3& side, const Vector3& forward)
	{
		return side * v.x + Vector3::AxisY * v.y + forward * v.z;
	}

	// 左右を入れ替える（左の腕の向きを、右の腕の向きにする）。
	Vector3 Mirror(const Vector3& local)
	{
		return Vector3(-local.x, local.y, local.z);
	}

	// a から b へ t だけ近づけた向き。
	Vector3 BlendDirection(const Vector3& a, const Vector3& b, float t)
	{
		Vector3 blended = a * (1.0f - t) + b * t;
		if (blended.LengthSq() <= FLT_EPSILON) {
			return a;
		}
		blended.Normalize();
		return blended;
	}

	Vector3 Normalized(Vector3 v)
	{
		if (v.LengthSq() > FLT_EPSILON) {
			v.Normalize();
		}
		return v;
	}

	Vector3 GetTranslation(const Matrix& m)
	{
		return Vector3(m.m[3][0], m.m[3][1], m.m[3][2]);
	}

	// center を中心に axis まわりへ angle 回す行列。
	Matrix MakeRotationAround(const Vector3& center, const Vector3& axis, float angle)
	{
		Matrix toOrigin, rotation, back;
		toOrigin.MakeTranslation(center * -1.0f);
		rotation.MakeRotationAxis(axis, angle);
		back.MakeTranslation(center);
		return toOrigin * rotation * back;
	}

	float Clamp(float value, float minValue, float maxValue)
	{
		return max(minValue, min(maxValue, value));
	}
}

void BodyLink::Init(ModelRender& model, const BodyLinkSettings& settings)
{
	static_assert(sizeof(BONE_NAMES) / sizeof(BONE_NAMES[0]) == enBone_Num, "BONE_NAMES と EnBone の数をそろえる");
	m_settings = settings;
	m_initialSettings = settings;
	m_isActive = false;
	if (!m_settings.isEnabled || !model.GetSkeleton().IsInited()) {
		return;
	}
	for (int i = 0; i < enBone_Num; i++) {
		m_boneNos[i] = model.FindBoneID(BONE_NAMES[i]);
		if (m_boneNos[i] == -1) {
			return;
		}
	}
	m_isActive = true;
}

Vector3 BodyLink::GetPosition(Skeleton& skeleton, EnBone bone) const
{
	return GetTranslation(skeleton.GetBone(m_boneNos[bone])->GetWorldMatrix());
}

void BodyLink::RotateAround(Skeleton& skeleton, EnBone bone, const Vector3& axis, float angle) const
{
	if (fabsf(angle) <= 1.0e-5f) {
		return;
	}
	SpringBones::RotateBoneTree(*skeleton.GetBone(m_boneNos[bone]), MakeRotationAround(GetPosition(skeleton, bone), axis, angle));
}

bool BodyLink::MakeLevelRotation(Skeleton& skeleton, EnBone left, EnBone right, const Vector3& forward, Vector3& axis, float& angle) const
{
	angle = 0.0f;
	Vector3 line = GetPosition(skeleton, right) - GetPosition(skeleton, left);
	if (line.LengthSq() <= FLT_EPSILON) {
		return false;
	}
	line.Normalize();
	// まっすぐな線: 前の向きと上の向きの両方に直角な向き。左 → 右の向きに合わせる。
	Vector3 target;
	target.Cross(Vector3::AxisY, forward);
	target.Normalize();
	if (target.Dot(line) < 0.0f) {
		target *= -1.0f;
	}
	axis.Cross(line, target);
	const float sinAngle = axis.Length();
	if (sinAngle <= 1.0e-5f) {
		return false;
	}
	axis *= 1.0f / sinAngle;
	angle = atan2f(sinAngle, line.Dot(target));
	return true;
}

float BodyLink::SignToward(Skeleton& skeleton, EnBone bone, EnBone pivot, const Vector3& axis, const Vector3& direction) const
{
	const Vector3 position = GetPosition(skeleton, bone);
	Vector3 moved = position;
	MakeRotationAround(GetPosition(skeleton, pivot), axis, PROBE_ANGLE).Apply(moved);
	return (moved - position).Dot(direction) >= 0.0f ? 1.0f : -1.0f;
}

void BodyLink::Update(Skeleton& skeleton)
{
	if (!m_isActive) {
		return;
	}
	const Vector3 up = Vector3::AxisY;
	Vector3 forward = Vector3::AxisZ;
	m_rotation.Apply(forward);
	forward.y = 0.0f;
	if (forward.LengthSq() <= FLT_EPSILON) {
		return;
	}
	forward.Normalize();

	// アニメーションの脚の形から、脚の振りと、どちらの足が浮いているかを読む。
	const Vector3 hipL = GetPosition(skeleton, enBone_UpperLegL);
	const Vector3 kneeL = GetPosition(skeleton, enBone_LowerLegL);
	const Vector3 footL = GetPosition(skeleton, enBone_FootL);
	const Vector3 hipR = GetPosition(skeleton, enBone_UpperLegR);
	const Vector3 kneeR = GetPosition(skeleton, enBone_LowerLegR);
	const Vector3 footR = GetPosition(skeleton, enBone_FootR);
	const float thighLength = ((kneeL - hipL).Length() + (kneeR - hipR).Length()) * 0.5f;
	const float shinLength = ((footL - kneeL).Length() + (footR - kneeR).Length()) * 0.5f;
	if (thighLength <= 1.0e-3f) {
		return;
	}
	// 右の膝が前に出ているほど正。
	const float swing = Clamp(((kneeR - hipR).Dot(forward) - (kneeL - hipL).Dot(forward)) / (2.0f * thighLength), -1.0f, 1.0f);
	// 右の足が高い（右脚が浮いている）ほど正。
	const float footHeight = Clamp((footR - footL).Dot(up) / ((thighLength + shinLength) * FULL_ROLL_FOOT_HEIGHT_RATIO), -1.0f, 1.0f);

	// ── アニメーションの胴の傾き・ひねりを、まっすぐに戻す ──
	// 走り・ダッシュ・止まるは、もとのクリップの1コマの胴の姿勢を全コマに残しているため、骨盤が傾き・ひねれたままになっている。
	// 骨盤の線（左右の太ももの付け根）と肩の線（左右の上腕の付け根）を、キャラクターの左右の向きにそろえる。
	if (m_settings.levelTorso) {
		// 骨盤の線をまっすぐにし、脚は戻して足の位置を保つ。
		Vector3 pelvisAxis;
		float pelvisAngle = 0.0f;
		if (MakeLevelRotation(skeleton, enBone_UpperLegL, enBone_UpperLegR, forward, pelvisAxis, pelvisAngle)) {
			RotateAround(skeleton, enBone_Hips, pelvisAxis, pelvisAngle);
			RotateAround(skeleton, enBone_UpperLegL, pelvisAxis, -pelvisAngle);
			RotateAround(skeleton, enBone_UpperLegR, pelvisAxis, -pelvisAngle);
		}
		// 肩の線をまっすぐにする（背骨に分けて回す）。
		Vector3 chestAxis;
		float chestAngle = 0.0f;
		if (MakeLevelRotation(skeleton, enBone_UpperArmL, enBone_UpperArmR, forward, chestAxis, chestAngle)) {
			RotateAround(skeleton, enBone_Spine, chestAxis, chestAngle * SPINE_SHARE);
			RotateAround(skeleton, enBone_Chest, chestAxis, chestAngle * CHEST_SHARE);
			RotateAround(skeleton, enBone_UpperChest, chestAxis, chestAngle * UPPER_CHEST_SHARE);
		}
		// 頭は、胴を戻した分を首で打ち消して、アニメーションの向きのままにする（逆の順に戻す）。
		RotateAround(skeleton, enBone_Neck, chestAxis, -chestAngle);
		RotateAround(skeleton, enBone_Neck, pelvisAxis, -pelvisAngle);
	}
	// 走っているときは、頭を前に向け、足の横の位置を左右でそろえる。
	if (m_settings.symmetrize && m_runWeight > 0.0f) {
		LevelHeadYaw(skeleton, forward);
		SymmetrizeLegs(skeleton, forward);
	}

	// ── ひねり（縦の軸まわり）──
	// 骨盤: 右の膝が前なら、右の腰が前へ出る向き。骨盤を回した分だけ脚を戻して、足の位置を保つ。
	const float yawSign = SignToward(skeleton, enBone_UpperLegR, enBone_Hips, up, forward);
	const float pelvisYaw = Math::DegToRad(m_settings.pelvisYawDeg) * swing * yawSign;
	RotateAround(skeleton, enBone_Hips, up, pelvisYaw);
	RotateAround(skeleton, enBone_UpperLegL, up, -pelvisYaw);
	RotateAround(skeleton, enBone_UpperLegR, up, -pelvisYaw);
	// 肩: 骨盤と逆向き。骨盤からの差を背骨に分けて回す。
	const float shoulderYaw = -Math::DegToRad(m_settings.shoulderYawDeg) * swing * yawSign;
	const float spineYaw = shoulderYaw - pelvisYaw;
	RotateAround(skeleton, enBone_Spine, up, spineYaw * SPINE_SHARE);
	RotateAround(skeleton, enBone_Chest, up, spineYaw * CHEST_SHARE);
	RotateAround(skeleton, enBone_UpperChest, up, spineYaw * UPPER_CHEST_SHARE);
	// 頭: 肩のひねりを首で戻し、前を向いたままにする。
	const float neckYaw = shoulderYaw * m_settings.headFollow - shoulderYaw;
	RotateAround(skeleton, enBone_Neck, up, neckYaw * NECK_SHARE);
	RotateAround(skeleton, enBone_Head, up, neckYaw * HEAD_SHARE);

	// ── 左右の傾き（前後の軸まわり）──
	// 骨盤: 浮いている脚の側の腰が下がる。脚は戻して、足の位置を保つ。
	const float rollSign = SignToward(skeleton, enBone_UpperLegR, enBone_Hips, forward, up);
	const float pelvisRoll = -Math::DegToRad(m_settings.pelvisRollDeg) * footHeight * rollSign;
	RotateAround(skeleton, enBone_Hips, forward, pelvisRoll);
	RotateAround(skeleton, enBone_UpperLegL, forward, -pelvisRoll);
	RotateAround(skeleton, enBone_UpperLegR, forward, -pelvisRoll);
	// 肩: 骨盤と逆へ少し傾ける。
	const float chestRoll = -pelvisRoll * m_settings.chestRollRatio;
	const float spineRoll = chestRoll - pelvisRoll;
	RotateAround(skeleton, enBone_Spine, forward, spineRoll * SPINE_SHARE);
	RotateAround(skeleton, enBone_Chest, forward, spineRoll * CHEST_SHARE);
	RotateAround(skeleton, enBone_UpperChest, forward, spineRoll * UPPER_CHEST_SHARE);
	// 頭: 水平に戻す。
	RotateAround(skeleton, enBone_Neck, forward, -chestRoll * NECK_SHARE);
	RotateAround(skeleton, enBone_Head, forward, -chestRoll * HEAD_SHARE);

	// ── 走っているときだけ、腕を振る量を変える ──
	const float swingScale = 1.0f + (m_settings.armSwingScale - 1.0f) * m_runWeight;
	ScaleArmSwing(skeleton, enBone_UpperArmL, enBone_LowerArmL, forward, swingScale);
	ScaleArmSwing(skeleton, enBone_UpperArmR, enBone_LowerArmR, forward, swingScale);

	// ── 腕の振りの大きさをそろえる ──
	// 走りのクリップは、上腕が後ろへ最大約 100°（真下を 0°）まで振れる。原神は 40° くらいまでなので、前・後ろの最大の角度で抑える。
	LimitArmSwing(skeleton, enBone_UpperArmL, enBone_LowerArmL, forward);
	LimitArmSwing(skeleton, enBone_UpperArmR, enBone_LowerArmR, forward);

	// ── 走っているときだけ、肩の関節・ひじの関節を前後へ回しておく ──
	const float shoulderOffset = m_settings.shoulderJointOffsetDeg * m_runWeight;
	const float elbowOffset = m_settings.elbowJointOffsetDeg * m_runWeight;
	RotateJointForward(skeleton, enBone_UpperArmL, enBone_LowerArmL, forward, shoulderOffset);
	RotateJointForward(skeleton, enBone_UpperArmR, enBone_LowerArmR, forward, shoulderOffset);
	RotateJointForward(skeleton, enBone_LowerArmL, enBone_HandL, forward, elbowOffset);
	RotateJointForward(skeleton, enBone_LowerArmR, enBone_HandR, forward, elbowOffset);
	// 上腕を体の外へ開く。
	const float spreadOffset = m_settings.upperArmSpreadDeg * m_runWeight;
	RotateJointOutward(skeleton, enBone_UpperArmL, enBone_LowerArmL, forward, spreadOffset);
	RotateJointOutward(skeleton, enBone_UpperArmR, enBone_LowerArmR, forward, spreadOffset);

	// ── 左右の腕の形をそろえる ──
	SymmetrizeArms(skeleton, forward, swing);
}

void BodyLink::RotateToward(Skeleton& skeleton, EnBone joint, EnBone tip, const Vector3& direction) const
{
	const Vector3 current = Normalized(GetPosition(skeleton, tip) - GetPosition(skeleton, joint));
	const Vector3 target = Normalized(direction);
	Vector3 axis;
	axis.Cross(current, target);
	const float sinAngle = axis.Length();
	if (sinAngle <= 1.0e-5f) {
		return;
	}
	axis *= 1.0f / sinAngle;
	RotateAround(skeleton, joint, axis, atan2f(sinAngle, current.Dot(target)));
}

void BodyLink::LevelHeadYaw(Skeleton& skeleton, const Vector3& forward) const
{
	// 頭の骨の Z 軸が顔の前の向き。
	const Matrix& head = skeleton.GetBone(m_boneNos[enBone_Head])->GetWorldMatrix();
	Vector3 face(head.m[2][0], 0.0f, head.m[2][2]);
	if (face.LengthSq() <= FLT_EPSILON) {
		return;
	}
	face.Normalize();
	Vector3 axis;
	axis.Cross(face, forward);
	const float sinAngle = axis.Length();
	if (sinAngle <= 1.0e-5f) {
		return;
	}
	axis *= 1.0f / sinAngle;
	RotateAround(skeleton, enBone_Head, axis, atan2f(sinAngle, face.Dot(forward)) * m_runWeight);
}

void BodyLink::SymmetrizeLegs(Skeleton& skeleton, const Vector3& forward) const
{
	Vector3 side;
	side.Cross(Vector3::AxisY, forward);
	side.Normalize();
	const EnBone upperLegs[2] = { enBone_UpperLegL, enBone_UpperLegR };
	const EnBone feet[2] = { enBone_FootL, enBone_FootR };
	const Vector3 hips = GetPosition(skeleton, enBone_Hips);
	// 股関節から見た足の横の位置（体の外を正）。
	float outwardSigns[2];
	float offsets[2];
	for (int i = 0; i < 2; i++) {
		const Vector3 hip = GetPosition(skeleton, upperLegs[i]);
		outwardSigns[i] = (hip - hips).Dot(side) >= 0.0f ? 1.0f : -1.0f;
		offsets[i] = (GetPosition(skeleton, feet[i]) - hip).Dot(side) * outwardSigns[i];
	}
	const float average = (offsets[0] + offsets[1]) * 0.5f;
	for (int i = 0; i < 2; i++) {
		const Vector3 hip = GetPosition(skeleton, upperLegs[i]);
		const Vector3 leg = GetPosition(skeleton, feet[i]) - hip;
		const float shift = (average - offsets[i]) * m_runWeight;
		RotateToward(skeleton, upperLegs[i], feet[i], leg + side * (shift * outwardSigns[i]));
	}
}

void BodyLink::SymmetrizeArms(Skeleton& skeleton, const Vector3& forward, float swing)
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
	m_time += deltaTime;

	// 脚の前後が入れ替わった時刻から、1歩の時間を求める。
	float swingSign = m_lastSwingSign;
	if (swing > STEP_SWING_THRESHOLD) {
		swingSign = 1.0f;
	}
	else if (swing < -STEP_SWING_THRESHOLD) {
		swingSign = -1.0f;
	}
	if (swingSign != m_lastSwingSign && m_lastSwingSign != 0.0f) {
		if (m_lastStepTime >= 0.0f) {
			const float interval = m_time - m_lastStepTime;
			// 入れ替わりを1回見のがすと2歩分になるので、今の1歩の時間より長すぎるものは使わない。
			// 2回続けて長ければ、歩幅が変わったとみなして使う。
			const bool isTooLong = m_stepDuration > 0.0f && interval > m_stepDuration * STEP_ACCEPT_RATIO;
			m_longStepCount = isTooLong ? m_longStepCount + 1 : 0;
			if (interval < MIN_STEP_SECONDS || interval > MAX_STEP_SECONDS) {
				m_longStepCount = 0;
			}
			else if (!isTooLong) {
				m_stepDuration = m_stepDuration > 0.0f ? (m_stepDuration + interval) * 0.5f : interval;
			}
			else if (m_longStepCount >= 2) {
				m_stepDuration = interval;
				m_longStepCount = 0;
			}
		}
		m_lastStepTime = m_time;
	}
	m_lastSwingSign = swingSign;

	// 今の腕の向きを残す（そろえる前の向き）。
	Vector3 side;
	side.Cross(Vector3::AxisY, forward);
	side.Normalize();
	const EnBone joints[4][2] = {
		{ enBone_UpperArmL, enBone_LowerArmL },
		{ enBone_LowerArmL, enBone_HandL },
		{ enBone_UpperArmR, enBone_LowerArmR },
		{ enBone_LowerArmR, enBone_HandR },
	};
	ArmSample sample;
	sample.time = m_time;
	for (int i = 0; i < 4; i++) {
		sample.directions[i] = Normalized(ToLocal(GetPosition(skeleton, joints[i][1]) - GetPosition(skeleton, joints[i][0]), side, forward));
	}
	m_armHistory.push_back(sample);
	while (!m_armHistory.empty() && m_armHistory.front().time < m_time - ARM_HISTORY_SECONDS) {
		m_armHistory.pop_front();
	}

	// 走っていて、1歩の時間がわかっているときだけそろえる。止まったら少しずつやめる。
	const bool isStepping = m_stepDuration > 0.0f && m_time - m_lastStepTime < m_stepDuration * STEP_TIMEOUT_RATIO;
	const float targetWeight = (m_settings.symmetrize && isStepping) ? m_runWeight : 0.0f;
	const float fadeStep = deltaTime / ARM_SYMMETRY_FADE_SECONDS;
	m_armSymmetryWeight = Clamp(targetWeight, m_armSymmetryWeight - fadeStep, m_armSymmetryWeight + fadeStep);
	if (m_armSymmetryWeight <= 0.0f) {
		return;
	}

	// 1歩前（脚の前後が今と逆だったとき）の記録を探す。
	const float pastTime = m_time - m_stepDuration;
	if (m_armHistory.front().time > pastTime) {
		return;
	}
	const ArmSample* past = &m_armHistory.front();
	for (const ArmSample& s : m_armHistory) {
		if (s.time > pastTime) {
			break;
		}
		past = &s;
	}

	// 左の腕は、1歩前の右の腕を左右に映した形と混ぜる（右の腕も同じ）。上腕を回してから前腕を回す。
	const float blend = 0.5f * m_armSymmetryWeight;
	for (int arm = 0; arm < 2; arm++) {
		const int self = arm * 2;
		const int other = (1 - arm) * 2;
		for (int part = 0; part < 2; part++) {
			const Vector3 target = BlendDirection(sample.directions[self + part], Mirror(past->directions[other + part]), blend);
			RotateToward(skeleton, joints[self + part][0], joints[self + part][1], ToWorld(target, side, forward));
		}
	}
}

void BodyLink::RotateJointOutward(Skeleton& skeleton, EnBone joint, EnBone tip, const Vector3& forward, float angleDeg) const
{
	if (fabsf(angleDeg) <= 1.0e-3f) {
		return;
	}
	// 外の向き: 胸の真ん中から、この腕の肩の方への水平な向き。
	Vector3 outward = GetPosition(skeleton, joint) - GetPosition(skeleton, enBone_UpperChest);
	Vector3 side;
	side.Cross(Vector3::AxisY, forward);
	side.Normalize();
	outward = side * (outward.Dot(side) >= 0.0f ? 1.0f : -1.0f);
	// 前後の軸まわりに回す。どちら向きに回すと tip が外へ出るかは、試しに回して決める。
	const float sign = SignToward(skeleton, tip, joint, forward, outward);
	RotateAround(skeleton, joint, forward, Math::DegToRad(angleDeg) * sign);
}

void BodyLink::ScaleArmSwing(Skeleton& skeleton, EnBone upperArm, EnBone lowerArm, const Vector3& forward, float scale) const
{
	if (fabsf(scale - 1.0f) <= 1.0e-3f) {
		return;
	}
	const Vector3 arm = GetPosition(skeleton, lowerArm) - GetPosition(skeleton, upperArm);
	// 横から見た上腕の角度。真下を 0°、後ろを正、前を負にする。
	const float backAngleDeg = Math::RadToDeg(atan2f(-arm.Dot(forward), -arm.y));
	// 後ろへ backAngleDeg × (scale - 1) だけ足す = 前へ その逆だけ回す。
	RotateJointForward(skeleton, upperArm, lowerArm, forward, -backAngleDeg * (scale - 1.0f));
}

void BodyLink::RotateJointForward(Skeleton& skeleton, EnBone joint, EnBone tip, const Vector3& forward, float angleDeg) const
{
	if (fabsf(angleDeg) <= 1.0e-3f) {
		return;
	}
	// キャラクターの左右の軸。どちら向きに回すと tip が前へ出るかは、試しに回して決める。
	Vector3 side;
	side.Cross(Vector3::AxisY, forward);
	side.Normalize();
	const float sign = SignToward(skeleton, tip, joint, side, forward);
	RotateAround(skeleton, joint, side, Math::DegToRad(angleDeg) * sign);
}

void BodyLink::LimitArmSwing(Skeleton& skeleton, EnBone upperArm, EnBone lowerArm, const Vector3& forward) const
{
	const Vector3 down = Vector3::AxisY * -1.0f;
	const Vector3 arm = GetPosition(skeleton, lowerArm) - GetPosition(skeleton, upperArm);
	// 横から見た上腕の角度。真下を 0°、後ろを正、前を負にする。
	const float angle = atan2f(-arm.Dot(forward), arm.Dot(down));
	const float maxAngle = Math::DegToRad(angle >= 0.0f ? m_settings.armBackMaxDeg : m_settings.armFrontMaxDeg);
	if (maxAngle <= 0.0f) {
		return;
	}
	// 最大の角度の半分までは、そのまま。そこから先は、最大の角度に近づくほど進まなくなる（なめらかに頭打ちにする）。
	const float magnitude = fabsf(angle);
	const float knee = maxAngle * ARM_LIMIT_KNEE_RATIO;
	if (magnitude <= knee) {
		return;
	}
	const float limited = knee + (maxAngle - knee) * tanhf((magnitude - knee) / (maxAngle - knee));
	// 上腕を真下へ近づける向き（上腕と真下の両方に直角な軸）に、縮めた分だけ回す。前腕・手も一緒に動く。
	Vector3 axis;
	axis.Cross(arm, down);
	if (axis.LengthSq() <= FLT_EPSILON) {
		return;
	}
	axis.Normalize();
	RotateAround(skeleton, upperArm, axis, magnitude - limited);
}
