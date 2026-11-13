#include "stdafx.h"
#include "FireBullet.h"
#include "BurningGround.h"
#include "EffectId.h"
#include "HitTarget.h"

namespace
{
	// エフェクトは Effekseer 用に自作したもの(Assets/effect/fire_*.efkproj が編集元)。
	// テクスチャは Effekseer サンプル(01_Pierre01, CC-0)のものを使用。
	constexpr int BULLET_EFFECT_ID = static_cast<int>(EffectId::FireBullet);
	constexpr int HIT_EFFECT_ID = static_cast<int>(EffectId::FireHit);

	struct EffectEntry
	{
		EffectId id;
		const char16_t* path;
	};
	// 炎魔法で使うエフェクトの一覧。FireMagic・BurningGround もこの登録を使う。
	constexpr EffectEntry EFFECTS[] = {
		{ EffectId::FireBullet, u"Assets/effect/fire_bullet.efk" },
		{ EffectId::FireCharge, u"Assets/effect/fire_charge.efk" },
		{ EffectId::FireCharged, u"Assets/effect/fire_charged.efk" },
		{ EffectId::FireHit, u"Assets/effect/fire_hit.efk" },
		{ EffectId::FireGround, u"Assets/effect/fire_ground.efk" },
	};

	// 溜めなし(0)〜最大まで溜めた(1)ときの値。間は線形に補間する。
	// 弾の速さ・半径、燃える床の広さなど強さに関わる値は magic.json（FireMagicSettings）にある。
	constexpr float MIN_BULLET_EFFECT_SCALE = 8.0f;     // 弾のエフェクトの拡大率。
	constexpr float MAX_BULLET_EFFECT_SCALE = 26.0f;
	constexpr float MIN_HIT_EFFECT_SCALE = 12.0f;       // 着弾エフェクトの拡大率。
	constexpr float MAX_HIT_EFFECT_SCALE = 26.0f;

	// 燃える床を置く場所を探すレイ。着弾点の少し手前・少し上から真下へ飛ばす。
	constexpr float FLOOR_RAY_BACK_OFF = 20.0f;         // 壁に当たったとき、壁にめり込まないよう手前に戻す距離。
	constexpr float FLOOR_RAY_LIFT = 10.0f;
	constexpr float FLOOR_SEARCH_DEPTH = 400.0f;       // これより下に床がなければ床は燃えない。

	float Lerp(float rate, float a, float b)
	{
		return a + (b - a) * rate;
	}

	BulletSettings MakeSettings(const FireMagicSettings& magicSettings, float chargeRate)
	{
		BulletSettings settings;
		settings.speed = magicSettings.speed;
		settings.range = magicSettings.range;
		settings.radius = Lerp(chargeRate, magicSettings.minRadius, magicSettings.maxRadius);
		return settings;
	}
}

FireBullet::FireBullet()
	: BulletBase(MakeSettings(FireMagicSettings(), 0.0f))
{
}

FireBullet::~FireBullet()
{
	// Effect のデストラクタは再生を止めないため、ここで明示的に止める。
	// 通常は両方とも再生が終わってから削除されるので何も起きない。
	if (!m_isEffectReady || EffectEngine::GetInstance() == nullptr) {
		return;
	}
	m_bulletEffect.Stop();
	m_hitEffect.Stop();
}

void FireBullet::Setup(const FireMagicSettings& settings, float chargeRate)
{
	m_settings = settings;
	m_chargeRate = chargeRate < 0.0f ? 0.0f : (chargeRate > 1.0f ? 1.0f : chargeRate);
	SetSettings(MakeSettings(m_settings, m_chargeRate));
}

