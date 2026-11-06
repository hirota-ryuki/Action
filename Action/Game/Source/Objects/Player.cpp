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
		settings.dashDistance = 300.0f;  // ダッシュスキルの総移動距離。
		settings.dashSpeed = 1000.0f;    // ダッシュの平均の速さ（走りの約2倍）。300 を 0.3秒で進む。
		settings.dashEndSpeed = 480.0f;  // 終わりは走る速さ。蹴った瞬間は 1520 になる。
		settings.dashAnimSpeed = 1.0f;   // 0.3秒の一蹴りを、ダッシュの0.3秒でそのまま流す（速く流すとバタバタして見える）。
		settings.dashAnimBlendSeconds = 0.05f;
		settings.dashCrouchSeconds = 8.0f / 30.0f; // しゃがみ込むクリップ（沈む4コマ＋底で踏ん張る4コマ）の長さ。
		settings.dashCrouchBlendSeconds = 0.12f;
		settings.dashCrouchSpeed = 200.0f;     // 入った勢いで少し進み、踏ん張って止まってから飛び出す。
		settings.dashKickBlendSeconds = 0.03f;
		settings.dashChainKickStartTime = 2.0f / 30.0f; // 一蹴りの f02 から（脚が伸び切ったところ）。
		settings.dashLandSeconds = 0.2f;       // 着地のクリップ（7コマ）の長さ。前の足の膝を曲げて受け止める。
		settings.dashLandBlendSeconds = 0.08f;
		settings.dashExitBlendSeconds = 0.25f; // 着地から走りへ。足の位置は合わせてあるので、ふつうより少し長めで足りる。
		// 着地のあとの走り・歩きの始め。Run（30fps・19コマ）と Walk（30fps・36コマ）で足の位置を測って決めた。
		// 着地の最後と同じ形のところ。左足で蹴ったあと: Run f10、Walk f00。右足で蹴ったあと: Run f01、Walk f17。
		settings.dashLToRunStartTime = 10.0f / 30.0f;
		settings.dashRToRunStartTime = 1.0f / 30.0f;
		settings.dashLToWalkStartTime = 0.0f;
		settings.dashRToWalkStartTime = 17.0f / 30.0f;
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
		const AnimClipDesc run = { "Assets/modelData/player/Animation/ThirdPersonRunPlayer_Run.fbx", true };
		desc.clips.resize(enPlayerAnimClip_Num);
		desc.clips[enPlayerAnimClip_Idle] = { "Assets/modelData/player/Animation/Idle.fbx", true };
		desc.clips[enPlayerAnimClip_Walk] = { "Assets/modelData/player/Animation/Walk.fbx", true };
		desc.clips[enPlayerAnimClip_Run] = run;
		desc.clips[enPlayerAnimClip_Jump] = run;
		// DashL・DashR は、Run の体全体を前へ倒し、片足で蹴って宙へ飛び出す1歩だけにしたもの（L・R は蹴る足）。
		// tools/UnrealScripts/dash_lean.json で作る。ループしないので、終わると飛び出したポーズ（後ろ足を伸ばし、片腕を前へ）で止まる。
		desc.clips[enPlayerAnimClip_DashL] = { "Assets/modelData/player/Animation/DashL.fbx", false };
		desc.clips[enPlayerAnimClip_DashR] = { "Assets/modelData/player/Animation/DashR.fbx", false };
		// 着地。Run を少し前へ倒し、前の足が着地して踏むまでの7コマで、腰を落として膝を曲げたもの。
		desc.clips[enPlayerAnimClip_DashLandL] = { "Assets/modelData/player/Animation/DashLandL.fbx", false };
		desc.clips[enPlayerAnimClip_DashLandR] = { "Assets/modelData/player/Animation/DashLandR.fbx", false };
		// 蹴る前のしゃがみ込み。蹴る足を前に出し、腰を約30cm落として膝を深く曲げ、反対の腕を前へ出す。
		// 底でさらに 6cm 沈んで踏ん張り、そのポーズで止まる。一蹴り（DashL・DashR）はこのポーズから始まる。
		desc.clips[enPlayerAnimClip_DashCrouchL] = { "Assets/modelData/player/Animation/DashCrouchL.fbx", false };
		desc.clips[enPlayerAnimClip_DashCrouchR] = { "Assets/modelData/player/Animation/DashCrouchR.fbx", false };
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
