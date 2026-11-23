#include "stdafx.h"
#include "Enemy.h"
#include "DashSlashAttack.h"
#include "HeavyComboAttack.h"
#include "SlashWaveAttack.h"
#include "PoseLayers.h"
#include <imgui/imgui.h>

namespace
{
	// 行列の位置（平行移動の成分）。
	Vector3 GetTranslation(const Matrix& m)
	{
		return Vector3(m.m[3][0], m.m[3][1], m.m[3][2]);
	}

	// 黒騎士の大きさ。兜のあご〜頭頂（モデルで 0.175）が、Player の顔のあご〜頭頂（0.217 × 1.128）と同じになる 1.128 × 0.217 ÷ 0.175。
	constexpr float MODEL_SCALE = 1.4f;
	// 当たり判定とカプセルは、前の大きさ（1.3）で体に合わせた値を、同じ割合で大きくする。
	constexpr float SIZE_RATIO = MODEL_SCALE / 1.3f;
	constexpr float HIT_RADIUS = 60.0f * SIZE_RATIO;         // 魔法に当たる球の半径。黒騎士の胴（肩当てまで）を覆う。
	constexpr float HIT_CENTER_HEIGHT = 110.0f * SIZE_RATIO; // 足元から当たり判定の中心までの高さ（黒騎士の胸の下あたり）。
	constexpr float WALK_SPEED = 140.0f;        // 余裕たっぷりにゆっくり歩く速さ。
	constexpr float WALK_ANIM_FOOT_SPEED = 229.0f;  // Walk を等速で再生したときに、足が地面を送る速さ（Blender で測った）。
	// 黒騎士の動き全体の速さ。アニメーションが崩れていないかを確かめるときは下げる（例: 0.5）。
	constexpr float PLAY_SPEED = 1.0f;
	constexpr float MAX_HP = 100.0f;
	// 1回当たったときに減る HP（仮）。4回で倒れる。魔法ごとの威力ができたら、そちらを使う。
	constexpr float HIT_DAMAGE = 25.0f;
	// 倒れ終わってから消えるまでの秒数。
	constexpr float VANISH_DELAY = 2.0f;
	// 攻撃1の構えで、マントを煽る強さ（マントの burstBack・burstLift に掛ける倍率）。
	constexpr float CHARGE_BURST_STRENGTH = 0.5f;
	// 攻撃1が構えで止める時刻（秒）。DashSlashAttack と同じ（Attack1 の 5.5コマ目）。窓で流すときに使う。
	constexpr float DASH_SLASH_CHARGE_AT = 5.5f / 30.0f;

	CharacterMoveSettings MakeMoveSettings()
	{
		CharacterMoveSettings settings;
		settings.walkSpeed = WALK_SPEED;      // Walk は WALK_SPEED / WALK_ANIM_FOOT_SPEED 倍でゆっくり再生する（EnemyMoveState）。
		settings.runSpeedMultiplier = 2.75f;  // 走るときは 385。Run の足が地面を送る速さ（約384）に合わせる。
		settings.gravity = 980.0f;
		settings.capsuleRadius = 30.0f * SIZE_RATIO;   // 黒騎士は肩当てが広いので、プレイヤー（20）より太くする。
		settings.capsuleHeight = 140.0f * SIZE_RATIO;
		return settings;
	}

