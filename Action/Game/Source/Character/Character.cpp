#include "stdafx.h"
#include "Character.h"

namespace
{
	// 撃つ位置をずらしたとき（aimOffsets）、照準のこの距離先の点へ向けて撃つ。
	constexpr float AIM_POINT_DISTANCE = 3000.0f;
}

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
	m_bodyLink.Init(m_model.GetModelRender(), config.bodyLink);
	// アニメーションの姿勢ができたあとに、体の連動 → 揺れ物の順に手を加える（揺れ物は連動した体の動きに遅れて付いてくる）。
	if (m_bodyLink.IsActive() || m_springBones.IsActive()) {
		m_model.GetModelRender().GetSkeleton().SetOnPostUpdateWorldMatrix([this](Skeleton& skeleton) {
			m_bodyLink.Update(skeleton);
			m_springBones.Update(skeleton);
		});
	}

	m_statusEffects.Init(&m_model.GetModelRender());

	m_hitTarget.Init(this, m_faction, config.hitRadius, config.hitCenterHeight);
	m_hitTarget.SetStatusEffects(&m_statusEffects);
	m_hitTarget.SetPosition(m_transform.position);

	m_castSettings = config.cast;
	m_skillController.Init(this, m_faction);

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
	// ④ スキル（動いたあとの位置から撃つ）
	UpdateSkill();
	// ⑤ Transform に反映
	SyncTransform();
	// ⑥ 当たり判定・モデルを合わせる
	m_hitTarget.SetPosition(m_transform.position);
	// 見た目だけ、速さが変わるのに合わせて前後に傾ける（髪・スカートの揺れは m_model.Update() の中で動く）。
	// 向きは、本当の向きへ少しずつ回る見た目の向きを使う。
	const Quaternion modelRotation = m_bodyLean.Update(m_transform.position, m_mover.GetVisualRotation(), deltaTime);
	m_model.SetTransform(m_transform.position, modelRotation);
	m_bodyLink.SetRotation(modelRotation);
	// 走っている度合い（歩く速さで 0、走る速さで 1）。走っているときだけ効かせる調整に使う。
	{
		const CharacterMoveSettings& move = m_mover.GetSettings();
		const float runSpeed = move.walkSpeed * move.runSpeedMultiplier;
		const float runWeight = runSpeed > move.walkSpeed
			? (m_mover.GetHorizontalSpeed() - move.walkSpeed) / (runSpeed - move.walkSpeed)
			: 0.0f;
		m_bodyLink.SetRunWeight(max(0.0f, min(1.0f, runWeight)));
	}
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
	m_skillController.SetFaction(faction);
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

void Character::UpdateSkill()
{
	// 凍っているなど動けないときは、溜めているスキルを捨てて何もしない。
	if (!m_statusEffects.CanAct()) {
		m_skillController.CancelAll();
		return;
	}

	Vector3 origin;
	Vector3 direction;
	MakeCastPose(origin, direction);
	// 派生クラスが入力の写しを書き換える（攻撃のモーションに合わせて撃つ、など）。
	CharacterInput castInput = m_input;
	FilterSkillInput(castInput);
	// ボタンごとの撃つ位置と向き。入力にずらす量（aimOffsets）があれば、そのボタンのスキルだけずらす。
	SkillPose poses[static_cast<int>(ActionButton::Num)];
	for (int button = 0; button < static_cast<int>(ActionButton::Num); button++) {
		poses[button].origin = origin;
		poses[button].direction = direction;
		ApplyAimOffset(m_input.aimOffsets[button], poses[button].origin, poses[button].direction);
	}
	// 入力をそのまま渡す。どのボタンでどのスキルを使うか、押した・押している・離したの判断は SkillController がする。
	m_skillController.Update(castInput, poses);
}

void Character::ApplyAimOffset(const Vector3& offset, Vector3& origin, Vector3& direction) const
{
	if (!m_input.hasAim || offset.LengthSq() <= FLT_EPSILON) {
		return;
	}
	// 撃つ位置だけずらし、向きは照準の先の点へ向け直す（画面の真ん中に飛んでいくように）。
	const Vector3 aimPoint = origin + direction * AIM_POINT_DISTANCE;
	origin += offset;
	direction = aimPoint - origin;
	direction.Normalize();
}

void Character::MakeCastPose(Vector3& origin, Vector3& direction) const
{
	if (m_input.hasAim) {
		origin = m_input.aimOrigin;
		direction = m_input.aimDirection;
	}
	else {
		origin = m_mover.GetPosition();
		origin.y += m_castSettings.castHeight;
		direction = m_mover.GetForward();
		direction.y = 0.0f;
	}
	if (direction.LengthSq() <= FLT_EPSILON) {
		direction = Vector3::AxisZ;
	}
	direction.Normalize();
	// 自分のカプセルに当たらないよう、前に出してから撃つ。
	origin += direction * m_castSettings.castForward;
}

void Character::SyncTransform()
{
	m_transform.position = m_mover.GetPosition();
	m_transform.rotation = m_mover.GetRotation();
}
