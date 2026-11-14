#include "stdafx.h"
#include "FireMagic.h"
#include "BurningGround.h"
#include "EffectId.h"
#include "HitTarget.h"

namespace
{
	// エフェクトは Effekseer 用に自作したもの(Assets/effect/fire_*.efkproj が編集元)。
	// テクスチャは Effekseer サンプル(01_Pierre01, CC-0)のものを使用。
	struct EffectEntry
	{
		EffectId id;
		const char16_t* path;
	};
	// 炎魔法で使うエフェクトの一覧。BurningGround もこの登録を使う。
	constexpr EffectEntry EFFECTS[] = {
		{ EffectId::FireFly, u"Assets/effect/fire_fly.efk" },
		{ EffectId::FireCharge, u"Assets/effect/fire_charge.efk" },
		{ EffectId::FireCharged, u"Assets/effect/fire_charged.efk" },
		{ EffectId::FireHit, u"Assets/effect/fire_hit.efk" },
		{ EffectId::FireGround, u"Assets/effect/fire_ground.efk" },
	};

	constexpr float MAX_CHARGE_SECONDS = 1.2f;  // 最大まで溜まる秒数。
	constexpr float FULL_PULSE_SPEED = 18.0f;   // 最大まで溜まった後に火の玉が脈打つ速さ（ラジアン/秒）。
	constexpr float FULL_PULSE_DEPTH = 0.12f;   // 脈打ちで大きさが変わる割合。

	// 溜めなし(0)〜最大まで溜めた(1)ときの値。間は線形に補間する。
	constexpr float MIN_RADIUS = 14.0f;               // 当たり判定の半径。
	constexpr float MAX_RADIUS = 45.0f;
	constexpr float MIN_FLY_EFFECT_SCALE = 8.0f;      // 飛ぶ炎（と溜め中の火の玉）のエフェクトの拡大率。
	constexpr float MAX_FLY_EFFECT_SCALE = 26.0f;
	constexpr float MIN_HIT_EFFECT_SCALE = 12.0f;     // 着弾エフェクトの拡大率。
	constexpr float MAX_HIT_EFFECT_SCALE = 26.0f;
	constexpr float MIN_GROUND_RADIUS = 130.0f;       // 炎魔法発射後の燃える床の半径。
	constexpr float MAX_GROUND_RADIUS = 300.0f;

	constexpr float SPEED = 1300.0f;
	constexpr float RANGE = 2000.0f;
	constexpr float GROUND_BURN_SECONDS = 4.0f;       // 床が燃えている時間。

	// 炎魔法発射後の燃える床を置く場所を探すレイ。着弾点の少し手前・少し上から真下へ飛ばす。
	constexpr float FLOOR_RAY_BACK_OFF = 20.0f;       // 壁に当たったとき、壁にめり込まないよう手前に戻す距離。
	constexpr float FLOOR_RAY_LIFT = 10.0f;
	constexpr float FLOOR_SEARCH_DEPTH = 400.0f;      // これより下に床がなければ床は燃えない。

	float Lerp(float rate, float a, float b)
	{
		return a + (b - a) * rate;
	}

	MagicSettings MakeSettings(float chargeRate)
	{
		MagicSettings settings;
		settings.speed = SPEED;
		settings.range = RANGE;
		settings.radius = Lerp(chargeRate, MIN_RADIUS, MAX_RADIUS);
		return settings;
	}

	// 溜め具合に応じた、飛ぶ炎のエフェクトの拡大率。溜め中の火の玉も同じ大きさにする。
	Vector3 GetFlyEffectScale(float chargeRate)
	{
		const float scale = Lerp(chargeRate, MIN_FLY_EFFECT_SCALE, MAX_FLY_EFFECT_SCALE);
		return Vector3(scale, scale, scale);
	}
}

FireMagic::FireMagic()
	: MagicBase(MakeSettings(0.0f))
{
}

FireMagic::~FireMagic()
{
	// Effect のデストラクタは再生を止めないため、ここで明示的に止める。
	// 通常はすべて再生が終わってから削除されるので何も起きない。
	if (!m_isEffectReady || EffectEngine::GetInstance() == nullptr) {
		return;
	}
	m_chargeEffect.Stop();
	m_chargedEffect.Stop();
	m_flyEffect.Stop();
	m_hitEffect.Stop();
}

bool FireMagic::RegisterEffects()
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

