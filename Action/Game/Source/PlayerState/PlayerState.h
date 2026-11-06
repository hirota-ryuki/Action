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
	Stop,    // 走っていて入力を離した。勢いで滑りながら踏ん張って止まる
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
	enPlayerAnimClip_DashL,  // ダッシュ（しゃがむ → 左足で蹴って低く滑る → 右足で着地）を通しで1本
	enPlayerAnimClip_DashR,  // ダッシュ（右足で蹴る）
	enPlayerAnimClip_StopL,  // 走りから止まる（左足を前へ突っ張り、腰を落として滑る → 起き上がる）
	enPlayerAnimClip_StopR,  // 走りから止まる（右足を前へ突っ張る）
	enPlayerAnimClip_Num,
};

// ステートの調整用の数値。歩く・走る・跳ぶ速さは CharacterMoveSettings にある。
struct PlayerStateSettings
{
	float dashDistance = 300.0f;      // ダッシュの総移動距離。0ならダッシュできない。
	float dashSpeed = 1000.0f;        // ダッシュの平均の速さ。蹴った瞬間がいちばん速く、だんだん dashEndSpeed まで遅くなる。
	float dashEndSpeed = 0.0f;        // ダッシュの終わりの速さ。着地の間もこの速さで進む（走りへ勢いをつなぐ）。dashSpeed 以下。
	// ダッシュのクリップは「しゃがむ → 蹴って飛ぶ → 着地」を通しで1本にしてある。各区間の長さはクリップに合わせる。
	float dashCrouchSeconds = 0.0f;   // しゃがむ長さ（秒）。0ならしゃがまずにすぐ蹴る。地面にいるときの最初の1回だけ。
	float dashCrouchBlendSeconds = 0.1f; // ダッシュのクリップに入るときの補間時間（しゃがむとき）。
	float dashCrouchSpeed = 0.0f;     // しゃがみ始めの進む速さ。しゃがみ終わりで0になる。
	float dashKickStartTime = 0.0f;   // クリップの中で、蹴って飛び始めるところ（秒）。しゃがまないときは、ここから流す。
	float dashLandSeconds = 0.0f;     // 着地の長さ（秒）。0なら着地せず、すぐ次の動きへ移る。
	float dashAnimSpeed = 1.0f;       // ダッシュのアニメーションの再生速度（倍）。
	float dashAnimBlendSeconds = 0.05f; // ダッシュに入るときのアニメーションの補間時間。短いほど一気に前へ出る。
	float dashExitBlendSeconds = 0.5f;  // ダッシュから次の動きへ移るときの補間時間。長いほど飛び出したポーズが残る。
	// ダッシュ（着地まで）のあと、走り・歩きをどこから始めるか（秒）。着地の最後と前後の足が同じところ。
	// 最初から始めると前後の足が逆で、補間の間に足が入れ替わってつまずいたように見える。
	float dashLToRunStartTime = 0.0f;   // 左足で蹴ったあと（右足で着地）。
	float dashRToRunStartTime = 0.0f;   // 右足で蹴ったあと（左足で着地）。
	float dashLToWalkStartTime = 0.0f;
	float dashRToWalkStartTime = 0.0f;
	// true なら、着地に入ったとき走るボタンを押していれば、着地の間に走りのアニメーションへ補間しておく（見た目だけ）。
	// false なら着地を最後まで流してから補間する（ダッシュのクリップが最後のポーズで止まってから補間するので、一瞬止まって見える）。
	bool dashLandIntoRun = false;
	bool dashCanChain = true;      // ダッシュ中にもう一度押すと、着地のあとすぐ次のダッシュを出す。false なら着地まで入力を受け付けない。
	bool dashEndWithStop = false;  // ダッシュのあと移動の入力がなければ、Idle ではなく Stop（勢いで滑って止まる）へ移る。
	bool dashKeepRun = false;      // ダッシュのあとは、走るボタンを離していても、移動の入力を離すまで走り続ける。
	// 走りから止まる（Stop）。クリップは「前の足を突っ張って滑る → 起き上がって立つ」の通しの1本。
	float stopSeconds = 0.0f;           // 止まるクリップを流す長さ（秒）。0なら滑らずにすぐ立ち止まる。
	float stopSlideSeconds = 0.0f;      // 勢いで滑る長さ（秒）。走る速さからだんだん遅くなって0になる。
	float stopBlendSeconds = 0.1f;      // 走りから止まるクリップへの補間時間。
	float stopExitBlendSeconds = 0.3f;  // 止まるクリップから次の動きへの補間時間。
	// 走りから歩きに移ってこの時間（秒）のうちに移動の入力を離しても、止まる動きにする。
	// 走るボタンを移動キーより少し先に離したとき（ほぼ同時に離したとき）にも滑って止まるように。
	float stopAfterRunSeconds = 0.0f;
	// どちらの足で踏ん張るかは、止まった瞬間の走りの位置で決める。走りの中で、その足がちょうど前に着くところ（秒）。
	float runAnimSeconds = 0.0f;        // 走りのクリップの1周期の長さ（秒）。
	float stopLRunTime = 0.0f;          // 左足が前に着くところ。ここに近ければ StopL。
	float stopRRunTime = 0.0f;          // 右足が前に着くところ。ここに近ければ StopR。
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
	// ひとつ前のステート。Enter の中で、どこから来たかを見るのに使う。PlayerStateMachine が入れる。
	PlayerStateId previousId = PlayerStateId::Idle;
	// ダッシュのあと、移動の入力を離すまで走り続ける（走るボタンを押していなくても Run）。
	// Dash が立て（dashKeepRun のとき）、PlayerStateMachine が移動の入力がないフレームで下ろす。
	bool keepRunAfterDash = false;
	// 走りのアニメーションの今の位置（秒、ループ前の通算）。Run が毎フレーム書き、Stop が踏ん張る足を決めるのに使う。
	float runAnimTime = 0.0f;

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
