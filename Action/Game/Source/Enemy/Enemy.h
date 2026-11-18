#pragma once

#include "Character.h"
#include "BlackKnightInputSource.h"
#include "EnemyStateMachine.h"

// 敵キャラクター（黒騎士）。
// 入力元（既定では黒騎士の AI）から CharacterInput を受け取って動くだけで、何をするかは AI が決める。
// 体の動かし方（立ち止まる・歩く・走る・剣を振る・凍結）の切り替えは EnemyStateMachine（ステートパターン）が行う。
// 陣営を変えれば（SetFaction）味方や中立のNPCとしても使える。
class Enemy : public Character
{
public:
	Enemy();

	Status& GetStatus() override
	{
		return m_status;
	}
	const Status& GetStatus() const override
	{
		return m_status;
	}
	bool IsFrozen() const
	{
		return m_statusEffects.IsFrozen();
	}

protected:
	CharacterConfig MakeConfig() const override;
	IInputSource& GetDefaultInputSource() override
	{
		return m_aiInputSource;
	}
	void OnStart() override;
	void UpdateAction(float deltaTime) override;
	// 攻撃のステートに入ったフレームのボタンだけを SkillController に渡す（倒れている間などに攻撃しないように）。
	void FilterSkillInput(CharacterInput& input) const override;

private:
	// F1 の開発用の窓「黒騎士のモーション」。Walk・Run を選ぶと、AI とステートを止めて、その場でそのクリップを繰り返す。
	void UpdateDebugWindow();
	// 窓から Run を流している間、剣を持つ右腕（肩の前後・ひじから下の内外・手首）を回す（アニメーションの姿勢ができたあとに呼ぶ）。
	void RotateDebugArm(Skeleton& skeleton);

	BlackKnightInputSource m_aiInputSource;
	int m_debugMotion = -1;           // 窓で選んだクリップ（EnEnemyAnimClip）。-1 ならふだんどおり AI で動く。
	int m_playingDebugMotion = -1;    // いま窓から流しているクリップ。-1 なら流していない。
	float m_debugPlaySpeed = 1.0f;    // 窓から流すときの再生の速さ。
	float m_debugShoulderDeg = 0.0f;  // 窓から Run を流すときに、右腕を肩で前へ回す角度（度。マイナスで後ろへ）。
	float m_debugForearmDeg = 0.0f;   // 上腕をひねって、ひじから下を内・外へ振る角度（度）。
	float m_debugElbowDeg = 0.0f;     // ひじを中心に、ひじから下を前へ回す角度（度。マイナスで後ろへ）。
	float m_debugWristBendDeg = 0.0f; // 手首を曲げる角度（度。ひじの曲がる軸まわり）。
	float m_debugWristSideDeg = 0.0f; // 手首を横へ倒す角度（度）。
	float m_debugWristTwistDeg = 0.0f;// 手首（前腕）をひねる角度（度）。
	Status m_status;
	EnemyStateMachine m_stateMachine;
	EnemyDamageState m_damage;  // 当たった・倒れて消えた（ステートと受け渡す）。
};
