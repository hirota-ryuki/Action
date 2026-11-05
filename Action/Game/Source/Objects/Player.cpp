#include "stdafx.h"
#include "Player.h"
#include "IceMagic.h"
#include "FireMagic.h"

namespace
{
	constexpr float kRespawnHeight = -1000.0f; // この高さを下回るとステージ外と判断してリスポーン。
	const Vector3 kSwordOffset = Vector3(5.0f, -3.0f, 10.0f);   // 右手ボーンローカル空間での剣の位置オフセット。
	const Vector3 kSwordScale = Vector3(1.35f, 1.35f, 1.35f);
	constexpr float kSwordRotOffsetX = 0.0f;
	constexpr float kSwordRotOffsetY = 0.0f;
	constexpr float kSwordRotOffsetZ = -90.0f; // 剣モデルのZ軸が手の向きと90度ずれているため補正。
	constexpr float kHitRadius = 45.0f;        // 弾に当たる球の半径。
	constexpr float kHitCenterHeight = 70.0f;  // 足元から当たり判定の中心までの高さ（胸のあたり）。

	CharacterMoveSettings MakeMoveSettings()
	{
		CharacterMoveSettings settings;
		settings.walkSpeed = 300.0f;
		settings.runSpeedMultiplier = 1.6f;  // 走る入力中の移動速度倍率。
		settings.jumpSpeed = 400.0f;
		settings.gravity = 980.0f;
		settings.capsuleRadius = 20.0f;
		settings.capsuleHeight = 100.0f;
		return settings;
	}

	PlayerStateSettings MakeStateSettings()
	{
		PlayerStateSettings settings;
		settings.rushDistance = 300.0f;  // ラッシュスキルの総移動距離。
		settings.rushSpeed = 1500.0f;    // ラッシュ中の移動速度（通常の5倍）。
		settings.rushPoseTime = 0.23f;   // ランアニメーションの疾走ポーズに合った時刻。
		settings.moveThreshold = 0.1f;   // 移動入力がこれより小さければ立ち止まる。
		settings.animBlendSeconds = 0.2f;
		return settings;
	}

	CharacterModelDesc MakeModelDesc()
	{
		CharacterModelDesc desc;
		// 3ds Max(Z-up)で作られているので Z を上にして起こす（Yのままだと仰向けに倒れる）。
		desc.modelPath = "Assets/modelData/player/Model/SK_Player.FBX";
		desc.upAxis = enModelUpAxisZ;
		// UnityChan(Enemy)の2倍の高さ(1.692)を狙った後、実機で大きすぎたのでその2/3にした。
		desc.scale = 1.128f;
		// 暫定: 今の SK_Player で使えるアニメーションは run だけ（旧Sapphiart用の idle/walk/jump はスケルトンが違う）。
		// クリップの数はステートが使う番号に合わせる必要があるので、全部の枠に run を入れておく。
		const AnimClipDesc run = { "Assets/modelData/player/Animation/ThirdPersonRunPlayer_Run.fbx", true };
		desc.clips.assign(enPlayerAnimClip_Num, run);
		return desc;
	}

	MagicCaster::Settings MakeCasterSettings()
	{
		MagicCaster::Settings settings;
		settings.castHeight = 70.0f;   // 三人称時、足元からこの高さで撃つ（胸のあたり）。
		settings.castForward = 60.0f;  // 自分のカプセル(半径20)に当たらないよう前に出して撃つ。
		return settings;
	}
}

Player::Player()
{

}

Player::~Player()
{

}