bool FireMagic::OnStart()
{
	if (!RegisterEffects()) {
		return false;
	}

	m_chargeEffect.Init(static_cast<int>(EffectId::FireCharge));
	m_chargedEffect.Init(static_cast<int>(EffectId::FireCharged));
	m_flyEffect.Init(static_cast<int>(EffectId::FireFly));
	m_hitEffect.Init(static_cast<int>(EffectId::FireHit));
	m_isEffectReady = true;

	// 新しいハンドルには行列が設定されていないので、再生直後に OnCharge() で必ず設定される。
	m_chargeEffect.Play();
	return true;
}

void FireMagic::OnCharge(float chargeSeconds)
{
	m_chargeRate = min(chargeSeconds / MAX_CHARGE_SECONDS, 1.0f);
	if (m_chargeRate >= 1.0f && !m_isFullCharged) {
		// 溜まり切った瞬間に一度だけ閃光を出す。
		m_isFullCharged = true;
		m_fullChargeSeconds = chargeSeconds;
		m_chargedEffect.Play();
	}

	m_chargeEffect.SetPosition(GetPosition());
	m_chargeEffect.SetRotation(GetRotation());
	// 飛ぶ炎と同じ大きさにして、溜めた火の玉がそのまま飛んでいくように見せる。
	Vector3 scale = GetFlyEffectScale(m_chargeRate);
	if (m_isFullCharged) {
		// 溜まり切った後は脈打たせて、撃てる状態だと分かるようにする。
		scale *= 1.0f + FULL_PULSE_DEPTH * sinf((chargeSeconds - m_fullChargeSeconds) * FULL_PULSE_SPEED);
	}
	m_chargeEffect.SetScale(scale);
	m_chargeEffect.UpdateRoot();

	if (m_isFullCharged) {
		// 閃光は火の玉についていかせる。
		m_chargedEffect.SetPosition(GetPosition());
		m_chargedEffect.SetRotation(GetRotation());
		m_chargedEffect.SetScale(GetFlyEffectScale(1.0f));
		m_chargedEffect.Update();
	}
}

void FireMagic::OnLaunch()
{
	// 手元の火の玉は即座に消し（立ち上る炎と煙は寿命まで残す）、同じ大きさの炎を飛ばす。
	m_chargeEffect.StopRoot();
	SetSettings(MakeSettings(m_chargeRate));
	m_flyEffect.Play();
}

void FireMagic::OnCancel()
{
	// 溜めは捨てる（飛ばさない）。火の玉は即座に消し、立ち上る炎と煙は寿命まで残す。
	m_chargeEffect.StopRoot();
}

void FireMagic::OnMove()
{
	m_flyEffect.SetPosition(GetPosition());
	m_flyEffect.SetRotation(GetRotation());
	m_flyEffect.SetScale(GetFlyEffectScale(m_chargeRate));
	// 全体ではなくルートだけを動かし、炎と煙の尾を通った場所に残す。
	m_flyEffect.UpdateRoot();
}

void FireMagic::OnHitTarget(HitTarget& target, const Vector3& hitPosition)
{
	(void)hitPosition;
	// 直撃では何もしない。相手は足元の炎魔法発射後の燃える床に入るので、そこで燃焼状態になる。
	m_hasHitTarget = true;
	m_hitFootPosition = target.GetFootPosition();
}

void FireMagic::OnExplode(const Vector3& hitPosition)
{
	// 飛んでいる炎は即座に消し、尾の炎と煙は寿命まで残す。
	m_flyEffect.StopRoot();

	const float hitScale = Lerp(m_chargeRate, MIN_HIT_EFFECT_SCALE, MAX_HIT_EFFECT_SCALE);
	m_hitEffect.Play();
	m_hitEffect.SetPosition(hitPosition);
	m_hitEffect.SetScale(Vector3(hitScale, hitScale, hitScale));
	m_hitEffect.Update();

	SpawnBurningGround(hitPosition);
}

bool FireMagic::IsVanishFinished() const
{
	// 溜めの名残・閃光・尾・着弾エフェクトがすべて消え切ったら終わり。
	// 炎魔法発射後の燃える床は別のオブジェクトなので待たない。
	return !m_chargeEffect.IsPlay() && !m_chargedEffect.IsPlay()
		&& !m_flyEffect.IsPlay() && !m_hitEffect.IsPlay();
}

void FireMagic::SpawnBurningGround(const Vector3& hitPosition)
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
		Lerp(m_chargeRate, MIN_GROUND_RADIUS, MAX_GROUND_RADIUS),
		GROUND_BURN_SECONDS,
		GetOwner(),
		GetFaction());
}
