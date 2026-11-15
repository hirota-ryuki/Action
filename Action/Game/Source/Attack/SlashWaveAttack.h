#pragma once

#include "AttackBase.h"

// 黒騎士の攻撃3。連続斬り（KinemIQ の slash combo）。剣が体の前を通る5回の振りで、斬撃（SlashMagic）を相手へ飛ばす。
// 斬撃は、振るコマが来たときに1つずつ作る。向きはそのときの撃つ向き（相手の胸のあたり）。
class SlashWaveAttack : public AttackBase
{
protected:
	const AttackData& GetData() const override;
	void OnSwing(float time) override;

private:
	void FireSlash(float rollDeg) const;

	int m_nextSlash = 0;   // 次に飛ばす斬撃の番号。
};