bool Player::Start()
{
	m_respawnPosition = m_mover.GetPosition();
	m_mover.Init(MakeMoveSettings());

	m_model.Init(MakeModelDesc());
	m_model.SetTransform(m_mover.GetPosition(), m_mover.GetRotation());
	m_model.Update();

	InitSword();

	m_statusEffects.Init(&m_model.GetModelRender());

	PlayerStateContext stateContext;
	stateContext.mover = &m_mover;
	stateContext.modelRender = &m_model.GetModelRender();
	stateContext.statusEffects = &m_statusEffects;
	stateContext.settings = MakeStateSettings();
	m_stateMachine.Init(stateContext);  // Idle から始める。

	m_hitTarget.Init(this, Faction::Player, kHitRadius, kHitCenterHeight);
	m_hitTarget.SetStatusEffects(&m_statusEffects);
	m_hitTarget.SetPosition(m_mover.GetPosition());

	m_magicCaster.Init(this, Faction::Player, MakeCasterSettings());
	m_magicCaster.AddMagic(std::make_unique<IceMagic>());
	m_magicCaster.SetSubMagic(std::make_unique<FireMagic>());  // 右クリックで撃つ。

	return true;
}

void Player::Update()
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();

	InputSourceContext context;
	context.self = this;
	context.faction = Faction::Player;
	context.position = m_mover.GetPosition();
	context.forward = m_mover.GetForward();
	context.deltaTime = deltaTime;
	context.isFirstPerson = !m_isRenderBody;
	m_inputSource->Update(context, m_input);

	m_statusEffects.Update(deltaTime);
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	m_stateMachine.Update(m_input, deltaTime);
	m_magicCaster.Update(m_input, m_mover.GetPosition(), m_mover.GetForward(), deltaTime, m_statusEffects.CanAct());

	RespawnIfNeeded();
	m_hitTarget.SetPosition(m_mover.GetPosition());

	m_model.SetTransform(m_mover.GetPosition(), m_mover.GetRotation());
	m_model.Update();

	UpdateSword();
}

void Player::Render(RenderContext& rc)
{
	if (!m_isRenderBody) {
		return;
	}

	m_model.Draw(rc);
	m_swordModelRender.Draw(rc);
}

void Player::InitSword()
{
	// 右手ボーンの名前はモデルのセットアップによって異なる。
	// 1つ目で見つからなければ別名で再検索する。
	m_rightHandBoneNo = m_model.GetModelRender().FindBoneID(L"Character1_RightHand");
	if (m_rightHandBoneNo < 0) {
		m_rightHandBoneNo = m_model.GetModelRender().FindBoneID(L"J_R_ForeArm_00_tw");
	}

	m_swordModelRender.Init("Assets/modelData/weapon/sowrd/sowrd.tkm");
	m_swordModelRender.SetShadowCasterFlag(true);
	UpdateSword();
}

void Player::UpdateSword()
{
	if (m_rightHandBoneNo < 0) {
		return;
	}

	Bone* rightHandBone = m_model.GetModelRender().GetBone(m_rightHandBoneNo);
	if (rightHandBone == nullptr) {
		return;
	}

	Vector3 swordPosition;
	Vector3 handScale;
	Quaternion swordRotation;
	rightHandBone->CalcWorldTRS(swordPosition, swordRotation, handScale);

	Vector3 offset = kSwordOffset;
	swordRotation.Apply(offset);
	swordPosition += offset;

	Quaternion offsetRotationX;
	offsetRotationX.SetRotationDegX(kSwordRotOffsetX);
	Quaternion offsetRotationY;
	offsetRotationY.SetRotationDegY(kSwordRotOffsetY);
	Quaternion offsetRotationZ;
	offsetRotationZ.SetRotationDegZ(kSwordRotOffsetZ);
	swordRotation = offsetRotationX * offsetRotationY * offsetRotationZ * swordRotation;

	m_swordModelRender.SetTRS(swordPosition, swordRotation, kSwordScale);
	m_swordModelRender.Update();
}

void Player::RespawnIfNeeded()
{
	if (m_mover.GetPosition().y < kRespawnHeight) {
		Respawn();
	}
}

void Player::Respawn()
{
	m_mover.SetPosition(m_respawnPosition);
	m_mover.Stop();
	m_stateMachine.Reset();
}
