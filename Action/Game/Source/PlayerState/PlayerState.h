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
	enPlayerAnimClip_DashL,  // ダッシュの一蹴り（左足で蹴る）
	enPlayerAnimClip_DashR,  // ダッシュの一蹴り（右足で蹴る）
	enPlayerAnimClip_DashLandL,  // 左足で蹴ったあとの着地（右足で、膝を曲げて受け止める）
	enPlayerAnimClip_DashLandR,  // 右足で蹴ったあとの着地（左足で受け止める）
	enPlayerAnimClip_DashCrouchL,  // 左足で蹴る前に、しゃがみ込む（左足を前に、膝を深く曲げる）
	enPlayerAnimClip_DashCrouchR,  // 右足で蹴る前に、しゃがみ込む
	enPlayerAnimClip_Num,
};

// ステートの調整用の数値。歩く・走る・跳ぶ速さは CharacterMoveSettings にある。
struct PlayerStateSettings
{
	float dashDistance = 300.0f;      // ダッシュの総移動距離。0ならダッシュできない。
	float dashSpeed = 1000.0f;        // ダッシュの平均の速さ。蹴った瞬間がいちばん速く、だんだん dashEndSpeed まで遅くなる。
	float dashEndSpeed = 0.0f;        // ダッシュの終わりの速さ。着地の間もこの速さで進む（走りへ勢いをつなぐ）。dashSpeed 以下。
	float dashCrouchSeconds = 0.0f;   // 蹴る前にしゃがみ込む長さ（秒）。0ならしゃがまずにすぐ蹴る。地面にいるときの最初の1回だけ。
	float dashCrouchBlendSeconds = 0.12f; // しゃがみ込みに入るときの補間時間。長いほどなめらかに沈む。
	float dashCrouchSpeed = 0.0f;     // しゃがみ込み始めの進む速さ。しゃがみ終わりで0になる（踏ん張って止まる）。
	float dashKickBlendSeconds = 0.03f; // しゃがんだポーズから一蹴りへ移るときの補間時間。一蹴りはしゃがんだポーズから始まるので短くてよい。
	float dashChainKickStartTime = 0.0f; // 連打で続けて蹴るとき、一蹴りをどこから流すか（秒）。しゃがんだところを飛ばす。
	float dashLandSeconds = 0.0f;     // 着地の長さ（秒）。0なら着地せず、すぐ次の動きへ移る。
	float dashLandBlendSeconds = 0.08f; // 飛び出したポーズから着地へ移るときの補間時間。
	float dashAnimSpeed = 1.0f;       // 一蹴りのアニメーション（0.3秒）の再生速度（倍）。距離÷速さの時間で流し終わるよう合わせる。
	float dashAnimBlendSeconds = 0.05f; // ダッシュに入るときのアニメーションの補間時間。短いほど一気に前へ出る。
	float dashExitBlendSeconds = 0.5f;  // ダッシュから次の動きへ移るときの補間時間。長いほど飛び出したポーズが残る。
	// ダッシュ（着地まで）のあと、走り・歩きをどこから始めるか（秒）。着地の最後と前後の足が同じところ。
	// 最初から始めると前後の足が逆で、補間の間に足が入れ替わってつまずいたように見える。
	float dashLToRunStartTime = 0.0f;   // 左足で蹴ったあと（右足で着地）。
	float dashRToRunStartTime = 0.0f;   // 右足で蹴ったあと（左足で着地）。
	float dashLToWalkStartTime = 0.0f;
	float dashRToWalkStartTime = 0.0f;
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
	// 次の PlayAnimation だけで使う補間時間。0より小さければ animBlendSeconds を使う。
	// 前のステートが Exit で入れておく（例: ダッシュのあとは長めにして、ポーズを残す）。
	mutable float nextBlendSeconds = -1.0f;
	// 次の PlayAnimation で、クリップごとに再生を始める位置（秒）。0より小さければ最初から。使ったら全部消す。
	mutable float nextStartTimes[enPlayerAnimClip_Num] = {};

	PlayerStateContext()
	{
		ClearNextStartTimes();
	}
	void ClearNextStartTimes() const
	{
		for (float& time : nextStartTimes) {
			time = -1.0f;
		}
	}

	// アニメーションを切り替えて、speed 倍の速さで再生する。
	void PlayAnimation(int animClip, float speed = 1.0f) const
	{
		const float blend = nextBlendSeconds >= 0.0f ? nextBlendSeconds : settings.animBlendSeconds;
		const float startTime = (animClip >= 0 && animClip < enPlayerAnimClip_Num) ? nextStartTimes[animClip] : -1.0f;
		nextBlendSeconds = -1.0f;
		ClearNextStartTimes();
		if (startTime >= 0.0f) {
			modelRender->PlayAnimationFromTime(animClip, blend, startTime);
		}
		else {
			modelRender->PlayAnimation(animClip, blend);
		}
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
