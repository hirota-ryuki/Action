#pragma once

#include "Faction.h"

// ボタン1つ分の状態。押しているかどうかを毎フレーム渡すと、押した瞬間・離した瞬間を判定する。
struct ButtonState
{
	bool isTrigger = false;  // このフレームで押された。
	bool isPress = false;    // 押されている。
	bool isRelease = false;  // このフレームで離された。

	void Update(bool isPressNow)
	{
		isTrigger = isPressNow && !isPress;
		isRelease = !isPressNow && isPress;
		isPress = isPressNow;
	}
};

// 入力のボタン。何に使うか（どのスキルか）は知らない。どのキーを当てるかは入力元が決める。
//   Action1  プレイヤーはマウス左ボタン / Yボタン
//   Action2  プレイヤーはマウス右ボタン / 右トリガー
enum class ActionButton
{
	Action1,
	Action2,
	Num,
};

// キャラクターが1フレームに受け取る入力。
// キャラクターはこれを受け取って動くだけで、パッドやカメラ、AIのことは知らない。
// 値だけの構造体なので、画面に表示したり記録・再生したりしやすい。
struct CharacterInput
{
	Vector3 moveDirection = Vector3::Zero;  // 移動したい方向（ワールドのXZ）。長さ0〜1で速さの割合を表す。
	Vector3 lookDirection = Vector3::Zero;  // 向きたい方向。ゼロなら移動方向を向く。
	Vector3 attackDirection = Vector3::Zero; // 攻撃した瞬間に向く方向（水平。プレイヤーはカメラの向き）。ゼロなら向きを変えない。
	bool isRun = false;
	ButtonState jump;
	ButtonState dash;
	ButtonState actions[static_cast<int>(ActionButton::Num)];  // Action1・Action2 のボタン。何に使うかは SkillController の登録で決まる。
	int attackIndex = 0;                    // Action1 で出す攻撃の番号。黒騎士の攻撃1〜3を 0〜2 で選ぶ。プレイヤーは使わない。
	bool hasAim = false;                    // true なら aimOrigin/aimDirection の方向へ撃つ。false ならキャラクターの正面へ撃つ。
	Vector3 aimOrigin = Vector3::Zero;
	Vector3 aimDirection = Vector3::AxisZ;
	// hasAim のとき、そのボタンのスキルだけ、撃つ位置をこれだけずらす（一人称で、Action1 の氷魔法を右手から撃つ）。
	// 向きは、照準の先の点へ向け直す（画面の真ん中に飛んでいくように）。
	Vector3 aimOffsets[static_cast<int>(ActionButton::Num)];

	ButtonState& Action(ActionButton button)
	{
		return actions[static_cast<int>(button)];
	}
	const ButtonState& Action(ActionButton button) const
	{
		return actions[static_cast<int>(button)];
	}
	Vector3& AimOffset(ActionButton button)
	{
		return aimOffsets[static_cast<int>(button)];
	}
	const Vector3& AimOffset(ActionButton button) const
	{
		return aimOffsets[static_cast<int>(button)];
	}
};

// 入力元が操作対象について知ってよい情報。
struct InputSourceContext
{
	const IGameObject* self = nullptr;  // 操作対象。AIが自分を狙わないように使う。
	Faction faction = Faction::Neutral;
	Vector3 position = Vector3::Zero;   // 足元の位置。
	Vector3 forward = Vector3::AxisZ;   // 正面の向き。
	float deltaTime = 0.0f;
	bool isFirstPerson = false;         // 一人称視点か。プレイヤーの照準をカメラに合わせるのに使う。
};

// 入力元のインターフェース。プレイヤーの操作もNPCのAIもこれを実装する。
// キャラクターが持つ入力元を差し替えるだけで、操作する人（またはAI）を入れ替えられる。
class IInputSource
{
public:
	virtual ~IInputSource() = default;
	// 毎フレーム呼ばれる。input には前フレームの値が入っているので、上書きして返す。
	// ButtonState は Update() に「押しているか」を渡すこと（押した瞬間の判定に前フレームの値を使うため）。
	virtual void Update(const InputSourceContext& context, CharacterInput& input) = 0;
};
