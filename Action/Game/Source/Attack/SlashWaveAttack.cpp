#include "stdafx.h"
#include "SlashWaveAttack.h"
#include "EnemyState.h"
#include "SlashMagic.h"

namespace
{
	const AttackData DATA = { enEnemyAnimClip_Attack3 };

	// 斬撃を出す時間（秒）。コマは retarget_black_knight.py --sword の剣先の通り道から読んだ（30コマで1秒）。
	// 27〜33コマ目の振りは体の横〜後ろを通るので出さない。
	//   12 右上→左下、49.5 右下→左上（斬り上げ）、62 右上→左下、78 右→左（横に払う）、90 上→下（振り下ろし）
	constexpr int SLASH_NUM = 5;
	constexpr float SLASH_TIMES[SLASH_NUM] = { 12.0f / 30.0f, 49.5f / 30.0f, 62.0f / 30.0f, 78.0f / 30.0f, 90.0f / 30.0f };
	// 斬撃を進む向きのまわりに回す角度。黒騎士から見て、- で左上→右下、+ で右上→左下。
	constexpr float SLASH_ROLL_DEG[SLASH_NUM] = { 45.0f, -45.0f, 45.0f, 0.0f, 90.0f };

	constexpr float SLASH_HEIGHT = 110.0f;   // 足元から、斬撃を出す高さ（黒騎士の胸のあたり）。
	constexpr float SLASH_FORWARD = 80.0f;   // 体の前へ出して、自分のカプセルに当たらないようにする。
}

const AttackData& SlashWaveAttack::GetData() const
{
	return DATA;
}

void SlashWaveAttack::OnSwing(float time)
{
	// 斬撃を飛ばすコマを過ぎたら、1つずつ出す。
	while (m_nextSlash < SLASH_NUM && time >= SLASH_TIMES[m_nextSlash]) {
		FireSlash(SLASH_ROLL_DEG[m_nextSlash]);
		m_nextSlash++;
	}
}

void SlashWaveAttack::FireSlash(float rollDeg) const
{
	Vector3 origin = GetBodyPosition() + GetBodyForward() * SLASH_FORWARD;
	origin.y += SLASH_HEIGHT;

	// 向きは、そのときの撃つ向き（AI が狙う相手の胸のあたり。狙いがなければ正面）。
	SlashMagic* slash = NewGO<SlashMagic>(SlashMagic::PRIORITY, SkillBase::OBJECT_NAME);
	slash->SetRoll(rollDeg);
	slash->Init(origin, GetDirection(), GetOwner(), GetFaction());
}