	BlackKnightInputSource::Settings MakeAiSettings()
	{
		BlackKnightInputSource::Settings settings;
		settings.searchRange = 1800.0f;    // この距離以内に入ると追いかけ始める（ここまでが「遠い」）。
		settings.nearRange = 220.0f;       // この距離以内が「近い」。剣を振る（剣の長さ＋踏み込み）。
		settings.midRange = 1200.0f;       // この距離以内が「中くらい」。
		settings.stopDistance = 150.0f;    // この距離まで近づいたら止まる（剣が届くところで止まる）。
		settings.decideInterval = 2.0f;    // 歩いている間、走るかを決め直す間隔（秒）。
		settings.runChance = 0.5f;         // 中くらい・遠いときに走る確率。残りは攻撃（中くらいなら攻撃1、遠いなら攻撃3）。
		settings.attackInterval = 5.4f;    // 近いときに攻撃2をする間隔（秒）。攻撃2は 4.3秒かかる（PLAY_SPEED が 1 のとき）。
		settings.firstAttackDelay = 0.6f;  // 相手を見つけてから最初に振れるまでの秒数。
		return settings;
	}

	// マントとフードの揺れ（骨の名前が J_Sec_Cloth・J_Sec_Hood で始まるもの）。体の動きから遅れて付いてきて、止まると揺り返す。
	// 重い布なのでプレイヤーの髪（3.5・0.6）よりゆっくり、大きく揺らす。
	SpringBonesSettings MakeSpringBonesSettings()
	{
		SpringBonesSettings settings;
		settings.isEnabled = true;
		settings.legRadius = 0.0f;  // マントは脚に当てない（脚のカプセルはモデルの大きさに合わせていないため）。
		SpringBoneGroupSettings cloth;
		cloth.match = L"Cloth";
		// 体へ引き戻す力を弱くし、後ろへ流される力（動くほど翻る）と、止まっていても吹くゆるい風を足して、バサバサ翻らせる。
		cloth.frequency = 0.9f;
		cloth.dampingRatio = 0.25f;
		cloth.maxAngleDeg = 65.0f;
		cloth.windDrag = 2.0f;
		// 踏み込み（急に速くなる）の出だしで、後ろ・上へ煽って、足に張り付かず、バサッと浮かせる。
		cloth.burstBack = 300.0f;
		cloth.burstLift = 250.0f;
		cloth.burstMaxAngleDeg = 110.0f;  // 煽った直後だけ、水平（90°）より上まで舞い上がれるようにする。
		cloth.breeze = 0.0f;  // 止まっているときは、風でひらひらさせない（動いたときだけ翻る）。
		settings.groups.push_back(cloth);
		// 背中の上（肩〜腰。ClothU）は肩・腕の重みと混ざっているので、大きく動かすと肩がぐじゃぐじゃになる。小さく・硬くして、少しだけ流す
		// （ClothU は Cloth より長いので、こちらが選ばれる）。
		SpringBoneGroupSettings backCloth;
		backCloth.match = L"ClothU";
		backCloth.frequency = 1.8f;
		backCloth.dampingRatio = 0.6f;
		backCloth.maxAngleDeg = 15.0f;
		backCloth.windDrag = 0.5f;
		// 構えで、肩のあたりから上へ舞い上がる（骨2本ぶん重なるので、1本ぶんは小さく）。煽った直後だけ上限を広げる。
		backCloth.burstBack = 150.0f;
		backCloth.burstLift = 300.0f;
		backCloth.burstMaxAngleDeg = 50.0f;
		settings.groups.push_back(backCloth);
		SpringBoneGroupSettings hood;
		hood.match = L"Hood";
		hood.frequency = 2.5f;
		hood.dampingRatio = 0.4f;
		hood.maxAngleDeg = 25.0f;
		settings.groups.push_back(hood);
		return settings;
	}

