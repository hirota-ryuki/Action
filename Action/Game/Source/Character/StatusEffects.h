#pragma once

// キャラクターにかかる状態異常（凍結・燃焼）を管理する。
// 凍結: モデルを水色にしてアニメーションを止める。移動を止めるのは持ち主側で CanAct() を見て行う。
// 燃焼: モデルを赤く揺らめかせる。行動は止めない（体力ができたら継続ダメージにつなげる）。
// 凍結と燃焼は打ち消し合う。燃やすと解凍され、凍らせると火が消える。
class StatusEffects
{
public:
	// 見た目を変えるモデル。nullptr なら見た目は変えない。
	void Init(ModelRender* modelRender);
	void Update(float deltaTime);

	// 一定時間凍らせる。凍結中に再度呼ぶと、残り時間の長い方で上書きする。燃焼中なら火を消す。
	void Freeze(float seconds);
	// 一定時間燃やす。燃焼中に再度呼ぶと、残り時間の長い方で上書きする。凍結中なら解凍する。
	void Burn(float seconds);

	bool IsFrozen() const
	{
		return m_freezeTimer > 0.0f;
	}
	bool IsBurning() const
	{
		return m_burnTimer > 0.0f;
	}
	// 移動・攻撃などの行動ができるか。
	bool CanAct() const
	{
		return !IsFrozen();
	}

private:
	void Unfreeze();
	void SetLook(float rate, const Vector4& mulColor, const Vector4& addColor);

	ModelRender* m_modelRender = nullptr;
	float m_freezeTimer = 0.0f;  // 凍結の残り秒数。0より大きい間は凍結中。
	float m_burnTimer = 0.0f;    // 燃焼の残り秒数。0より大きい間は燃焼中。
	float m_burnElapsed = 0.0f;  // 燃え始めてからの秒数。炎の揺らめきに使う。
};
