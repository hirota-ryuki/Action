#include "stdafx.h"
#include "Character.h"

Character::Character(Faction faction)
	: m_faction(faction)
{
}

bool Character::Start()
{
	const CharacterConfig config = MakeConfig();

	// キャラコンは SetPosition() で決めた位置に作る。
	m_mover.Init(config.move);
	SyncTransform();

	// モデルを、ModelRender を使う部品（状態異常・派生クラスの部品）より先に Init する。
	m_model.Init(config.model);
	m_model.SetTransform(m_transform.position, m_transform.rotation);
	m_model.Update();
	m_bodyLean.Init(config.lean);
	m_springBones.Init(m_model.GetModelRender(), config.springBones);

	m_statusEffects.Init(&m_model.GetModelRender());

	m_hitTarget.Init(this, m_faction, config.hitRadius, config.hitCenterHeight);
	m_hitTarget.SetStatusEffects(&m_statusEffects);
	m_hitTarget.SetPosition(m_transform.position);

	m_magicCaster.Init(this, m_faction, config.caster);

	GetStatus().Init(config.maxHp);

	OnStart();
	return true;
}

void Character::Update()
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();

	// ① 入力
	UpdateInput(deltaTime);
	// ② 状態異常
	m_statusEffects.Update(deltaTime);
	// ③ 動く（派生クラス）
	UpdateAction(deltaTime);
	// ④ 魔法（動いたあとの位置から撃つ）
	m_magicCaster.Update(m_input, m_mover.GetPosition(), m_mover.GetForward(), deltaTime, m_statusEffects.CanAct());
	// ⑤ Transform に反映
	SyncTransform();
	// ⑥ 当たり判定・モデルを合わせる
	m_hitTarget.SetPosition(m_transform.position);
	// 見た目だけ、速さが変わるのに合わせて前後に傾ける（髪・スカートの揺れは m_model.Update() の中で動く）。
	// 向きは、本当の向きへ少しずつ回る見た目の向きを使う。
	m_model.SetTransform(m_transform.position, m_bodyLean.Update(m_transform.position, m_mover.GetVisualRotation(), deltaTime));
	m_model.Update();
	// ⑦ モデルを更新したあとの処理（派生クラス）
	OnLateUpdate(deltaTime);
}

void Character::Render(RenderContext& rc)
{
	if (!m_isRenderBody) {
		return;
	}
	m_model.Draw(rc);
	OnRender(rc);
}

void Character::SetPosition(const Vector3& position)
{
	m_transform.position = position;
	m_mover.SetPosition(position);
}

void Character::SetFaction(Faction faction)
{
	m_faction = faction;
	m_hitTarget.SetFaction(faction);
	m_magicCaster.SetFaction(faction);
}

void Character::UpdateInput(float deltaTime)
{
	InputSourceContext context;
	context.self = this;
	context.faction = m_faction;
	context.position = m_transform.position;
	context.forward = GetForward();
	context.deltaTime = deltaTime;
	FillInputContext(context);

	IInputSource& inputSource = m_inputSource != nullptr ? *m_inputSource : GetDefaultInputSource();
	inputSource.Update(context, m_input);
}

void Character::SyncTransform()
{
	m_transform.position = m_mover.GetPosition();
	m_transform.rotation = m_mover.GetRotation();
}
