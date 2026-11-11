#pragma once

#include "Status.h"

// プレイヤーの、遊んでいる間に変わる値。Status（HP）に、持っているポーションの数を足したもの。
// スタミナなど Player だけの値は、使うときにここへ足す。
// 落ちて戻ったとき（Player::Respawn）も触らないので、ポーションの数はそのまま残る。
class PlayerStatus : public Status
{
public:
	int GetPotionCount() const
	{
		return m_potionCount;
	}
	void SetPotionCount(int count)
	{
		m_potionCount = max(0, count);
	}
	// 拾ったときに増やす。上限はない。
	void AddPotion(int count)
	{
		m_potionCount += max(0, count);
	}
	// 1つ減らす。持っていなければ何もせず false。
	bool UsePotion()
	{
		if (m_potionCount <= 0) {
			return false;
		}
		--m_potionCount;
		return true;
	}

private:
	int m_potionCount = 0;
};