	CharacterModelDesc MakeModelDesc()
	{
		CharacterModelDesc desc;
		// 黒騎士。体は Meshy で作り、骨格を SK_Player に合わせる（tools/BlenderScripts/make_black_knight_meshy.py で作る）。
		// アニメーションは既存のモーションを黒騎士の骨格へ移したもの（tools/BlenderScripts/retarget_black_knight.py で作る）。
		desc.modelPath = "Assets/modelData/enemy/BlackKnight/BlackKnight.fbx";
		desc.upAxis = enModelUpAxisZ;
		desc.scale = MODEL_SCALE;
		desc.clips.resize(enEnemyAnimClip_Num);
		desc.clips[enEnemyAnimClip_Idle] = { "Assets/modelData/enemy/BlackKnight/Animation/Idle.fbx", true };
		desc.clips[enEnemyAnimClip_Walk] = { "Assets/modelData/enemy/BlackKnight/Animation/Walk.fbx", true };
		desc.clips[enEnemyAnimClip_Run] = { "Assets/modelData/enemy/BlackKnight/Animation/Run.fbx", true };
		desc.clips[enEnemyAnimClip_Attack1] = { "Assets/modelData/enemy/BlackKnight/Animation/Attack1.fbx", false };
		desc.clips[enEnemyAnimClip_Attack2] = { "Assets/modelData/enemy/BlackKnight/Animation/Attack2.fbx", false };
		desc.clips[enEnemyAnimClip_Attack3] = { "Assets/modelData/enemy/BlackKnight/Animation/Attack3.fbx", false };
		desc.clips[enEnemyAnimClip_Hit] = { "Assets/modelData/enemy/BlackKnight/Animation/Hit.fbx", false };
		desc.clips[enEnemyAnimClip_Dead] = { "Assets/modelData/enemy/BlackKnight/Animation/Dead.fbx", false };
		return desc;
	}

}

Enemy::Enemy()
	: Character(Faction::Enemy)
	, m_aiInputSource(MakeAiSettings())
{
}

CharacterConfig Enemy::MakeConfig() const
{
	CharacterConfig config;
	config.move = MakeMoveSettings();
	config.model = MakeModelDesc();
	config.hitRadius = HIT_RADIUS;
	config.hitCenterHeight = HIT_CENTER_HEIGHT;
	config.maxHp = MAX_HP;
	config.springBones = MakeSpringBonesSettings();
	return config;
}

void Enemy::OnStart()
{
	EnemyStateContext stateContext;
	stateContext.mover = &m_mover;
	stateContext.model = &m_model;
	stateContext.statusEffects = &m_statusEffects;
	stateContext.owner = this;
	stateContext.faction = GetFaction();
	stateContext.settings.walkAnimSpeed = WALK_SPEED / WALK_ANIM_FOOT_SPEED;
	stateContext.settings.playSpeed = PLAY_SPEED;
	stateContext.settings.vanishDelay = VANISH_DELAY;
	stateContext.settings.rootMoveScale = 100.0f * MODEL_SCALE;
	m_damage.status = &m_status;
	stateContext.damage = &m_damage;
	stateContext.skills = &m_skillController;
	// 魔法などが当たったら HP を減らし、ステートに知らせる（ひるむか・倒れるかはステートが決める）。
	m_hitTarget.SetOnHit([this](const HitInfo&) {
		if (m_status.IsHpEmpty()) {
			return;
		}
		m_status.Damage(HIT_DAMAGE);
		m_damage.isHitRequested = true;
	});
	m_stateMachine.Init(stateContext);  // Idle から始める（Idle のアニメーションを流す）。

	// 剣の攻撃は、体を動かすスキル（AttackBase）。体を渡してから、攻撃1〜3 を Action1〜Action3 に登録する。
	SkillBody body;
	body.model = &m_model;
	body.mover = &m_mover;
	body.timeScale = PLAY_SPEED;
	body.rootMoveScale = stateContext.settings.rootMoveScale;
	body.blendSeconds = stateContext.settings.animBlendSeconds;
	// 攻撃1が構え（Attack1 の 6コマ目あたり。左腰に剣を構えてかがむ）で止めて溜め始めた瞬間に、マントを後ろ・上へ舞い上がらせる。
	body.onChargeStart = [this]() {
		TriggerSpringBurst(m_mover.GetForward() * -1.0f, CHARGE_BURST_STRENGTH);
	};
	m_skillController.SetBody(body);
	m_skillController.Register<DashSlashAttack>(ActionButton::Action1);
	m_skillController.Register<HeavyComboAttack>(ActionButton::Action2);
	m_skillController.Register<SlashWaveAttack>(ActionButton::Action3);
}

