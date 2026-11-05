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

// キャラクターが1フレームに受け取る入力。
// キャラクターはこれを受け取って動くだけで、パッドやカメラ、AIのことは知らない。
// 値だけの構造体なので、画面に表示したり記録・再生したりしやすい。
struct CharacterInput
{
	Vector3 moveDirection = Vector3::Zero;  // 移動したい方向（ワールドのXZ）。長さ0〜1で速さの割合を表す。
	Vector3 lookDirection = Vector3::Zero;  // 向きたい方向。ゼロなら移動方向を向く。
	bool isRun = false;
	ButtonState jump;
	ButtonState rush;
	ButtonState fire;
	int magicSwitch = 0;                    // -1: 前の魔法へ、+1: 次の魔法へ、0: 切り替えない。
	bool hasAim = false;                    // true なら aimOrigin/aimDirection の方向へ撃つ。false ならキャラクターの正面へ撃つ。
	Vector3 aimOrigin = Vector3::Zero;
	Vector3 aimDirection = Vector3::AxisZ;
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
