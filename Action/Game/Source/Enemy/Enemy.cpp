#include "stdafx.h"
#include "Enemy.h"
#include "DashSlashAttack.h"
#include "HeavyComboAttack.h"
#include "SlashWaveAttack.h"
#include "SpringBones.h"
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
	m_skillController.SetBody(body);
	m_skillController.Register<DashSlashAttack>(ActionButton::Action1);
	m_skillController.Register<HeavyComboAttack>(ActionButton::Action2);
	m_skillController.Register<SlashWaveAttack>(ActionButton::Action3);

	// 黒騎士は揺れ物・体の連動を使わない（垂れ布の揺れはモーションに焼き込んである）ので、
	// アニメーションの姿勢ができたあとの処理は、開発用の窓の「剣を持つ右腕」の調整だけに使う。
	m_model.GetModelRender().GetSkeleton().SetOnPostUpdateWorldMatrix([this](Skeleton& skeleton) {
		RotateDebugArm(skeleton);
	});
}

namespace
{
	// bone とその子孫を、center を中心に、axis（向き）のまわりに deg 度回す。
	void RotateBoneTreeAround(Bone& bone, const Vector3& center, Vector3 axis, float deg)
	{
		if (fabsf(deg) < 0.01f || axis.LengthSq() <= FLT_EPSILON) {
			return;
		}
		axis.Normalize();
		Quaternion rotation;
		rotation.SetRotationDeg(axis, deg);
		Matrix toOrigin, rotationMatrix, back;
		toOrigin.MakeTranslation(center * -1.0f);
		rotationMatrix.MakeRotationFromQuaternion(rotation);
		back.MakeTranslation(center);
		SpringBones::RotateBoneTree(bone, toOrigin * rotationMatrix * back);
	}
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
	RotateBoneTreeAround(*upper, shoulder, shoulder - position(leftNo), m_debugShoulderDeg);

	// ② ひじから下 内・外: 上腕を、その向き（肩 → ひじ）を軸にひねる。ひじは動かず、曲げた前腕が体の内・外へ振れる。
	RotateBoneTreeAround(*upper, shoulder, position(lowerNo) - shoulder, m_debugForearmDeg);

	// ③ ひじから下 前後: ひじを中心に、①と同じ体の横の向きのまわりに前腕から先を回す（＋で前）。
	RotateBoneTreeAround(*skeleton.GetBone(lowerNo), position(lowerNo), shoulder - position(leftNo), m_debugElbowDeg);

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
	RotateBoneTreeAround(*hand, wrist, bendAxis, m_debugWristBendDeg);
	RotateBoneTreeAround(*hand, wrist, sideAxis, m_debugWristSideDeg);
	RotateBoneTreeAround(*hand, wrist, forearm, m_debugWristTwistDeg);
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

void Enemy::UpdateAction(float deltaTime)
{
	// 倒れ終わって消えたら、体を隠して当たり判定を外し、何もしない。
	if (m_damage.isVanished) {
		SetRenderBody(false);
		m_hitTarget.SetEnabled(false);
		return;
	}
	UpdateDebugWindow();
	if (m_debugMotion >= 0) {
		// 窓で選んだクリップを、その場で繰り返す。AI の入力は捨てる（攻撃もしない）。
		m_input = CharacterInput();
		if (m_playingDebugMotion != m_debugMotion) {
			m_playingDebugMotion = m_debugMotion;
			m_model.PlayAnimation(m_debugMotion, 0.2f);
		}
		m_model.GetModelRender().SetAnimationSpeed(m_debugPlaySpeed);
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
	ImGui::RadioButton(u8"AI（ふだんどおり）", &m_debugMotion, -1);
	ImGui::RadioButton(u8"Walk（歩く）", &m_debugMotion, enEnemyAnimClip_Walk);
	ImGui::RadioButton(u8"Run（走る）", &m_debugMotion, enEnemyAnimClip_Run);
	// 歩く速さに合わせた Walk の速さ（足が滑らない速さ）にすぐ戻せるように、ボタンも置く。
	ImGui::SliderFloat(u8"再生の速さ", &m_debugPlaySpeed, 0.05f, 1.5f, "%.2f");
	if (ImGui::Button(u8"等速（1.0）")) {
		m_debugPlaySpeed = 1.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"ゲームの Walk の速さ")) {
		m_debugPlaySpeed = WALK_SPEED / WALK_ANIM_FOOT_SPEED;
	}
	// Run を流している間だけ効く。決めた角度は、モーションの設定（black_knight_anims.json の Run の adjust）に書き写す。
	ImGui::SeparatorText(u8"剣を持つ右腕（Run のときだけ）");
	ImGui::SliderFloat(u8"右肩 前後（＋で前）", &m_debugShoulderDeg, -60.0f, 60.0f, "%.0f");
	ImGui::SliderFloat(u8"ひじから下 内・外（－で内）", &m_debugForearmDeg, -90.0f, 90.0f, "%.0f");
	ImGui::SliderFloat(u8"ひじから下 前後（＋で前）", &m_debugElbowDeg, -90.0f, 90.0f, "%.0f");
	ImGui::SliderFloat(u8"手首 曲げ", &m_debugWristBendDeg, -90.0f, 90.0f, "%.0f");
	ImGui::SliderFloat(u8"手首 横", &m_debugWristSideDeg, -90.0f, 90.0f, "%.0f");
	ImGui::SliderFloat(u8"手首 ひねり", &m_debugWristTwistDeg, -90.0f, 90.0f, "%.0f");
	if (ImGui::Button(u8"右腕を戻す")) {
		m_debugShoulderDeg = 0.0f;
		m_debugForearmDeg = 0.0f;
		m_debugElbowDeg = 0.0f;
		m_debugWristBendDeg = 0.0f;
		m_debugWristSideDeg = 0.0f;
		m_debugWristTwistDeg = 0.0f;
	}
	ImGui::End();
}

