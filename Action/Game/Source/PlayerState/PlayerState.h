#pragma once

class CharacterMover;
class StatusEffects;
struct CharacterInput;

// プレイヤーのステートの種類。
enum class PlayerStateId
{
	Idle,    // 立ち止まっている
	Walk,    // 歩いている
	Run,     // 走っている
	Air,     // 空中にいる（跳んだ・足場から落ちた）
	Dash,    // ダッシュ中（決まった距離を一気に進む）
	Frozen,  // 凍結などで行動できない
	Num,
};

// プレイヤーのアニメーションクリップの番号。
enum EnPlayerAnimClip
{
	enPlayerAnimClip_Idle,
	enPlayerAnimClip_Walk,
	enPlayerAnimClip_Run,
	enPlayerAnimClip_Jump,
	enPlayerAnimClip_Dash,
	enPlayerAnimClip_Num,
};

// ステートの調整用の数値。歩く・走る・跳ぶ速さは CharacterMoveSettings にある。
struct PlayerStateSettings
{
	float dashDistance = 300.0f;      // ダッシュの総移動距離。0ならダッシュできない。
	float dashSpeed = 1500.0f;        // ダッシュ中の移動速度。
	float dashAnimSpeed = 1.5f;       // ダッシュのアニメーションの再生速度（倍）。
	float dashAnimBlendSeconds = 0.05f; // ダッシュに入るときのアニメーションの補間時間。短いほど一気に前へ出る。
	float moveThreshold = 0.1f;       // 移動入力の長さがこれより小さければ立ち止まっているとみなす。
	float animBlendSeconds = 0.2f;    // アニメーションを切り替えるときの補間時間。
	float walkAnimMoveSpeed = 0.0f;   // 歩くアニメーションを等速で再生したとき、足が地面を送る速さ。0なら等速で再生する。
};

// ステートが触ってよいもの。ステートは Player クラスを直接は知らない。
struct PlayerStateContext
{
	CharacterMover* mover = nullptr;
	ModelRender* modelRender = nullptr;
	const StatusEffects* statusEffects = nullptr;
	PlayerStateSettings settings;

	// アニメーションを切り替えて、speed 倍の速さで再生する。
	void PlayAnimation(int animClip, float speed = 1.0f) const
	{
		modelRender->PlayAnimation(animClip, settings.animBlendSeconds);
		modelRender->SetAnimationSpeed(speed);
	}
};

// プレイヤーのステートの基底クラス。
// PlayerStateMachine が毎フレーム次の順に呼ぶ。
//   1. CheckTransition: 次にどのステートへ移るかを決める
//   2. 移るなら、今のステートの Exit → 次のステートの Enter
//   3. Update: そのステートの動き
// ステートは自分の中だけで動き方と遷移を決める。ほかのステートの中身は知らない。
class PlayerState
{
public:
	virtual ~PlayerState() = default;

	virtual PlayerStateId GetId() const = 0;
	// デバッグ表示用の名前。
	virtual const char* GetName() const = 0;

	// このステートに入ったとき。アニメーションの切り替えなど。
	virtual void Enter(PlayerStateContext& context, const CharacterInput& input)
	{
	}
	// 次のステートを返す。移らないなら GetId() を返す。
	// 凍結（行動できない）への遷移は、どのステートからでも PlayerStateMachine が行うので、ここでは見なくてよい。
	virtual PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const = 0;
	// 1フレームの動き。CharacterMover の Move 系の関数を1回だけ呼ぶこと（重力はそこでかかる）。
	virtual void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) = 0;
	// このステートから出るとき。
	virtual void Exit(PlayerStateContext& context)
	{
	}
};
