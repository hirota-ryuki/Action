#include "stdafx.h"
#include "SpringBones.h"
#include "PoseLayers.h"

namespace
{
	// 1フレームの時間の上限。ロードなどで長く止まったときに、ばねが暴れないようにする。
	constexpr float MAX_DELTA_TIME = 1.0f / 20.0f;
	// 1フレームでこれより大きく動いたら、ワープ（リスポーンなど）とみなして揺れをなくす。
	constexpr float TELEPORT_DISTANCE = 200.0f;
	constexpr float TELEPORT_DISTANCE_SQ = TELEPORT_DISTANCE * TELEPORT_DISTANCE;
	// 持ち主の動きを調べる骨（腰）。windDrag に使う。
	const wchar_t* const OWNER_BONE_NAME = L"J_Bip_C_Hips";
	// 持ち主の速さをならす速さ（大きいほどすぐ追いつく。1/秒）。
	constexpr float OWNER_VELOCITY_SMOOTHING = 10.0f;
	// 持ち主の速さが、この加速（cm/秒²）より急に増えたら、踏み込みの出だしとみなして煽る。歩き出し・走り出しでは届かない。
	constexpr float BURST_ACCELERATION = 20000.0f;
	// 煽ったあと、次に煽れるまでの秒数。
	constexpr float BURST_COOLDOWN_SECONDS = 0.4f;
	// 風の揺らぎを、骨の番号ごとにずらす量（ラジアン）。
	constexpr float BREEZE_PHASE_PER_BONE = 1.7f;
	// 脚の当たり判定に使う骨（付け根 → 先）。
	const wchar_t* const LEG_BONE_PAIRS[][2] = {
		{ L"J_Bip_L_UpperLeg", L"J_Bip_L_LowerLeg" },
		{ L"J_Bip_L_LowerLeg", L"J_Bip_L_Foot" },
		{ L"J_Bip_R_UpperLeg", L"J_Bip_R_LowerLeg" },
		{ L"J_Bip_R_LowerLeg", L"J_Bip_R_Foot" },
	};

	Vector3 GetTranslation(const Matrix& m)
	{
		return Vector3(m.m[3][0], m.m[3][1], m.m[3][2]);
	}

	// 線分 a-b の上で p に一番近い点。
	Vector3 ClosestPointOnSegment(const Vector3& p, const Vector3& a, const Vector3& b)
	{
		const Vector3 ab = b - a;
		const float lengthSq = ab.LengthSq();
		if (lengthSq <= FLT_EPSILON) {
			return a;
		}
		float t = (p - a).Dot(ab) / lengthSq;
		t = max(0.0f, min(1.0f, t));
		return a + ab * t;
	}

	// from の向きを to の向きへ回すクォータニオン（どちらも正規化済み）。ほとんど同じ向きなら false。
	bool MakeRotationBetween(const Vector3& from, const Vector3& to, Quaternion& out)
	{
		Vector3 axis;
		axis.Cross(from, to);
		const float sinAngle = axis.Length();
		if (sinAngle <= 1.0e-6f) {
			return false;
		}
		axis *= 1.0f / sinAngle;
		out.SetRotation(axis, atan2f(sinAngle, from.Dot(to)));
		return true;
	}

	int CountDepth(Skeleton& skeleton, int boneNo)
	{
		int depth = 0;
		for (int no = skeleton.GetBone(boneNo)->GetParentBoneNo(); no != -1; no = skeleton.GetBone(no)->GetParentBoneNo()) {
			depth++;
		}
		return depth;
	}
}

