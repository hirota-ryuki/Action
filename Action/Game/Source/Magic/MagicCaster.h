#pragma once

#include <functional>
#include "Faction.h"
#include "MagicBase.h"

struct ButtonState;

// キャラクターが使う魔法を枠（スロット）ごとに登録し、ボタンに合わせて撃つ。
// どのボタンで撃つか、どこから撃つかは知らない（キャラクターが Fire() に渡す）。
class MagicCaster
{
public:
	void Init(IGameObject* owner, Faction faction);
	void SetFaction(Faction faction)
	{
		m_faction = faction;
	}

	// slot の枠に、T の魔法を登録する。クールダウンは T::COOLDOWN（秒）。
	template <class T>
	void Register(int slot)
	{
		Slot& target = GetOrAddSlot(slot);
		target.create = [] { return NewGO<T>(1, MagicBase::OBJECT_NAME); };
		target.cooldown = T::COOLDOWN;
	}

	// 毎フレーム、ボタンの状態を渡す。押した瞬間に魔法を作り、溜め撃ちなら離すまで撃つ位置に合わせる。
	void Fire(int slot, const ButtonState& button, const Vector3& origin, const Vector3& direction);
	// 溜めている魔法を、飛ばさずに消す（凍ったときなど）。
	void CancelAll();

private:
	struct Slot
	{
		std::function<MagicBase*()> create;  // 魔法を1つ作る。
		float cooldown = 0.0f;               // 撃ってから次に撃てるまでの秒数。
		float lastFireTime = 0.0f;           // 前に撃った時刻（GameClock）。
		bool hasFired = false;               // 一度でも撃ったか。
		MagicBase* holding = nullptr;        // 溜めている（撃つ位置で待たせている）魔法。
	};

	Slot& GetOrAddSlot(int slot);
	bool IsReady(const Slot& slot) const;

	std::vector<Slot> m_slots;
	IGameObject* m_owner = nullptr;
	Faction m_faction = Faction::Neutral;
};
