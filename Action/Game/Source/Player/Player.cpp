#include "stdafx.h"
#include "Player.h"
#include "PlayerConfig.h"
#include "IceMagic.h"
#include "FireMagic.h"
#include <imgui/imgui.h>

namespace
{
	// ジャンプ中の度合いが 0 から 1（1 から 0）まで変わるのにかかる時間（秒）。アニメーションの切り替えの補間と同じくらいにする。
	constexpr float JUMP_WEIGHT_FADE_SECONDS = 0.2f;
}

Player::Player()
	: Character(Faction::Player)
{
}

CharacterConfig Player::MakeConfig() const
{
	return PlayerConfig::MakeCharacterConfig();
}

void Player::OnStart()
{
	m_fallRespawn.Init(GetPosition(), PlayerConfig::RESPAWN_HEIGHT);
	m_sword.Init(m_model.GetModelRender(), PlayerConfig::MakeSwordDesc());

	PlayerStateContext stateContext;
	stateContext.mover = &m_mover;
	stateContext.animator = &m_animator;
	stateContext.statusEffects = &m_statusEffects;
	stateContext.settings = PlayerConfig::MakeStateSettings();
	m_animator.Init(&m_model.GetModelRender(), stateContext.settings.animBlendSeconds);
	m_stateMachine.Init(stateContext);  // Idle から始める。

	m_magicCaster.AddMagic(std::make_unique<IceMagic>());
	m_magicCaster.SetSubMagic(std::make_unique<FireMagic>());  // 右クリックで撃つ。
}

void Player::FillInputContext(InputSourceContext& context) const
{
	context.isFirstPerson = !IsRenderBody();
}

void Player::UpdateAction(float deltaTime)
{
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	m_stateMachine.Update(m_input, deltaTime);
	UpdateJumpWeight(deltaTime);
	if (m_fallRespawn.IsFallen(m_mover.GetPosition())) {
		Respawn();
	}
}

void Player::OnLateUpdate(float deltaTime)
{
	m_sword.Update();
	UpdateDebugUI();
}

void Player::UpdateJumpWeight(float deltaTime)
{
	const PlayerStateId id = m_stateMachine.GetCurrentId();
	const float step = deltaTime / JUMP_WEIGHT_FADE_SECONDS;
	auto approach = [step](float current, float target) {
		return target > current ? min(target, current + step) : max(target, current - step);
	};
	m_jumpWeight = approach(m_jumpWeight, (id == PlayerStateId::Air || id == PlayerStateId::Land) ? 1.0f : 0.0f);
	m_airWeight = approach(m_airWeight, id == PlayerStateId::Air ? 1.0f : 0.0f);
	SetBodyLinkJumpWeight(m_jumpWeight, m_airWeight);
}

void Player::UpdateDebugUI()
{
	// F1 で開発用の画面を出しているときだけ（Release では ImGui を起動しないので、見ないで呼ぶと落ちる）。
	if (!DebugUI::IsVisible()) {
		return;
	}
	// 変えた値は次のフレームから効く。保存はしないので、決まったら player.json の bodyLink に書き写す。
	BodyLinkSettings& bodyLink = GetBodyLinkSettings();
	ImGui::Begin(u8"プレイヤーの体");
	ImGui::SliderFloat(u8"腕を振る量", &bodyLink.armSwingScale, 0.0f, 2.0f, u8"%.2f 倍");
	ImGui::TextUnformatted(u8"走っているときの上腕の前後の振り。1 でアニメーションのまま。");
	ImGui::Separator();
	ImGui::SliderFloat(u8"腕を前まで出す角度", &bodyLink.armFrontMaxDeg, 0.0f, 120.0f, u8"%.0f°");
	ImGui::SliderFloat(u8"腕を後ろまで引く角度", &bodyLink.armBackMaxDeg, 0.0f, 120.0f, u8"%.0f°");
	ImGui::TextUnformatted(u8"上腕の角度（真下を 0°）。0 なら抑えない。");
	ImGui::Separator();
	ImGui::SliderFloat(u8"肩の関節を前後に", &bodyLink.shoulderJointOffsetDeg, -60.0f, 60.0f, u8"%.0f°");
	ImGui::SliderFloat(u8"ひじの関節を前後に", &bodyLink.elbowJointOffsetDeg, -60.0f, 60.0f, u8"%.0f°");
	ImGui::TextUnformatted(u8"走っているときだけ回す角度（立ち止まり・歩きはそのまま）。正なら前、負なら後ろ。");
	ImGui::SliderFloat(u8"上腕を横に広げる", &bodyLink.upperArmSpreadDeg, -30.0f, 60.0f, u8"%.0f°");
	ImGui::TextUnformatted(u8"走っているときだけ。正なら体の外へ、負なら内へ。");
	ImGui::Separator();
	ImGui::SliderFloat(u8"膝を左右に（股関節から）", &bodyLink.jumpLegSpreadDeg, -30.0f, 30.0f, u8"%.0f°");
	ImGui::TextUnformatted(u8"跳ぶ・空中・着地のときだけ。股関節を中心に、正なら膝を外へ、負なら内へ（数字の1つ目＝横が変わる）。");
	ImGui::SliderFloat(u8"太ももを前後に回す（空中）", &bodyLink.jumpKneeForwardDeg, -45.0f, 90.0f, u8"%.0f°");
	ImGui::SliderFloat(u8"太ももを上げる（空中）", &bodyLink.jumpHipFlexScale, 0.0f, 2.0f, u8"%.2f 倍");
	ImGui::SliderFloat(u8"膝を曲げる（空中）", &bodyLink.jumpKneeBendScale, 0.0f, 2.0f, u8"%.2f 倍");
	ImGui::TextUnformatted(u8"跳ぶ・空中のときだけ（着地は変えない）。0° / 1 倍でアニメーションのまま。");
	{
		// 調べる用の表示。
		const BodyLink::DebugInfo& d = GetBodyLinkDebugInfo();
		const char* names[2] = { u8"左", u8"右" };
		ImGui::Text(u8"顔の向き（前から右回り）: %.1f°", d.faceYawDeg);
		for (int i = 0; i < 2; i++) {
			ImGui::Text(u8"%s膝（横,上,前） 前: %.1f, %.1f, %.1f", names[i], d.kneeBefore[i].x, d.kneeBefore[i].y, d.kneeBefore[i].z);
			ImGui::Text(u8"    膝を前に出した後: %.1f, %.1f, %.1f", d.kneeAfterForward[i].x, d.kneeAfterForward[i].y, d.kneeAfterForward[i].z);
			ImGui::Text(u8"    股を開いた後: %.1f, %.1f, %.1f", d.kneeAfter[i].x, d.kneeAfter[i].y, d.kneeAfter[i].z);
		}
	}
	ImGui::Separator();
	ImGui::Checkbox(u8"左右をそろえる", &bodyLink.symmetrize);
	ImGui::TextUnformatted(u8"走っているときだけ。腕・足の形を左右でそろえ、頭を前に向ける。");
	ImGui::Separator();
	if (ImGui::Button(u8"初期値に戻す")) {
		ResetBodyLinkSettings();
	}
	ImGui::SameLine();
	ImGui::TextUnformatted(u8"player.json の値に戻す");
	ImGui::End();
}

void Player::OnRender(RenderContext& rc)
{
	m_sword.Draw(rc);
}

void Player::Respawn()
{
	m_mover.SetPosition(m_fallRespawn.GetRespawnPosition());
	m_mover.Stop();
	m_stateMachine.Reset();
}