void SpringBones::Init(ModelRender& model, const SpringBonesSettings& settings)
{
	m_settings = settings;
	m_joints.clear();
	m_legCapsules.clear();
	m_isFirstFrame = true;
	if (!m_settings.isEnabled) {
		return;
	}
	Skeleton& skeleton = model.GetSkeleton();
	if (!skeleton.IsInited()) {
		return;
	}

	for (int boneNo = 0; boneNo < skeleton.GetNumBones(); boneNo++) {
		Bone* bone = skeleton.GetBone(boneNo);
		const std::wstring name = bone->GetName();
		if (name.compare(0, m_settings.bonePrefix.size(), m_settings.bonePrefix) != 0) {
			continue;
		}
		// 先の位置を決める子がない骨（_end など、いちばん先の骨）は動かさない。
		if (bone->GetChildren().empty()) {
			continue;
		}
		// 名前に入っている文字のうち、いちばん長いものに当てはまるまとまりにする（Skirt より CoatSkirt を先にする）。
		const SpringBoneGroupSettings* group = nullptr;
		for (const SpringBoneGroupSettings& candidate : m_settings.groups) {
			if (candidate.match.empty() || name.find(candidate.match) == std::wstring::npos) {
				continue;
			}
			if (group == nullptr || candidate.match.size() > group->match.size()) {
				group = &candidate;
			}
		}
		if (group == nullptr) {
			continue;
		}
		Joint joint;
		joint.boneNo = boneNo;
		joint.childBoneNo = bone->GetChildren().front()->GetNo();
		joint.depth = CountDepth(skeleton, boneNo);
		joint.group = group;
		m_joints.push_back(joint);
	}
	// 親の揺れを子に伝えてから子を揺らすので、根元から順に並べる。
	std::stable_sort(m_joints.begin(), m_joints.end(), [](const Joint& a, const Joint& b) {
		return a.depth < b.depth;
	});

	m_ownerBoneNo = skeleton.FindBoneID(OWNER_BONE_NAME);
	m_ownerVelocity = Vector3::Zero;
	m_time = 0.0f;

	if (m_settings.legRadius > 0.0f) {
		for (const auto& pair : LEG_BONE_PAIRS) {
			Capsule capsule;
			capsule.startBoneNo = skeleton.FindBoneID(pair[0]);
			capsule.endBoneNo = skeleton.FindBoneID(pair[1]);
			if (capsule.startBoneNo != -1 && capsule.endBoneNo != -1) {
				m_legCapsules.push_back(capsule);
			}
		}
	}
}

