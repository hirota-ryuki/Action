#pragma once

#include "State.h"
#include "CharacterInput.h"
#include "StatusEffects.h"

// ステートを持ち、毎フレーム遷移と更新を行う（Player・Enemy で共通）。
// 行動できない（凍結中など）ときは、今のステートによらず Frozen へ移す。
//   Id: ステートの種類の enum class。Frozen（行動できないとき）と Num（種類の数）を持つこと。
//   Context: ステートが触ってよいもの。statusEffects（const StatusEffects*）と previousId（Id）を持つこと。
// 派生クラスはコンストラクタで、Id の全部のステートを Register<ステートのクラス>() で登録する。
template <class Id, class Context>
class StateMachine
{
public:
	using StateType = State<Id, Context>;

	virtual ~StateMachine() = default;

	// context の部品は初期化済みのものを渡す。firstId から始める（行動できないときは Frozen）。
	void Init(const Context& context, Id firstId)
	{
		for (const auto& state : m_states) {
			K2_ASSERT(state != nullptr, "登録していないステートがあります。派生クラスのコンストラクタで Register してください。");
		}
		m_context = context;
		m_firstId = firstId;
		m_current = nullptr;
		Reset();
	}

	// 遷移を決めてから動く。押したフレームに動き始め、1フレームに移るのは1回だけ。
	void Update(const CharacterInput& input, float deltaTime)
	{
		if (m_current == nullptr) {
			return;
		}
		OnBeforeUpdate(m_context, input);
		const Id next = CanAct() ? m_current->CheckTransition(m_context, input) : Id::Frozen;
		if (next != m_current->GetId()) {
			ChangeState(next, input);
		}
		m_current->Update(m_context, input, deltaTime);
	}

	// 最初のステートに戻す（リスポーン時など）。行動できないときは Frozen にする。
	void Reset()
	{
		ChangeState(CanAct() ? m_firstId : Id::Frozen, CharacterInput());
	}

	Id GetCurrentId() const
	{
		return m_current != nullptr ? m_current->GetId() : m_firstId;
	}
	// デバッグ表示用。
	const char* GetCurrentName() const
	{
		return m_current != nullptr ? m_current->GetName() : "None";
	}

protected:
	// ステートを登録する。番号はステート自身の GetId() から取る（番号と中身を取り違えないように）。
	template <class T>
	void Register()
	{
		auto state = std::make_unique<T>();
		const int index = static_cast<int>(state->GetId());
		K2_ASSERT(m_states[index] == nullptr, "同じ番号のステートを2回登録しています。");
		m_states[index] = std::move(state);
	}

	const Context& GetContext() const
	{
		return m_context;
	}

	// 遷移を決める前に毎フレーム呼ばれる。キャラクターだけの前処理を書く。
	virtual void OnBeforeUpdate(Context& context, const CharacterInput& input)
	{
	}

private:
	void ChangeState(Id next, const CharacterInput& input)
	{
		if (m_current != nullptr) {
			m_current->Exit(m_context);
			m_context.previousId = m_current->GetId();
		}
		m_current = m_states[static_cast<int>(next)].get();
		m_current->Enter(m_context, input);
	}

	bool CanAct() const
	{
		return m_context.statusEffects == nullptr || m_context.statusEffects->CanAct();
	}

	Context m_context;
	// ステートは最初に全部作り、番号で引く。切り替えのたびに new しない。
	std::unique_ptr<StateType> m_states[static_cast<int>(Id::Num)];
	StateType* m_current = nullptr;
	Id m_firstId = Id::Frozen;
};