bool FireBullet::RegisterEffects()
{
	EffectEngine* effectEngine = EffectEngine::GetInstance();
	if (effectEngine == nullptr) {
		K2_ASSERT(false, "EffectEngine が未生成のため炎魔法のエフェクトを再生できません。");
		return false;
	}

	// 登録済みの番号は内部で無視されるので、何度呼んでも読み込みは1回だけ。
	for (const EffectEntry& entry : EFFECTS) {
		const int id = static_cast<int>(entry.id);
		effectEngine->ResistEffect(id, entry.path);
		if (effectEngine->LoadEffect(id) == nullptr) {
			K2_ASSERT(false, "炎魔法のエフェクト(Assets/effect/fire_*.efk)の読み込みに失敗しました。");
			return false;
		}
	}
	return true;
}

Vector3 FireBullet::GetBulletEffectScale(float chargeRate)
{
	const float scale = Lerp(chargeRate, MIN_BULLET_EFFECT_SCALE, MAX_BULLET_EFFECT_SCALE);
	return Vector3(scale, scale, scale);
}

bool FireBullet::OnStart()
{
	if (!RegisterEffects()) {
		return false;
	}

	m_bulletEffect.Init(BULLET_EFFECT_ID);
	m_hitEffect.Init(HIT_EFFECT_ID);
	m_isEffectReady = true;

	// 新しいハンドルには行列が設定されていないので、再生直後に OnMove() で必ず設定される。
	m_bulletEffect.Play();
	return true;
}

void FireBullet::OnMove()
{
	m_bulletEffect.SetPosition(GetPosition());
	m_bulletEffect.SetRotation(GetRotation());
	m_bulletEffect.SetScale(GetBulletEffectScale(m_chargeRate));
	// 全体ではなくルートだけを動かし、炎と煙の尾を通った場所に残す。
	m_bulletEffect.UpdateRoot();
}

void FireBullet::OnHitTarget(HitTarget& target, const Vector3& hitPosition)
{
	(void)hitPosition;
	// 直撃では何もしない。相手は足元の燃える床に入るので、そこで燃焼状態になる。
	m_hasHitTarget = true;
	m_hitFootPosition = target.GetFootPosition();
}

void FireBullet::OnExplode(const Vector3& hitPosition)
{
	// 弾本体は即座に消し、尾の炎と煙は寿命まで残す。
	m_bulletEffect.StopRoot();

	const float hitScale = Lerp(m_chargeRate, MIN_HIT_EFFECT_SCALE, MAX_HIT_EFFECT_SCALE);
	m_hitEffect.Play();
	m_hitEffect.SetPosition(hitPosition);
	m_hitEffect.SetScale(Vector3(hitScale, hitScale, hitScale));
	m_hitEffect.Update();

	SpawnBurningGround(hitPosition);
}

bool FireBullet::IsVanishFinished() const
{
	// 尾と着弾エフェクトが両方消え切ったら終わり。燃える床は別のオブジェクトなので待たない。
	return !m_bulletEffect.IsPlay() && !m_hitEffect.IsPlay();
}

void FireBullet::SpawnBurningGround(const Vector3& hitPosition)
{
	PhysicsWorld* physicsWorld = PhysicsWorld::GetInstance();
	if (physicsWorld == nullptr) {
		return;
	}

	// 相手に当たったらその足元から、地形・射程切れなら着弾点の少し手前から、真下の床を探す。
	// 地形のレイはキャラクターのカプセルにも当たるので、相手に当たったときは足元から調べる。
	Vector3 rayStart = m_hasHitTarget ? m_hitFootPosition : hitPosition - GetDirection() * FLOOR_RAY_BACK_OFF;
	rayStart.y += FLOOR_RAY_LIFT;
	Vector3 rayEnd = rayStart;
	rayEnd.y -= FLOOR_SEARCH_DEPTH;

	Vector3 floorPosition;
	if (!physicsWorld->RayTest(rayStart, rayEnd, floorPosition)) {
		// 下に床がない（崖の外など）。床は燃やさない。
		return;
	}

	BurningGround* ground = NewGO<BurningGround>(1, BurningGround::OBJECT_NAME);
	ground->Init(
		floorPosition,
		Lerp(m_chargeRate, m_settings.minGroundRadius, m_settings.maxGroundRadius),
		m_settings.groundBurnSeconds,
		GetOwner(),
		GetFaction());
}