void SpringBones::Update(Skeleton& skeleton)
{
	if (m_isSuspended) {
		return;
	}
	const float deltaTime = min(g_gameTime->GetFrameDeltaTime(), MAX_DELTA_TIME);
	if (deltaTime <= 0.0f) {
		return;
	}

	m_time += deltaTime;
	m_burstWideLeft = max(0.0f, m_burstWideLeft - deltaTime);
	// 持ち主の動き（腰の骨のワールドでの位置の変わり方）。急に変わらないよう、少しならす。ワープ（リスポーンなど）は動きにしない。
	if (m_ownerBoneNo != -1) {
		const Vector3 ownerPosition = GetTranslation(skeleton.GetBone(m_ownerBoneNo)->GetWorldMatrix());
		if (!m_isFirstFrame && (ownerPosition - m_prevOwnerPosition).LengthSq() <= TELEPORT_DISTANCE_SQ) {
			const Vector3 rawVelocity = (ownerPosition - m_prevOwnerPosition) * (1.0f / deltaTime);
			m_ownerVelocity += (rawVelocity - m_ownerVelocity) * (1.0f - expf(-OWNER_VELOCITY_SMOOTHING * deltaTime));
			// 急に速くなった（踏み込み・ダッシュの出だし）ら、マントを後ろ・上へバサッと煽る（このフレームの揺れ物に1回だけ）。
			m_burstCooldown = max(0.0f, m_burstCooldown - deltaTime);
			const float speedUp = (rawVelocity - m_prevRawVelocity).Length() / deltaTime;
			if (m_burstCooldown <= 0.0f && speedUp > BURST_ACCELERATION && rawVelocity.LengthSq() > m_prevRawVelocity.LengthSq()) {
				Vector3 horizontal(rawVelocity.x, 0.0f, rawVelocity.z);
				if (horizontal.LengthSq() > FLT_EPSILON) {
					horizontal.Normalize();
					Burst(horizontal * -1.0f);  // 動く向きの逆（後ろ）へ。
					m_burstStrength = 1.0f;
					m_burstCooldown = BURST_COOLDOWN_SECONDS;
				}
			}
			m_prevRawVelocity = rawVelocity;
		}
		else {
			m_ownerVelocity = Vector3::Zero;
			m_prevRawVelocity = Vector3::Zero;
		}
		m_prevOwnerPosition = ownerPosition;
	}

	for (Joint& joint : m_joints) {
		Bone* bone = skeleton.GetBone(joint.boneNo);
		const Vector3 head = GetTranslation(bone->GetWorldMatrix());
		// 親の揺れを反映したあとの、アニメーションどおりの先の位置。
		const Vector3 animTail = GetTranslation(skeleton.GetBone(joint.childBoneNo)->GetWorldMatrix());
		Vector3 animDirection = animTail - head;
		const float length = animDirection.Length();
		if (length <= 1.0e-4f) {
			continue;
		}
		animDirection *= 1.0f / length;

		if (m_isFirstFrame || (animTail - joint.prevAnimTail).LengthSq() > TELEPORT_DISTANCE_SQ) {
			joint.tail = animTail;
			joint.velocity = Vector3::Zero;
			joint.prevAnimTail = animTail;
			continue;
		}

		// アニメーションどおりの先の位置へ、ばねで引き戻す。
		// 減衰はアニメーションどおりの先の速さとの差にかける（同じ速さで走っている間は遅れない）。
		const Vector3 animVelocity = (animTail - joint.prevAnimTail) * (1.0f / deltaTime);
		const float omega = Math::PI2 * joint.group->frequency;
		Vector3 acceleration =
			(animTail - joint.tail) * (omega * omega) +
			(animVelocity - joint.velocity) * (2.0f * joint.group->dampingRatio * omega);
		// 動くと後ろへ流される（動く向きの逆へ押す）。
		acceleration -= m_ownerVelocity * joint.group->windDrag;
		// 止まっていても吹く、ゆるい風。向き・強さを、周期のちがう揺らぎを重ねて変える。骨ごとに位相をずらし、マントが一枚岩に動かないようにする。
		if (joint.group->breeze > 0.0f) {
			const float phase = static_cast<float>(joint.boneNo) * BREEZE_PHASE_PER_BONE;
			const float t = m_time;
			const Vector3 gust(
				sinf(t * 1.3f + phase) + 0.5f * sinf(t * 2.9f + phase * 2.0f),
				0.4f * sinf(t * 1.7f + phase * 1.5f),
				sinf(t * 1.1f + phase * 0.7f + 1.0f) + 0.5f * sinf(t * 3.3f + phase));
			acceleration += gust * joint.group->breeze;
		}
		joint.velocity += acceleration * deltaTime;
		// 急に速くなったときの煽り（後ろ・上へ速さを足す）。
		if (m_isBurstPending && (joint.group->burstBack > 0.0f || joint.group->burstLift > 0.0f)) {
			joint.velocity += (m_burstDirection * joint.group->burstBack + Vector3::AxisY * joint.group->burstLift) * m_burstStrength;
		}
		const Vector3 movedTail = joint.tail + joint.velocity * deltaTime;

		// 骨の長さを保ち、アニメーションの形から曲がりすぎないようにする。
		Vector3 direction = movedTail - head;
		if (direction.LengthSq() <= FLT_EPSILON) {
			direction = animDirection;
		}
		direction.Normalize();
		// 煽った直後は、上限を burstMaxAngleDeg まで広げ、BURST_WIDE_SECONDS かけて元に戻す。
		float maxAngleDeg = joint.group->maxAngleDeg;
		if (joint.group->burstMaxAngleDeg > maxAngleDeg) {
			maxAngleDeg += (joint.group->burstMaxAngleDeg - maxAngleDeg) * (m_burstWideLeft / BURST_WIDE_SECONDS);
		}
		const float maxAngle = Math::DegToRad(maxAngleDeg);
		Quaternion limitRotation;
		if (direction.Dot(animDirection) < cosf(maxAngle) && MakeRotationBetween(animDirection, direction, limitRotation)) {
			Vector3 axis;
			axis.Cross(animDirection, direction);
			axis.Normalize();
			limitRotation.SetRotation(axis, maxAngle);
			direction = animDirection;
			limitRotation.Apply(direction);
		}
		// ジャンプ中は、アニメーションの形より上へ上がらない（向きの高さをアニメーションの高さまで下げる）。
		// 左右・前後の揺れは残す。脚に押し出す前に行うので、脚に押される分は上がってよい。
		if (joint.group->noLiftInJump && m_jumpWeight > 0.0f && direction.y > animDirection.y) {
			Vector3 horizontal(direction.x, 0.0f, direction.z);
			if (horizontal.LengthSq() <= 1.0e-6f) {
				horizontal = Vector3(animDirection.x, 0.0f, animDirection.z);
			}
			if (horizontal.LengthSq() > 1.0e-6f) {
				horizontal.Normalize();
				const float horizontalLength = sqrtf(max(0.0f, 1.0f - animDirection.y * animDirection.y));
				const Vector3 lowered = horizontal * horizontalLength + Vector3::AxisY * animDirection.y;
				direction = direction + (lowered - direction) * m_jumpWeight;
				direction.Normalize();
			}
		}
		Vector3 tail = head + direction * length;

		// 脚にめり込まないように押し出し、もう一度長さをそろえる。
		if (!m_legCapsules.empty()) {
			tail = PushOutFromLegs(skeleton, tail, animTail);
			direction = tail - head;
			if (direction.LengthSq() <= FLT_EPSILON) {
				direction = animDirection;
			}
			direction.Normalize();
			tail = head + direction * length;
		}

		// ジャンプ中は、真下から jumpMaxAngleFromDownDeg より上へ上げない（脚に押したあとに抑える）。
		// スカートの骨はアニメーションの中で太ももと一緒に動き、跳んで太ももを上げると水平より上まで上がって中が見えるため。
		// 太ももを大きく上げたときは、太ももがスカートに少し入り込む。
		if (joint.group->noLiftInJump && m_jumpWeight > 0.0f && m_settings.jumpMaxAngleFromDownDeg > 0.0f) {
			const float maxAngle = Math::DegToRad(m_settings.jumpMaxAngleFromDownDeg);
			const float angle = acosf(max(-1.0f, min(1.0f, -direction.y)));
			if (angle > maxAngle) {
				Vector3 horizontal(direction.x, 0.0f, direction.z);
				if (horizontal.LengthSq() > 1.0e-6f) {
					horizontal.Normalize();
					const float limited = angle + (maxAngle - angle) * m_jumpWeight;
					direction = horizontal * sinf(limited) + Vector3::AxisY * -cosf(limited);
					tail = head + direction * length;
				}
			}
		}
		joint.velocity = (tail - joint.tail) * (1.0f / deltaTime);
		joint.tail = tail;
		joint.prevAnimTail = animTail;

		// アニメーションの向きから揺れた向きへ、根元を中心に骨と子孫を回す。
		Quaternion rotation;
		if (!MakeRotationBetween(animDirection, direction, rotation)) {
			continue;
		}
		Matrix toOrigin, rotationMatrix, back;
		toOrigin.MakeTranslation(head * -1.0f);
		rotationMatrix.MakeRotationFromQuaternion(rotation);
		back.MakeTranslation(head);
		PoseLayers::RotateBoneTree(*bone, toOrigin * rotationMatrix * back);
	}
	m_isBurstPending = false;
	m_isFirstFrame = false;
}

Vector3 SpringBones::PushOutFromLegs(Skeleton& skeleton, const Vector3& tail, const Vector3& animTail) const
{
	Vector3 result = tail;
	for (const Capsule& capsule : m_legCapsules) {
		const Vector3 start = GetTranslation(skeleton.GetBone(capsule.startBoneNo)->GetWorldMatrix());
		const Vector3 end = GetTranslation(skeleton.GetBone(capsule.endBoneNo)->GetWorldMatrix());
		// アニメーションの形がもとから脚に近いときは、その近さまでは許す（アニメーションの形は変えない）。
		const float animDistance = (animTail - ClosestPointOnSegment(animTail, start, end)).Length();
		const float radius = min(m_settings.legRadius, animDistance);
		const Vector3 closest = ClosestPointOnSegment(result, start, end);
		Vector3 away = result - closest;
		const float distance = away.Length();
		if (distance >= radius) {
			continue;
		}
		if (distance <= 1.0e-4f) {
			away = animTail - closest;
			if (away.LengthSq() <= FLT_EPSILON) {
				continue;
			}
		}
		away.Normalize();
		result = closest + away * radius;
	}
	return result;
}