void Enemy::OnAddPoseLayers(PoseLayers& layers)
{
	// 黒騎士は揺れ物・体の連動を使わない（垂れ布の揺れはモーションに焼き込んである）ので、
	// 層は、開発用の窓の「剣を持つ右腕」の調整だけ。
	layers.Add([this](Skeleton& skeleton) { RotateDebugArm(skeleton); });
	layers.Add([this](Skeleton& skeleton) { RotateDebugCape(skeleton); });
}

void Enemy::RotateDebugCape(Skeleton& skeleton)
{
	if (m_playingDebugMotion != enEnemyAnimClip_Attack1 && m_playingDebugMotion != enEnemyAnimClip_Attack2) {
		return;
	}
	const int rightNo = skeleton.FindBoneID(L"J_Bip_R_UpperArm");
	const int leftNo = skeleton.FindBoneID(L"J_Bip_L_UpperArm");
	if (rightNo < 0 || leftNo < 0) {
		return;
	}
	auto position = [&](int boneNo) { return GetTranslation(skeleton.GetBone(boneNo)->GetWorldMatrix()); };
	// 体の横の向き（左肩 → 右肩）。この軸まわりに＋で回すと、垂れた骨の先が前へ振れる（右腕の「前後」と同じ）ので、
	// 後ろ・上へ上げるときは逆に回す。
	const Vector3 side = position(rightNo) - position(leftNo);
	// 背中の上の骨を上げる。先側の骨の位置は、根元側を回したあとのもの。
	auto lift = [&](const wchar_t* chain, const float deg[3]) {
		for (int k = 0; k < 3; k++) {
			wchar_t name[32];
			swprintf_s(name, L"%s_%d", chain, k + 1);
			const int boneNo = skeleton.FindBoneID(name);
			if (boneNo < 0 || deg[k] == 0.0f) {
				continue;
			}
			PoseLayers::RotateBoneTreeAround(*skeleton.GetBone(boneNo), position(boneNo), side, -deg[k]);
		}
	};
	lift(L"J_Sec_ClothUB", m_debugCapeUpperDeg);
	lift(L"J_Sec_ClothUL", m_debugCapeUpperDeg);
	lift(L"J_Sec_ClothUR", m_debugCapeUpperDeg);
	// 腰から下（Hips の子）は、背中の上（UpperChest の子）と別に動くので、背の曲がりのぶんだけ境目がずれて折れる。
	// まず、腰から下の根元を背中の上の先の位置へ移し、向きも背中の上の先の向きにそろえて、真っ直ぐにつなぐ（この形から、下のスライダーで曲げる）。
	const wchar_t* weldPairs[2][3] = {
		{ L"J_Sec_ClothUL_2", L"J_Sec_ClothUL_3", L"J_Sec_ClothL" },
		{ L"J_Sec_ClothUR_2", L"J_Sec_ClothUR_3", L"J_Sec_ClothR" },
	};
	for (const auto& pair : weldPairs) {
		wchar_t lowerName1[32], lowerName2[32];
		swprintf_s(lowerName1, L"%s_1", pair[2]);
		swprintf_s(lowerName2, L"%s_2", pair[2]);
		const int upper2 = skeleton.FindBoneID(pair[0]);
		const int upper3 = skeleton.FindBoneID(pair[1]);
		const int lower1 = skeleton.FindBoneID(lowerName1);
		const int lower2 = skeleton.FindBoneID(lowerName2);
		if (upper2 < 0 || upper3 < 0 || lower1 < 0 || lower2 < 0) {
			continue;
		}
		Bone& lowerBone = *skeleton.GetBone(lower1);
		const Vector3 tail = position(upper3) * 2.0f - position(upper2);  // 背中の上の最後の骨の先（骨 3 本が同じ間隔で並ぶ）。
		Matrix move;
		move.MakeTranslation(tail - position(lower1));
		PoseLayers::RotateBoneTree(lowerBone, move);
		Vector3 upperDirection = position(upper3) - position(upper2);
		Vector3 lowerDirection = position(lower2) - position(lower1);
		upperDirection.Normalize();
		lowerDirection.Normalize();
		const float cosine = max(-1.0f, min(1.0f, lowerDirection.Dot(upperDirection)));
		Vector3 axis;
		axis.Cross(lowerDirection, upperDirection);
		if (axis.LengthSq() > 1.0e-8f) {
			axis.Normalize();
			PoseLayers::RotateBoneTreeAround(lowerBone, tail, axis, Math::RadToDeg(acosf(cosine)));
		}
	}
	lift(L"J_Sec_ClothL", m_debugCapeLowerDeg);
	lift(L"J_Sec_ClothR", m_debugCapeLowerDeg);
}

