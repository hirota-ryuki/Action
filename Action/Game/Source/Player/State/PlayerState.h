#pragma once

#include "PlayerAnimator.h"
#include "State.h"

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
	Land,    // 空中から着地した。しゃがんで衝撃を受け、立ち上がる
	Frozen,  // 凍結などで行動できない
	Num,
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
	float dashAnimBlendSeconds = 0.05f; // しゃがまずに蹴るところから流すとき（連打で続けて出す・しゃがみを使わない）の補間時間。短いほど一気に前へ出る。
	float dashExitBlendSeconds = 0.5f;  // ダッシュから次の動きへ移るときの補間時間。長いほど飛び出したポーズが残る。
	// ダッシュ（着地まで）のあと、走り・歩きをどこから始めるか（秒）。着地の最後と前後の足が同じところ。
	// 最初から始めると前後の足が逆で、補間の間に足が入れ替わってつまずいたように見える。
	float dashToRunStartTime = 0.0f;
	float dashToWalkStartTime = 0.0f;
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
	// ジャンプ。跳ぶ（JumpStart）→ 空中（JumpLoop）→ 着地（JumpLand）の3本のクリップを順に流す。
	// 時間はどれもクリップの中の位置（秒）。再生速度を変えても、クリップのどこで切り替えるかは変わらない。
	float jumpStartTime = 0.0f;         // JumpStart をここから流す。
	float jumpToLoopTime = 0.0f;        // JumpStart がここまで進んだら JumpLoop へ移る。
	float jumpAnimSpeed = 1.0f;         // JumpStart の再生速度（倍）。
	float jumpStartBlendSeconds = 0.1f; // 地面の動きから JumpStart への補間時間。
	float jumpLoopBlendSeconds = 0.2f;  // JumpStart から JumpLoop・足場から落ちたときの JumpLoop への補間時間。
	float landStartTime = 0.0f;         // JumpLand をここから流す。
	float landEndTime = 0.0f;           // JumpLand がここまで進んだら Idle へ移る。landStartTime 以下なら着地せず、すぐ次の動きへ移る。
	float landAnimSpeed = 1.0f;         // JumpLand の再生速度（倍）。
	float landBlendSeconds = 0.1f;      // 空中から JumpLand への補間時間。
	float landExitBlendSeconds = 0.3f;  // JumpLand から次の動きへの補間時間。
	bool landCancelByMove = true;       // 着地の間に移動の入力があれば、すぐ歩き・走りへ移る。false なら最後まで着地する。
	float moveThreshold = 0.1f;       // 移動入力の長さがこれより小さければ立ち止まっているとみなす。
	float animBlendSeconds = 0.2f;    // アニメーションを切り替えるときの補間時間。
	float walkAnimMoveSpeed = 0.0f;   // 歩くアニメーションを等速で再生したとき、足が地面を送る速さ。0なら等速で再生する。
};

// ステートが触ってよいもの。ステートは Player クラスを直接は知らない。
struct PlayerStateContext
{
	CharacterMover* mover = nullptr;
	PlayerAnimator* animator = nullptr;
	const StatusEffects* statusEffects = nullptr;
	PlayerStateSettings settings;
	// ひとつ前のステート。Enter の中で、どこから来たかを見るのに使う。StateMachine が入れる。
	PlayerStateId previousId = PlayerStateId::Idle;
	// ダッシュのあと、移動の入力を離すまで走り続ける（走るボタンを押していなくても Run）。
	// Dash が立て（dashKeepRun のとき）、PlayerStateMachine が移動の入力がないフレームで下ろす。
	bool keepRunAfterDash = false;
};

// プレイヤーのステートの基底クラス。中身は共通の State（Character/StateMachine/State.h）にある。
using PlayerState = State<PlayerStateId, PlayerStateContext>;
