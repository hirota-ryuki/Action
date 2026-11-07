#pragma once

// キャラクターの、遊んでいる間に変わる値（HP など）。
// 速さ・時間などの調整用の定数はここに入れない（各キャラクターの Config に書く）。
// 凍結・燃焼のような一時的な状態は StatusEffects にある。
// キャラクターごとの値は、これを継承して足す（例: PlayerStatus）。
class Status
{
public:
	virtual ~Status() = default;

	// HP を最大にして始める。
	void Init(float maxHp)
	{
		m_maxHp = maxHp;
		m_hp = maxHp;
	}

	float GetHp() const
	{
		return m_hp;
	}
	float GetMaxHp() const
	{
		return m_maxHp;
	}
	// 最大HPに対する今のHPの割合（0〜1）。HPゲージに使う。最大HPが 0 以下なら 0。
	float GetHpRate() const
	{
		if (m_maxHp <= 0.0f) {
			return 0.0f;
		}
		return min(1.0f, max(0.0f, m_hp / m_maxHp));
	}
	// HP を減らす。0 より下にはならない。
	void Damage(float amount)
	{
		m_hp = max(0.0f, m_hp - amount);
	}
	// HP を増やす。最大より上にはならない。
	void Heal(float amount)
	{
		m_hp = min(m_maxHp, m_hp + amount);
	}
	bool IsHpEmpty() const
	{
		return m_hp <= 0.0f;
	}

private:
	float m_hp = 100.0f;
	float m_maxHp = 100.0f;
};
