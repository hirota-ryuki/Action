#pragma once

#include "Faction.h"
#include "MagicBase.h"

struct CharacterInput;
struct ButtonState;

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
	// サブの発射ボタン（subFire）で撃つ魔法。切り替えの対象にはならず、いつでも撃てる。
	void SetSubMagic(std::unique_ptr<MagicBase> magic);

	// canAct が false（凍結中など）の間は撃てない。押している途中なら中断する。
	// 照準の指定がないときは、position の castHeight の高さから forward の向き（水平）へ撃つ。
	void Update(const CharacterInput& input, const Vector3& position, const Vector3& forward, float deltaTime, bool canAct);

	MagicBase* GetCurrentMagic() const;
	const MagicBase* GetSubMagic() const
	{
		return m_subMagic.get();
	}
	void SelectMagic(int index);

private:
	MagicCastContext MakeContext(const CharacterInput& input, const Vector3& position, const Vector3& forward, float deltaTime) const;
	// メインの魔法に渡す情報。input.mainAimOffset があれば、撃つ位置をずらす。
	static MagicCastContext MakeMainContext(const CharacterInput& input, const MagicCastContext& context);
	// ボタンの状態に応じて魔法の OnPress / OnHold / OnRelease を呼ぶ。
	static void UpdateButton(MagicBase& magic, const ButtonState& button, const MagicCastContext& context, bool& isHolding);
	void CancelHolding();
	void CancelSubHolding();

	std::vector<std::unique_ptr<MagicBase>> m_magics;
	std::unique_ptr<MagicBase> m_subMagic;
	IGameObject* m_owner = nullptr;
	Settings m_settings;
	Faction m_faction = Faction::Neutral;
	int m_currentIndex = 0;
	bool m_isHolding = false;     // 発射ボタンを押して魔法を使っている最中か。
	bool m_isSubHolding = false;  // サブの発射ボタンを押してサブの魔法を使っている最中か。
};
