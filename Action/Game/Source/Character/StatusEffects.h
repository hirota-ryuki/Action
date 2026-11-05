#pragma once

// キャラクターにかかる状態異常（今は凍結のみ）を管理する。
// 凍結中はモデルを水色にしてアニメーションを止める。移動を止めるのは持ち主側で CanAct() を見て行う。
class StatusEffects
{
public:
	// 見た目を変えるモデル。nullptr なら見た目は変えない。
	void Init(ModelRender* modelRender);
	void Update(float deltaTime);

	// 一定時間凍らせる。凍結中に再度呼ぶと、残り時間の長い方で上書きする。
	void Freeze(float seconds);

	bool IsFrozen() const
	{
		return m_freezeTimer > 0.0f;
	}
	// 移動・攻撃などの行動ができるか。
	bool CanAct() const
	{
		return !IsFrozen();
	}

private:
	void SetFrozenLook(float frozenRate);

	ModelRender* m_modelRender = nullptr;
	float m_freezeTimer = 0.0f;  // 凍結の残り秒数。0より大きい間は凍結中。
};
