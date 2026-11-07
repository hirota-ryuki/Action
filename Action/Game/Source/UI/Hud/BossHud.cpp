#include "stdafx.h"
#include "BossHud.h"

#include "Character.h"
#include "UISprite.h"

namespace
{
	// HPゲージ。画面上の中央に、横長に置く（Player のゲージは画面下なので重ならない）。
	constexpr float HP_WIDTH = 720.0f;
	constexpr float HP_HEIGHT = 12.0f;
	constexpr float HP_BORDER = 2.0f;
	constexpr float HP_TOP = 100.0f;  // 画面の上端から、ゲージの中央までの距離。
	// HPゲージの左端の中央。
	const Vector2 HP_POSITION = { -HP_WIDTH * 0.5f, static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - HP_TOP };
	const Vector4 HP_COLOR = { 0.9f, 0.28f, 0.22f, 1.0f };      // 赤。
	const Vector4 HP_BACK_COLOR = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 HP_BORDER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 HP_TRAIL_COLOR = { 1.0f, 0.92f, 0.8f, 0.8f };  // 少し暖かい白。

	// 名前は HPゲージの上に小さく。
	constexpr float NAME_SCALE = 0.55f;
	constexpr float NAME_GAP = 24.0f;  // ゲージの中央から、名前の中央までの距離。
	const Vector2 NAME_POSITION = { 0.0f, HP_POSITION.y + NAME_GAP };
	const Vector4 NAME_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NAME_SHADOW_OFFSET = 1.5f;

	constexpr float TRAIL_DELAY = 0.5f;  // 減ってから、遅れて減るバーが縮み始めるまでの秒数。
	constexpr float TRAIL_SPEED = 0.5f;  // 1秒にゲージ全体の 50% 縮む。
	constexpr float FADE_SECONDS = 0.3f; // フェードイン・アウトにかける秒数。
}

void BossHud::Init()
{
	Gauge::Settings hp;
	hp.width = HP_WIDTH;
	hp.height = HP_HEIGHT;
	hp.borderSize = HP_BORDER;
	hp.borderColor = HP_BORDER_COLOR;
	hp.backColor = HP_BACK_COLOR;
	hp.barColor = HP_COLOR;
	hp.barImagePath = UISprite::GAUGE_PATH;
	hp.useTrail = true;
	hp.trailColor = HP_TRAIL_COLOR;
	hp.trailDelay = TRAIL_DELAY;
	hp.trailSpeed = TRAIL_SPEED;
	m_hp.Init(hp);
	m_hp.SetPosition(HP_POSITION);

	m_name.SetPosition(NAME_POSITION);
	m_name.SetScale(NAME_SCALE);
	m_name.SetShadow(NAME_SHADOW_OFFSET, NAME_SHADOW_COLOR);

	SetAlpha(0.0f);
}

void BossHud::Show(const Character* boss, const wchar_t* name)
{
	m_boss = boss;
	m_name.SetText(name);
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}
	// 戦い始めに白いバーが減っていく演出にならないよう、今のHPにそろえる。
	m_hp.ResetTrail();
	m_targetAlpha = 1.0f;
}

void BossHud::Hide()
{
	m_boss = nullptr;
	m_targetAlpha = 0.0f;
}

void BossHud::Update(float deltaTime)
{
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}

	// 目標の透明度へ、一定の速さで近づける。
	const float step = deltaTime / FADE_SECONDS;
	if (m_alpha < m_targetAlpha) {
		SetAlpha(min(m_targetAlpha, m_alpha + step));
	}
	else if (m_alpha > m_targetAlpha) {
		SetAlpha(max(m_targetAlpha, m_alpha - step));
	}

	m_name.Update(deltaTime);
	m_hp.Update(deltaTime);
}

void BossHud::Draw(RenderContext& rc)
{
	m_name.Draw(rc);
	m_hp.Draw(rc);
}

void BossHud::SetAlpha(float alpha)
{
	m_alpha = alpha;
	// 名前とゲージに同じ透明度を入れる。0 のときは部品の側で描かない。
	m_name.SetAlpha(alpha);
	m_hp.SetAlpha(alpha);
}
