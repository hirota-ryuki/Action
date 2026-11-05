#pragma once

#include "Faction.h"
#include "MagicBase.h"

struct CharacterInput;

// キャラクターが持つ魔法の一覧と、今選んでいる魔法を管理する。
// 入力の発射ボタン・切り替えに応じて、魔法の OnPress / OnHold / OnRelease を呼ぶ。
class MagicCaster
{
public:
	struct Settings
	{
		float castHeight = 70.0f;   // 照準の指定がないとき、足元からこの高さで撃つ。
		float castForward = 60.0f;  // 自分のカプセルに当たらないよう、撃つ位置を前に出す距離。
	};

	void Init(IGameObject* owner, Faction faction, const Settings& settings);
	void SetFaction(Faction faction)
	{
		m_faction = faction;
	}
	void AddMagic(std::unique_ptr<MagicBase> magic);

	// canAct が false（凍結中など）の間は撃てない。押している途中なら中断する。
	// 照準の指定がないときは、position の castHeight の高さから forward の向き（水平）へ撃つ。
	void Update(const CharacterInput& input, const Vector3& position, const Vector3& forward, float deltaTime, bool canAct);

	MagicBase* GetCurrentMagic() const;
	void SelectMagic(int index);

private:
	MagicCastContext MakeContext(const CharacterInput& input, const Vector3& position, const Vector3& forward, float deltaTime) const;
	void CancelHolding();

	std::vector<std::unique_ptr<MagicBase>> m_magics;
	IGameObject* m_owner = nullptr;
	Settings m_settings;
	Faction m_faction = Faction::Neutral;
	int m_currentIndex = 0;
	bool m_isHolding = false;  // 発射ボタンを押して魔法を使っている最中か。
};
