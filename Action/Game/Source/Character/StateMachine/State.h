#pragma once

struct CharacterInput;

// ステートの基底クラス（Player・Enemy で共通）。Id と Context はキャラクターごとに決める。
//   Id: ステートの種類の enum class
//   Context: ステートが触ってよいもの（移動・モデルなど）。ステートは持ち主のクラスを直接は知らない。
// StateMachine が毎フレーム次の順に呼ぶ。
//   1. CheckTransition: 次にどのステートへ移るかを決める
//   2. 移るなら、今のステートの Exit → 次のステートの Enter
//   3. Update: そのステートの動き
// ステートは自分の中だけで動き方と遷移を決める。ほかのステートの中身は知らない。
// ステートは最初に1つずつ作って使い回すので、中のデータは Enter で必ず初期化する。
template <class Id, class Context>
class State
{
public:
	virtual ~State() = default;

	virtual Id GetId() const = 0;
	// デバッグ表示用の名前。
	virtual const char* GetName() const = 0;

	// このステートに入ったとき。アニメーションの切り替えなど。
	virtual void Enter(Context& context, const CharacterInput& input)
	{
	}
	// 次のステートを返す。移らないなら GetId() を返す。
	// 凍結（行動できない）への遷移は、どのステートからでも StateMachine が行うので、ここでは見なくてよい。
	virtual Id CheckTransition(const Context& context, const CharacterInput& input) const = 0;
	// 1フレームの動き。CharacterMover の Move 系（Update を含む）の関数を1回だけ呼ぶこと（重力はそこでかかる）。
	virtual void Update(Context& context, const CharacterInput& input, float deltaTime) = 0;
	// このステートから出るとき。
	virtual void Exit(Context& context)
	{
	}
};