void Enemy::RotateDebugArm(Skeleton& skeleton)
{
	if (m_playingDebugMotion != enEnemyAnimClip_Run) {
		return;
	}
	const int upperNo = skeleton.FindBoneID(L"J_Bip_R_UpperArm");
	const int leftNo = skeleton.FindBoneID(L"J_Bip_L_UpperArm");
	const int lowerNo = skeleton.FindBoneID(L"J_Bip_R_LowerArm");
	const int handNo = skeleton.FindBoneID(L"J_Bip_R_Hand");
	if (upperNo < 0 || leftNo < 0 || lowerNo < 0 || handNo < 0) {
		return;
	}
	Bone* upper = skeleton.GetBone(upperNo);
	Bone* hand = skeleton.GetBone(handNo);
	auto position = [&](int boneNo) { return GetTranslation(skeleton.GetBone(boneNo)->GetWorldMatrix()); };

	// ① 右肩 前後: 右肩の関節を中心に、左右の肩を結ぶ向き（体の横の向き）のまわりに回す。
	const Vector3 shoulder = position(upperNo);
	PoseLayers::RotateBoneTreeAround(*upper, shoulder, shoulder - position(leftNo), m_debugShoulderDeg);

	// ② ひじから下 内・外: 上腕を、その向き（肩 → ひじ）を軸にひねる。ひじは動かず、曲げた前腕が体の内・外へ振れる。
	PoseLayers::RotateBoneTreeAround(*upper, shoulder, position(lowerNo) - shoulder, m_debugForearmDeg);

	// ③ ひじから下 前後: ひじを中心に、①と同じ体の横の向きのまわりに前腕から先を回す（＋で前）。
	PoseLayers::RotateBoneTreeAround(*skeleton.GetBone(lowerNo), position(lowerNo), shoulder - position(leftNo), m_debugElbowDeg);

	// ④ 手首: 手首を中心に、前腕から見た3つの向きで回す（剣も一緒に回る）。
	const Vector3 elbow = position(lowerNo);
	const Vector3 wrist = position(handNo);
	Vector3 forearm = wrist - elbow;               // ひねりの軸（ひじ → 手首）。
	Vector3 upperDir = elbow - shoulder;
	Vector3 bendAxis = Cross(upperDir, forearm);   // ひじの曲がる軸。手首の曲げもこの軸まわり。
	if (bendAxis.LengthSq() <= FLT_EPSILON) {
		bendAxis = Cross(Vector3::AxisY, forearm);
	}
	const Vector3 sideAxis = Cross(forearm, bendAxis);  // 手首を横（親指側・小指側）へ倒す軸。
	PoseLayers::RotateBoneTreeAround(*hand, wrist, bendAxis, m_debugWristBendDeg);
	PoseLayers::RotateBoneTreeAround(*hand, wrist, sideAxis, m_debugWristSideDeg);
	PoseLayers::RotateBoneTreeAround(*hand, wrist, forearm, m_debugWristTwistDeg);
}

