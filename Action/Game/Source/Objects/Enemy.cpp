#include "stdafx.h"
#include "Enemy.h"
#include "IceMagic.h"

namespace
{
	constexpr float kHitRadius = 45.0f;        // 弾に当たる球の半径。
	constexpr float kHitCenterHeight = 60.0f;  // 足元から当たり判定の中心までの高さ。

	CharacterMoveSettings MakeMoveSettings()
	{
		CharacterMoveSettings settings;
		settings.walkSpeed = 140.0f;
		settings.gravity = 980.0f;
		settings.capsuleRadius = 20.0f;
		settings.capsuleHeight = 100.0f;
		return settings;
	}

	NpcInputSource::Settings MakeAiSettings()
	{
		NpcInputSource::Settings settings;
		settings.searchRange = 900.0f;   // この距離以内に入ると追跡を開始する。
		settings.stopDistance = 90.0f;   // この距離まで近づいたら停止する（近づきすぎ防止）。
		settings.canFire = true;
		settings.fireRange = 700.0f;
		settings.fireInterval = 4.0f;    // 凍結(2.5秒)より長くして、凍らされ続けないようにする。
		settings.firstFireDelay = 1.5f;
		settings.castHeight = 60.0f;
		return settings;
	}

	CharacterModelDesc MakeModelDesc()
	{
		CharacterModelDesc desc;
		desc.modelPath = "Assets/modelData/unityChan.tkm";
		desc.upAxis = enModelUpAxisY;
		desc.scale = 1.08f;
		desc.clips.resize(Enemy::enAnimClip_Num);
		desc.clips[Enemy::enAnimClip_Idle] = { "Assets/animData/idle.tka", true };
		desc.clips[Enemy::enAnimClip_Run] = { "Assets/animData/run.tka", true };
		return desc;
	}

	MagicCaster::Settings MakeCasterSettings()
	{
		MagicCaster::Settings settings;
		settings.castHeight = 60.0f;
		settings.castForward = 60.0f;  // 自分のカプセル(半径20)に当たらないよう前に出して撃つ。
		return settings;
	}
}

Enemy::Enemy()
	: m_npcInputSource(MakeAiSettings())
{

}

Enemy::~Enemy()
{

}

bool Enemy::Start()
{
	m_mover.Init(MakeMoveSettings());

	m_model.Init(MakeModelDesc());
	m_model.SetTransform(m_mover.GetPosition(), m_mover.GetRotation());
	m_model.PlayAnimation(enAnimClip_Idle);
	m_model.Update();

	m_statusEffects.Init(&m_model.GetModelRender());

	m_hitTarget.Init(this, m_faction, kHitRadius, kHitCenterHeight);
	m_hitTarget.SetStatusEffects(&m_statusEffects);
	m_hitTarget.SetPosition(m_mover.GetPosition());

	m_magicCaster.Init(this, m_faction, MakeCasterSettings());
	m_magicCaster.AddMagic(std::make_unique<IceMagic>());

	return true;
}

void Enemy::SetFaction(Faction faction)
{
	m_faction = faction;
	m_hitTarget.SetFaction(faction);
	m_magicCaster.SetFaction(faction);
}

void Enemy::Update()
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();

	InputSourceContext context;
	context.self = this;
	context.faction = m_faction;
	context.position = m_mover.GetPosition();
	context.forward = m_mover.GetForward();
	context.deltaTime = deltaTime;
	m_inputSource->Update(context, m_input);

	m_statusEffects.Update(deltaTime);
	const bool canAct = m_statusEffects.CanAct();
	if (canAct) {
		m_mover.Update(m_input, deltaTime);
		UpdateAnimation();
	}
	else {
		// 凍結中は入力を無視し、重力で落ちるだけにする。
		m_mover.Update(CharacterInput(), deltaTime);
	}
	m_magicCaster.Update(m_input, m_mover.GetPosition(), m_mover.GetForward(), deltaTime, canAct);

	m_hitTarget.SetPosition(m_mover.GetPosition());

	m_model.SetTransform(m_mover.GetPosition(), m_mover.GetRotation());
	m_model.Update();
}

void Enemy::Render(RenderContext& rc)
{
	m_model.Draw(rc);
}

void Enemy::UpdateAnimation()
{
	const int newAnimNo = m_mover.IsMoving() ? enAnimClip_Run : enAnimClip_Idle;

	if (newAnimNo != m_currentAnimNo) {
		m_model.PlayAnimation(newAnimNo, 0.2f);
		m_currentAnimNo = newAnimNo;
	}
}
