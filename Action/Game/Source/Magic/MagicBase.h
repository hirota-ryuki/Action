#pragma once

#include "Faction.h"

// 魔法を使うときに渡される情報。
struct MagicCastContext
{
	IGameObject* caster = nullptr;      // 使ったキャラクター。
	Faction faction = Faction::Neutral;
	Vector3 origin = Vector3::Zero;     // 撃ち出す位置。
	Vector3 direction = Vector3::AxisZ; // 撃ち出す向き（正規化済み）。
	float deltaTime = 0.0f;
};

// 魔法の基底クラス。「どう撃つか」を決める（押した瞬間に撃つ、溜めて離したら撃つ、押している間出し続ける、など）。
// 飛んでいく弾そのものは BulletBase の派生クラスが担当する。
// MagicCaster が発射ボタンの状態に応じて OnPress / OnHold / OnRelease を呼ぶ。
class MagicBase
{
public:
	virtual ~MagicBase() = default;

	// デバッグ表示やUI用の名前。
	virtual const char* GetName() const = 0;

	// 発射ボタンを押した瞬間。
	virtual void OnPress(const MagicCastContext& context) { (void)context; }
	// 発射ボタンを押している間、毎フレーム（押した瞬間のフレームも含む）。
	virtual void OnHold(const MagicCastContext& context) { (void)context; }
	// 発射ボタンを離した瞬間。
	virtual void OnRelease(const MagicCastContext& context) { (void)context; }
	// 押している途中で中断されたとき（凍結した、魔法を切り替えた、など）。溜めの破棄などに使う。
	virtual void OnCancel() {}

	// 毎フレーム呼ばれる。クールダウンを進める。
	virtual void Update(float deltaTime)
	{
		if (m_cooldown > 0.0f) {
			m_cooldown -= deltaTime;
		}
	}

	bool IsReady() const
	{
		return m_cooldown <= 0.0f;
	}

	// これまでに撃った回数。一人称の腕が、撃った瞬間に手を動かすのに使う（前のフレームと比べる）。
	int GetCastCount() const
	{
		return m_castCount;
	}
	// 溜めている最中か。溜め撃ちの魔法だけが true を返す。一人称の腕が、溜めている間は両手を上げるのに使う。
	virtual bool IsCharging() const
	{
		return false;
	}

protected:
	void StartCooldown(float seconds)
	{
		m_cooldown = seconds;
	}
	// 弾を撃ったときに呼ぶ。
	void NotifyCast()
	{
		++m_castCount;
	}

private:
	float m_cooldown = 0.0f;  // 再び使えるまでの残り秒数。
	int m_castCount = 0;
};
