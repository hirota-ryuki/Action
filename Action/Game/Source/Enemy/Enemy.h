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
	// 倒れた（消えた）あとでも、HP を最大に戻して、その場で立たせ直す（Idle から始める）。F1 の窓の「復活」ボタンから呼ぶ。
	void Revive();

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
	void OnAddPoseLayers(PoseLayers& layers) override;

private:
	// F1 の開発用の窓「黒騎士のモーション」。Walk・Run を選ぶと、AI とステートを止めて、その場でそのクリップを繰り返す。
	void UpdateDebugWindow();
	// 窓から Run を流している間、剣を持つ右腕（肩の前後・ひじから下の内外・手首）を回す（アニメーションの姿勢ができたあとに呼ぶ）。
	void RotateDebugArm(Skeleton& skeleton);
	// 窓から Attack1 を流している間、マントの骨を、肩のあたり（背中の上）と腰から下の2か所、根元側・先側の2本ずつ、後ろ・上へ回す
	// （構えのコマで止めて、角度を決める。決めた値は black_knight_anims.json に書き写して焼き込む）。
	void RotateDebugCape(Skeleton& skeleton);

	BlackKnightInputSource m_aiInputSource;
	int m_debugMotion = -1;           // 窓で選んだクリップ（EnEnemyAnimClip）。-1 ならふだんどおり AI で動く。
	int m_playingDebugMotion = -1;    // いま窓から流しているクリップ。-1 なら流していない。
	float m_debugPlaySpeed = 1.0f;    // 窓から流すときの再生の速さ。
	bool m_isDebugSpringSuspended = false;  // 窓で、揺れ物を止めている。
	bool m_debugPaused = false;       // 窓から流しているクリップを一時停止する。
	float m_debugCapeUpperDeg[3] = {30.0f, 12.0f, 6.0f};  // 背中の上（肩〜腰）のマントの根元側・先側の骨を、後ろ・上へ回す角度（度）。
	float m_debugCapeLowerDeg[3] = {10.0f, 30.0f, 50.0f};  // 腰から下のマントの根元側・先側の骨を、後ろ・上へ回す角度（度）。
	float m_debugPlayTime = 0.0f;    // 窓から流し始めてからの秒数（再生の速さを掛けたもの）。Attack1 の構えのコマで、マントを煽るのに使う。
	bool m_isDebugBurstDone = false;  // この流し方では、もう煽った。
	bool m_debugRestart = false;     // 窓から流しているクリップを、最初からもう一度流す（攻撃・やられなど、1回で終わるクリップ用）。
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
