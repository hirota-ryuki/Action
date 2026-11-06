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
		// 歩きは原神のような落ち着いた速さにする（Walk アニメーションを約1.3倍で再生して足をそろえる）。
		settings.walkSpeed = 170.0f;
		settings.runSpeedMultiplier = 480.0f / 170.0f;  // 走る速さは 480 のまま変えない。
		settings.jumpSpeed = 400.0f;
		settings.gravity = 980.0f;
		settings.capsuleRadius = 20.0f;
		settings.capsuleHeight = 100.0f;
		return settings;
	}

	PlayerStateSettings MakeStateSettings()
	{
		PlayerStateSettings settings;
		settings.dashDistance = 390.0f;  // ダッシュスキルの総移動距離（はじめ 300、1.3倍にした）。
		// ダッシュのクリップ（原神の動画の 33.41〜33.94秒、30fps・17コマ）の区間に合わせる。
		//   しゃがむ f00〜f04（4コマ）、蹴って低く滑る f04〜f11（7コマ）、着地 f11〜f16（5コマ）。
		settings.dashSpeed = 390.0f / (7.0f / 30.0f); // 390 を 7コマ（約0.233秒）で進む（平均 約1671）。
		settings.dashEndSpeed = 480.0f;  // 終わりは走る速さ。蹴った瞬間は約2860 になる。
		settings.dashAnimSpeed = 1.0f;   // 動画と同じ速さで流す。
		settings.dashAnimBlendSeconds = 0.05f;
		settings.dashCrouchSeconds = 4.0f / 30.0f;
		settings.dashCrouchBlendSeconds = 0.1f;
		settings.dashCrouchSpeed = 200.0f;     // 入った勢いで少し進み、止まってから飛び出す。
		settings.dashKickStartTime = 4.0f / 30.0f;
		settings.dashLandSeconds = 5.0f / 30.0f;
		settings.dashExitBlendSeconds = 0.15f; // ダッシュの最後と走りの始めは同じ形なので、短くてよい。
		// ダッシュのあとの走り・歩きの始め。ダッシュの最後（原神の動画の 33.94秒）と同じ足の形のところ。
		// Run（原神の走りの1周期、30fps・15コマ）: 左足で蹴ったあと f05、右足で蹴ったあと f12。
		settings.dashLToRunStartTime = 5.0f / 30.0f;
		settings.dashRToRunStartTime = 12.0f / 30.0f;
		settings.dashLToWalkStartTime = 0.0f;
		settings.dashRToWalkStartTime = 17.0f / 30.0f;
		settings.dashLandIntoRun = true; // 走るなら、着地（5コマ）の間にダッシュから走りへ補間する。
		settings.dashCanChain = false;   // 着地まで終わるまで入力を受け付けない（連打で続けて出さない）。
		settings.dashEndWithStop = true; // ダッシュのあと入力がなければ、勢いで滑って止まる。
		settings.dashKeepRun = true;     // ダッシュのあとは、移動の入力を離すまで走り続ける（ちょこちょこ歩きに戻らない）。
		// 走りから止まる。StopL / StopR（原神の動画の 29.78〜30.89秒、30fps・34コマ）に合わせる。
		//   f00〜f04 前の足を着いて腰を落とす、f04〜f14 突っ張って低く滑る、f14〜f33 起き上がって立つ。
		settings.stopSeconds = 30.0f / 30.0f;      // 立ち上がったところ（f30）で Idle へ移る。
		settings.stopSlideSeconds = 14.0f / 30.0f; // 走る速さ（480）から 0.47秒で止まる。約112 滑る。
		settings.stopBlendSeconds = 0.1f;
		settings.stopExitBlendSeconds = 0.3f;
		settings.stopAfterRunSeconds = 0.2f;       // 左Shift を移動キーより 0.2秒まで先に離しても滑って止まる。
		// Run（15コマ、最後は最初と同じ）で、前の足が地面に着くところ。右足 f02、左足 f09。
		settings.runAnimSeconds = 14.0f / 30.0f;
		settings.stopLRunTime = 9.0f / 30.0f;
		settings.stopRRunTime = 2.0f / 30.0f;
		settings.moveThreshold = 0.1f;   // 移動入力がこれより小さければ立ち止まる。
		settings.animBlendSeconds = 0.2f;
		// Walk.fbx（Mixamo の Female Walk）の足が地面を送る速さ。UE で足のボーンを測った 115/秒 × モデルの大きさ 1.128。
		settings.walkAnimMoveSpeed = 130.0f;
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
		// Idle・Walk は Mixamo のモーションを UE で SK_Player にリターゲットしたもの。
		// Jump はまだ SK_Player 用が無いので、暫定で run を入れておく。
		// Run・DashL・DashR は、原神の動画から MediaPipe で関節の位置を読み取り、その角度で SK_Player のポーズを作ったもの
		// （tools/UnrealScripts/dash_roto.json）。Run は走りの1周期（34.25〜34.72秒）。
		const AnimClipDesc run = { "Assets/modelData/player/Animation/Run.fbx", true };
		desc.clips.resize(enPlayerAnimClip_Num);
		desc.clips[enPlayerAnimClip_Idle] = { "Assets/modelData/player/Animation/Idle.fbx", true };
		desc.clips[enPlayerAnimClip_Walk] = { "Assets/modelData/player/Animation/Walk.fbx", true };
		desc.clips[enPlayerAnimClip_Run] = run;
		desc.clips[enPlayerAnimClip_Jump] = run;
		// DashL・DashR は「しゃがむ → 蹴って低く滑る → 着地」の通し（33.41〜33.94秒、L・R は蹴る足）。
		desc.clips[enPlayerAnimClip_DashL] = { "Assets/modelData/player/Animation/DashL.fbx", false };
		desc.clips[enPlayerAnimClip_DashR] = { "Assets/modelData/player/Animation/DashR.fbx", false };
		// StopL・StopR は走りから止まる「前の足を突っ張って滑る → 起き上がる」の通し（29.78〜30.89秒、L・R は突っ張る足。
		// tools/UnrealScripts/stop_roto.json）。
		desc.clips[enPlayerAnimClip_StopL] = { "Assets/modelData/player/Animation/StopL.fbx", false };
		desc.clips[enPlayerAnimClip_StopR] = { "Assets/modelData/player/Animation/StopR.fbx", false };
		return desc;
	}

	MagicCaster::Settings MakeCasterSettings()
	{
		MagicCaster::Settings settings;
		settings.castHeight = 140.0f;   // 三人称時、足元からこの高さで撃つ（胸のあたり）。
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
