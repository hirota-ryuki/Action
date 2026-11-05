#include "stdafx.h"
#include "IceMagic.h"
#include "EffectId.h"
#include "Enemy.h"

namespace
{
	// エフェクトは Effekseer 用に自作したもの(Assets/effect/ice_*.efkproj が編集元)。
	// テクスチャは Effekseer サンプル(01_NextSoft01, CC-0)のものを使用。
	constexpr int kBulletEffectId = static_cast<int>(EffectId::IceBullet);
	constexpr int kHitEffectId = static_cast<int>(EffectId::IceHit);
	constexpr const char16_t* kBulletEffectPath = u"Assets/effect/ice_bullet.efk";
	constexpr const char16_t* kHitEffectPath = u"Assets/effect/ice_hit.efk";

	const Vector3 kEffectScale = { 10.0f, 10.0f, 10.0f };  // Effekseerの1単位をワールド10単位に拡大（炎と同じ）。
	constexpr float kSpeed = 1500.0f;          // 弾の速さ（毎秒）。
	constexpr float kRange = 2000.0f;          // 射程。これだけ飛ぶと消える。
	constexpr float kEnemyHitRadius = 55.0f;   // 敵の中心からこの距離以内を通ると命中。
	constexpr float kEnemyCenterHeight = 60.0f;// 敵の足元から当たり判定の中心までの高さ。
	constexpr float kFreezeSeconds = 2.5f;     // 命中した敵を凍らせる時間。

	// 線分 from→to 上で point に最も近い点を返す。
	Vector3 ClosestPointOnSegment(const Vector3& from, const Vector3& to, const Vector3& point)
	{
		Vector3 segment = to - from;
		const float lengthSq = segment.LengthSq();
		if (lengthSq <= FLT_EPSILON) {
			return from;
		}
		float t = (point - from).Dot(segment) / lengthSq;
		t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
		return from + segment * t;
	}
}

IceMagic::IceMagic()
{

}

IceMagic::~IceMagic()
{
	// Effect のデストラクタは再生を止めないため、ここで明示的に止める。
	// 通常は両方とも再生が終わってから削除されるので何も起きない。
	if (!m_isValid || EffectEngine::GetInstance() == nullptr) {
		return;
	}
	m_bulletEffect.Stop();
	m_hitEffect.Stop();
}

void IceMagic::Init(const Vector3& position, const Vector3& direction)
{
	m_position = position;
	m_direction = direction;
	if (m_direction.LengthSq() <= FLT_EPSILON) {
		m_direction = Vector3::AxisZ;
	}
	m_direction.Normalize();
	m_rotation.SetRotationYFromDirectionXZ(m_direction);
}

bool IceMagic::Start()
{
	EffectEngine* effectEngine = EffectEngine::GetInstance();
	if (effectEngine == nullptr) {
		K2_ASSERT(false, "EffectEngine が未生成のため氷魔法のエフェクトを再生できません。");
		return true;
	}

	// 登録済みの番号は内部で無視されるので、弾を何発撃っても読み込みは1回だけ。
	effectEngine->ResistEffect(kBulletEffectId, kBulletEffectPath);
	effectEngine->ResistEffect(kHitEffectId, kHitEffectPath);
	if (effectEngine->LoadEffect(kBulletEffectId) == nullptr || effectEngine->LoadEffect(kHitEffectId) == nullptr) {
		K2_ASSERT(false, "氷魔法のエフェクト(Assets/effect/ice_bullet.efk, ice_hit.efk)の読み込みに失敗しました。");
		return true;
	}

	m_bulletEffect.Init(kBulletEffectId);
	m_hitEffect.Init(kHitEffectId);
	m_isValid = true;

	m_bulletEffect.Play();
	// 新しいハンドルには行列が設定されていないので、再生直後に必ず設定する。
	ApplyBulletTransform();

	return true;
}

void IceMagic::Update()
{
	if (!m_isValid) {
		DeleteGO(this);
		return;
	}

	switch (m_state) {
	case EnState::Flying:
		UpdateFlying();
		break;
	case EnState::Vanishing:
		UpdateVanishing();
		break;
	}
}

void IceMagic::UpdateFlying()
{
	const float moveDistance = kSpeed * g_gameTime->GetFrameDeltaTime();
	const Vector3 nextPosition = m_position + m_direction * moveDistance;

	// 敵 → 地形の順に調べる。地形のレイは敵のカプセルにも当たるが、
	// 敵の判定半径の方が大きいので、敵に向かって撃てば先に敵として命中する。
	Vector3 hitPosition;
	Enemy* hitEnemy = FindHitEnemy(m_position, nextPosition, hitPosition);
	if (hitEnemy != nullptr) {
		hitEnemy->Freeze(kFreezeSeconds);
		Explode(hitPosition);
		return;
	}

	PhysicsWorld* physicsWorld = PhysicsWorld::GetInstance();
	if (physicsWorld != nullptr && physicsWorld->RayTest(m_position, nextPosition, hitPosition)) {
		Explode(hitPosition);
		return;
	}

	m_position = nextPosition;
	m_flightDistance += moveDistance;
	if (m_flightDistance >= kRange) {
		// 射程切れは空中で砕ける演出にする。
		Explode(m_position);
		return;
	}

	ApplyBulletTransform();
}

void IceMagic::UpdateVanishing()
{
	// 軌跡のパーティクルと着弾エフェクトが両方消え切ったら削除する。
	if (!m_bulletEffect.IsPlay() && !m_hitEffect.IsPlay()) {
		DeleteGO(this);
	}
}

Enemy* IceMagic::FindHitEnemy(const Vector3& from, const Vector3& to, Vector3& hitPosition) const
{
	Enemy* hitEnemy = nullptr;
	float nearestDistanceSq = kEnemyHitRadius * kEnemyHitRadius;
	QueryGOs<Enemy>("enemy", [&](Enemy* enemy) {
		if (enemy->IsDead()) {
			return true;
		}
		Vector3 center = enemy->GetPosition();
		center.y += kEnemyCenterHeight;
		const Vector3 closest = ClosestPointOnSegment(from, to, center);
		const float distanceSq = (center - closest).LengthSq();
		if (distanceSq <= nearestDistanceSq) {
			nearestDistanceSq = distanceSq;
			hitEnemy = enemy;
			hitPosition = closest;
		}
		return true;
	});
	return hitEnemy;
}

void IceMagic::Explode(const Vector3& hitPosition)
{
	m_position = hitPosition;
	ApplyBulletTransform();
	// 弾本体は即座に消し、軌跡のパーティクルは寿命まで残す。
	m_bulletEffect.StopRoot();

	m_hitEffect.Play();
	m_hitEffect.SetPosition(hitPosition);
	m_hitEffect.SetRotation(m_rotation);
	m_hitEffect.SetScale(kEffectScale);
	m_hitEffect.Update();

	m_state = EnState::Vanishing;
}

void IceMagic::ApplyBulletTransform()
{
	m_bulletEffect.SetPosition(m_position);
	m_bulletEffect.SetRotation(m_rotation);
	m_bulletEffect.SetScale(kEffectScale);
	// 全体ではなくルートだけを動かし、軌跡のパーティクルを発生した場所に残す。
	m_bulletEffect.UpdateRoot();
}
