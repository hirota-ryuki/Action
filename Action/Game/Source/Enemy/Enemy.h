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

	BlackKnightInputSource m_aiInputSource;
	int m_debugMotion = -1;           // 窓で選んだクリップ（EnEnemyAnimClip）。-1 ならふだんどおり AI で動く。
	int m_playingDebugMotion = -1;    // いま窓から流しているクリップ。-1 なら流していない。
	float m_debugPlaySpeed = 1.0f;    // 窓から流すときの再生の速さ。
	Status m_status;
	EnemyStateMachine m_stateMachine;
	EnemyDamageState m_damage;  // 当たった・倒れて消えた（ステートと受け渡す）。
};