void Enemy::FilterSkillInput(CharacterInput& input) const
{
	// ステートは SkillController より先に更新される。ボタンを押したフレームに攻撃のステートへ入っていなければ
	// （当たってひるんだ、倒れている、など）、攻撃を作らない。
	if (m_stateMachine.GetCurrentId() == EnemyStateId::Attack) {
		return;
	}
	for (ButtonState& action : input.actions) {
		action = ButtonState();
	}
}

void Enemy::Revive()
{
	m_status.Init(MAX_HP);
	m_damage.isVanished = false;
	m_damage.isHitRequested = false;
	SetRenderBody(true);
	m_hitTarget.SetEnabled(true);
	m_playingDebugMotion = -1;
	m_stateMachine.Reset();  // Dead から Idle に戻す。
}

void Enemy::UpdateAction(float deltaTime)
{
	// 消えている間も、窓の「復活」ボタンは押せるようにする（先に窓を出す）。
	UpdateDebugWindow();
	// 倒れ終わって消えたら、体を隠して当たり判定を外し、何もしない。
	if (m_damage.isVanished) {
		SetRenderBody(false);
		m_hitTarget.SetEnabled(false);
		return;
	}
	if (m_debugMotion >= 0) {
		// 窓で選んだクリップを、その場で繰り返す。AI の入力は捨てる（攻撃もしない）。
		m_input = CharacterInput();
		if (m_playingDebugMotion != m_debugMotion || m_debugRestart) {
			if (m_debugRestart) {
				// 同じクリップを流しているときは PlayAnimation が何もしないので、いったん別のクリップに替えてから流し直す（補間なしで最初から）。
				m_model.PlayAnimation(m_debugMotion == enEnemyAnimClip_Idle ? enEnemyAnimClip_Walk : enEnemyAnimClip_Idle, 0.0f);
				m_model.PlayAnimation(m_debugMotion, 0.0f);
			}
			else {
				m_model.PlayAnimation(m_debugMotion, 0.2f);
			}
			m_playingDebugMotion = m_debugMotion;
			m_debugRestart = false;
			m_debugPlayTime = 0.0f;
			m_isDebugBurstDone = false;
		}
		const float debugSpeed = m_debugPaused ? 0.0f : m_debugPlaySpeed;
		m_model.GetModelRender().SetAnimationSpeed(debugSpeed);
		// 窓で Attack1 を流しているときも、ゲーム中と同じ構えのコマで、マントを舞い上がらせる。
		m_debugPlayTime += deltaTime * debugSpeed;
		if (m_debugMotion == enEnemyAnimClip_Attack1 && !m_isDebugBurstDone && m_debugPlayTime >= DASH_SLASH_CHARGE_AT) {
			m_isDebugBurstDone = true;
			TriggerSpringBurst(m_mover.GetForward() * -1.0f, CHARGE_BURST_STRENGTH);
		}
		return;
	}
	if (m_playingDebugMotion >= 0) {
		// 窓で「AI」に戻したら、Idle からやり直す（流していたクリップのまま止まらないように）。
		m_playingDebugMotion = -1;
		m_stateMachine.Reset();
	}
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	// PLAY_SPEED を掛けて、移動・攻撃の時間（突進・斬撃を出すとき）もアニメーションと同じだけ遅くする。
	m_stateMachine.Update(m_input, deltaTime * PLAY_SPEED);
}

