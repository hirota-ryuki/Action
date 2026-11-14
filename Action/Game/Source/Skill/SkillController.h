#pragma once

#include <functional>
#include "Faction.h"
#include "CharacterInput.h"
#include "SkillBase.h"

// キャラクターが使うスキルを、入力のボタン（ActionButton）ごとに登録し、入力を見て使う。
// キーやマウスは知らない（入力元が CharacterInput にボタンの状態を入れる）。
// 同時に使えるスキルは1つ。溜めている間は、ほかのボタンを受け付けない。
class SkillController
{
public:
	void Init(IGameObject* owner, Faction faction);
	void SetFaction(Faction faction)
	{
		m_faction = faction;
	}

	// button を押したら、T のスキルを使う。クールダウンは T::COOLDOWN（秒）。
	template <class T>
	void Register(ActionButton button)
	{
		Entry& entry = m_entries[static_cast<int>(button)];
		entry.create = [] { return NewGO<T>(1, SkillBase::OBJECT_NAME); };
		entry.cooldown = T::COOLDOWN;
	}

	// 毎フレーム、入力と撃つ位置・向きを渡す。登録したボタンが押された瞬間にスキルを作り、そのあとの入力を渡す。
	void Update(const CharacterInput& input, const Vector3& origin, const Vector3& direction);
	// 溜めているスキルを、使わずに消す（凍ったときなど）。
	void CancelAll();

private:
	struct Entry
	{
		std::function<SkillBase*()> create;  // スキルを1つ作る。空なら登録なし。
		float cooldown = 0.0f;               // 手を離してから、次に使えるまでの秒数。
		float lastUseTime = 0.0f;            // 前に手を離した時刻（GameClock）。
		bool hasUsed = false;                // 一度でも使ったか。
	};

	bool IsReady(const Entry& entry) const;
	// 使っている最中のスキルが手を離した。クールダウンを始める。
	void FinishActive();

	Entry m_entries[static_cast<int>(ActionButton::Num)];
	SkillBase* m_active = nullptr;  // 使っている最中（溜めている）のスキル。
	int m_activeButton = -1;        // m_active を使ったボタン。
	IGameObject* m_owner = nullptr;
	Faction m_faction = Faction::Neutral;
};
