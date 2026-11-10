#include "stdafx.h"
#include "Player.h"
#include "PlayerConfig.h"
#include "IceMagic.h"
#include "FireMagic.h"
#include <imgui/imgui.h>

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
	if (m_fallRespawn.IsFallen(m_mover.GetPosition())) {
		Respawn();
	}
}

void Player::OnLateUpdate(float deltaTime)
{
	m_sword.Update();
	UpdateDebugUI();
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
