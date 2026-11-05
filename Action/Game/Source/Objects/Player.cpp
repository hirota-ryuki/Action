#include "stdafx.h"
#include "Player.h"
#include "tkFile/FbxRuntimeImporter.h"
#include "IceMagic.h"

namespace
{
	constexpr float kRushRunPoseTime = 0.23f;  // ラッシュ中に停止させるランアニメーションの再生位置（秒）。疾走ポーズに合った時刻。
	constexpr float kRespawnHeight = -1000.0f; // この高さを下回るとステージ外と判断してリスポーン。
	constexpr float kAnimMoveThreshold = 0.1f; // 移動入力がこれより小さければ待機アニメにする。
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
		settings.rushDistance = 300.0f;      // ラッシュスキルの総移動距離。
		settings.rushSpeed = 1500.0f;        // ラッシュ中の移動速度（通常の5倍）。
		settings.capsuleRadius = 20.0f;
		settings.capsuleHeight = 100.0f;
		return settings;
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

	// player.tkmは3ds Max(Z-up)で作成されているため、enModelUpAxisZを指定して起こす（Yのままだと仰向けに倒れた状態になる）。
	// 注意: SK_Player.FBXはランタイムFBXインポート(フェーズ2)でスキン・スケルトン付きで変換される。
	// idle/walk/jumpは旧Sapphiartモデル用のアニメーションクリップ(旧スケルトンのボーンインデックス参照)
	// のため、新しいSK_Playerのスケルトンとは互換性が無い。今はrunアニメーションのみ対応する。
	// ModelRender::Init()はAnimation::Init()経由で内部的にPlay(0)(enAnimClip_Idle)を
	// 呼び出すため、Init()を呼ぶ前に全クリップ(少なくともenAnimClip_Idle)をロードしておく必要がある
	// (他の呼び出し元(Enemy.cpp等)と同じ規約)。
	// SK_Playerのスケルトン(.tks)はInit()内のFBX→tkm変換で生成されるため、
	// アニメーションをリターゲットするにはInit()より前に一度tkm/tksの変換だけを済ませておく。
	nsK2EngineLow::FbxRuntimeImporter::ResolveToTkmFilePath("Assets/modelData/player/Model/SK_Player.FBX");

	// ThirdPersonRunPlayer_Run.fbxのアニメーションを、SK_Playerのスケルトンにボーン名ベースでリターゲットして.tkaへ変換。
	// 現状はrunクリップのみ対応のため、暫定的に全クリップ枠にrunアニメーションを読み込ませておく
	// (Animation::Init()のPlay(0)がクラッシュしないようにするため)。
	std::string runTkaPath = nsK2EngineLow::FbxRuntimeImporter::ResolveAnimationFbxToTka(
		"Assets/modelData/player/Animation/ThirdPersonRunPlayer_Run.fbx",
		"Assets/modelData/player/Model/SK_Player.tks");
	if (!runTkaPath.empty()) {
		for (int i = 0; i < enAnimClip_Num; i++) {
			m_animClips[i].Load(runTkaPath.c_str());
			m_animClips[i].SetLoopFlag(true);
		}
	}

	m_modelRender.Init("Assets/modelData/player/Model/SK_Player.FBX", m_animClips, enAnimClip_Num, enModelUpAxisZ);
	m_modelRender.SetShadowCasterFlag(true);
	m_modelRender.SetPosition(m_mover.GetPosition());
	m_modelRender.SetRotation(m_mover.GetRotation());
	// UnityChan(Enemy)の2倍の高さを狙って1.692に設定した後、実機で見て大きすぎたため
	// その2/3のサイズ(1.692 * 2/3 ≒ 1.128)に調整。
	m_modelRender.SetScale(Vector3(1.128f, 1.128f, 1.128f));

	if (!runTkaPath.empty()) {
		m_modelRender.PlayAnimation(enAnimClip_Run);
	}

	m_modelRender.Update();

	InitSword();

	m_statusEffects.Init(&m_modelRender);

	m_hitTarget.Init(this, Faction::Player, kHitRadius, kHitCenterHeight);
	m_hitTarget.SetStatusEffects(&m_statusEffects);
	m_hitTarget.SetPosition(m_mover.GetPosition());

	m_magicCaster.Init(this, Faction::Player, MakeCasterSettings());
	m_magicCaster.AddMagic(std::make_unique<IceMagic>());

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
	const bool canAct = m_statusEffects.CanAct();
	if (canAct) {
		m_mover.Update(m_input, deltaTime);
		UpdateAnimation();
	}
	else {
		// 凍結中は入力を無視し、重力で落ちるだけにする。
		m_mover.CancelRush();
		m_mover.Update(CharacterInput(), deltaTime);
	}
	m_magicCaster.Update(m_input, m_mover.GetPosition(), m_mover.GetForward(), deltaTime, canAct);

	RespawnIfNeeded();
	m_hitTarget.SetPosition(m_mover.GetPosition());

	m_modelRender.SetPosition(m_mover.GetPosition());
	m_modelRender.SetRotation(m_mover.GetRotation());
	m_modelRender.Update();

	UpdateSword();
}

void Player::Render(RenderContext& rc)
{
	if (!m_isRenderBody) {
		return;
	}

	m_modelRender.Draw(rc);
	m_swordModelRender.Draw(rc);
}

void Player::UpdateAnimation()
{
	if (m_mover.IsRushing()) {
		if (m_currentAnimNo != enAnimClip_Run || !m_isRushPoseActive) {
			m_modelRender.PlayAnimationAtTime(enAnimClip_Run, kRushRunPoseTime);
			m_modelRender.SetAnimationSpeed(0.0f);
			m_currentAnimNo = enAnimClip_Run;
			m_isRushPoseActive = true;
		}
		return;
	}

	if (m_isRushPoseActive) {
		m_modelRender.SetAnimationSpeed(1.0f);
		m_isRushPoseActive = false;
	}

	const float moveLength = m_input.moveDirection.Length();

	int newAnimNo;
	if (!m_mover.IsOnGround()) {
		newAnimNo = enAnimClip_Jump;
	}
	else if (moveLength < kAnimMoveThreshold) {
		newAnimNo = enAnimClip_Idle;
	}
	else if (m_input.isRun) {
		newAnimNo = enAnimClip_Run;
	}
	else {
		newAnimNo = enAnimClip_Walk;
	}

	// Avoid rewinding the clip if it is already playing
	if (newAnimNo != m_currentAnimNo) {
		m_modelRender.PlayAnimation(newAnimNo, 0.2f);
		m_currentAnimNo = newAnimNo;
	}
}

void Player::InitSword()
{
	// 右手ボーンの名前はモデルのセットアップによって異なる。
	// 1つ目で見つからなければ別名で再検索する。
	m_rightHandBoneNo = m_modelRender.FindBoneID(L"Character1_RightHand");
	if (m_rightHandBoneNo < 0) {
		m_rightHandBoneNo = m_modelRender.FindBoneID(L"J_R_ForeArm_00_tw");
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

	Bone* rightHandBone = m_modelRender.GetBone(m_rightHandBoneNo);
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
	m_isRushPoseActive = false;
}