void Enemy::UpdateDebugWindow()
{
	// Release ビルドでは ImGui を起動しないので、出していないときは何も呼ばない。
	if (!DebugUI::IsVisible()) {
		return;
	}
	ImGui::Begin(u8"黒騎士のモーション");
	if (ImGui::Button(u8"復活（HP を全回復して、その場で立たせる）")) {
		Revive();
	}
	// ボタンを押すと、そのクリップを最初から流す（同じボタンをもう一度押すと、もう一度最初から。一時停止も解く）。流しているクリップは緑にする。
	auto playButton = [this](const char* label, int clip) {
		const bool isPlaying = m_debugMotion == clip;
		if (isPlaying) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.25f, 1.0f));
		}
		if (ImGui::Button(label)) {
			m_debugMotion = clip;
			m_debugRestart = true;
			m_debugPaused = false;
		}
		if (isPlaying) {
			ImGui::PopStyleColor();
		}
	};
	playButton(u8"AI（ふだんどおり）", -1);
	playButton(u8"Idle（立ち）", enEnemyAnimClip_Idle);
	ImGui::SameLine();
	playButton(u8"Walk（歩く）", enEnemyAnimClip_Walk);
	ImGui::SameLine();
	playButton(u8"Run（走る）", enEnemyAnimClip_Run);
	playButton(u8"Attack1（居合い・踏み込み）", enEnemyAnimClip_Attack1);
	playButton(u8"Attack2（重い連続斬り）", enEnemyAnimClip_Attack2);
	playButton(u8"Attack3（連続斬り）", enEnemyAnimClip_Attack3);
	playButton(u8"Hit（やられ）", enEnemyAnimClip_Hit);
	ImGui::SameLine();
	playButton(u8"Dead（倒れる）", enEnemyAnimClip_Dead);
	ImGui::Checkbox(u8"一時停止", &m_debugPaused);
	// 歩く速さに合わせた Walk の速さ（足が滑らない速さ）にすぐ戻せるように、ボタンも置く。
	ImGui::SliderFloat(u8"再生の速さ", &m_debugPlaySpeed, 0.05f, 2.0f, "%.2f");
	if (ImGui::Button(u8"等速（1.0）")) {
		m_debugPlaySpeed = 1.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"ゲームの Walk の速さ")) {
		m_debugPlaySpeed = WALK_SPEED / WALK_ANIM_FOOT_SPEED;
	}
	// Attack1・Attack2 を流している間だけ効く。構えのコマで「一時停止」して、角度を決める（＋で後ろ・上へ）。
	// マントが引っ張られて見えるとき、揺れ物（SpringBones）のせいかを切り分ける。チェックすると揺れを止めて、アニメーションの姿勢のままにする。
	if (ImGui::Checkbox(u8"揺れ物を止める（切り分け用）", &m_isDebugSpringSuspended)) {
		SetSpringBonesSuspended(m_isDebugSpringSuspended);
	}
	ImGui::SeparatorText(u8"マントを上げる（Attack1・Attack2 のときだけ）");
	ImGui::SliderFloat(u8"背中の上 1（肩）", &m_debugCapeUpperDeg[0], -30.0f, 120.0f, "%.0f");
	ImGui::SliderFloat(u8"背中の上 2", &m_debugCapeUpperDeg[1], -30.0f, 120.0f, "%.0f");
	ImGui::SliderFloat(u8"背中の上 3（腰の手前）", &m_debugCapeUpperDeg[2], -30.0f, 120.0f, "%.0f");
	ImGui::SliderFloat(u8"腰から下 1（腰）", &m_debugCapeLowerDeg[0], -30.0f, 120.0f, "%.0f");
	ImGui::SliderFloat(u8"腰から下 2", &m_debugCapeLowerDeg[1], -30.0f, 120.0f, "%.0f");
	ImGui::SliderFloat(u8"腰から下 3", &m_debugCapeLowerDeg[2], -30.0f, 120.0f, "%.0f");
	if (ImGui::Button(u8"マントを戻す")) {
		for (int k = 0; k < 3; k++) {
			m_debugCapeUpperDeg[k] = 0.0f;
			m_debugCapeLowerDeg[k] = 0.0f;
		}
	}
	ImGui::End();
}

