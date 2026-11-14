#pragma once

#include "Actor.h"
#include "Faction.h"
#include "Status.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "CharacterModel.h"
#include "StatusEffects.h"
#include "HitTarget.h"
#include "SkillController.h"
#include "BodyLean.h"
#include "SpringBones.h"
#include "BodyLink.h"

// 魔法を撃つ位置の調整値。
struct CharacterCastSettings
{
	float castHeight = 70.0f;   // 照準の指定がないとき、足元からこの高さで撃つ。
	float castForward = 60.0f;  // 自分のカプセルに当たらないよう、撃つ位置を前に出す距離。
};

// キャラクターごとの調整値。派生クラスが MakeConfig() で返す。
struct CharacterConfig
{
	CharacterMoveSettings move;
	CharacterModelDesc model;
	CharacterCastSettings cast;
	float hitRadius = 45.0f;        // 魔法に当たる球の半径。
	float hitCenterHeight = 60.0f;  // 足元から当たり判定の中心までの高さ。
	float maxHp = 100.0f;
	BodyLeanSettings lean;              // 体の傾き。既定は使わない。
	SpringBonesSettings springBones;    // 髪・スカートの揺れ。既定は使わない。
	BodyLinkSettings bodyLink;          // 脚に合わせた骨盤・背骨・肩・首の連動。既定は使わない。
};

// Player・Enemy など、キャラクターの基底クラス。
// 全キャラクターに共通の部品（入力・移動・モデル・状態異常・当たり判定・魔法）を持ち、
// 初期化の順番と1フレームの順番をここで決める。派生クラスは「動く」部分（UpdateAction）と、自分だけの部品を書く。
// 位置は Actor の Transform が正。キャラコン（CharacterMover）が動いた結果を、毎フレーム Transform に書く。
class Character : public Actor
{
public:
	bool Start() final;
	void Update() final;
	void Render(RenderContext& rc) final;

	// Start() の前に呼んでもよい。キャラコンの位置も合わせて変える。
	void SetPosition(const Vector3& position) override;

	// 陣営。Start() の前後どちらで呼んでもよい。
	void SetFaction(Faction faction);
	Faction GetFaction() const
	{
		return m_faction;
	}

	// 入力元を差し替える（AIで動かす、手で操作する、などのデバッグ用）。nullptr で既定の入力元に戻る。
	void SetInputSource(IInputSource* inputSource)
	{
		m_inputSource = inputSource;
	}
	// このフレームに受け取った入力。デバッグ表示用。
	const CharacterInput& GetInput() const
	{
		return m_input;
	}
	StatusEffects& GetStatusEffects()
	{
		return m_statusEffects;
	}
	// UI などの、読むだけの側に渡す。
	const StatusEffects& GetStatusEffects() const
	{
		return m_statusEffects;
	}
	// 遊んでいる間に変わる値。派生クラスが自分の Status（PlayerStatus など）を返す。
	virtual Status& GetStatus() = 0;
	// 読むだけの側（UI など）に渡す const 版。
	virtual const Status& GetStatus() const = 0;

	// 持っているスキル。一人称の腕が、撃った・溜めているを見るのに使う。
	const SkillController& GetSkillController() const
	{
		return m_skillController;
	}

	// 体を描くか。一人称カメラのときは false にする。
	void SetRenderBody(bool isRenderBody)
	{
		m_isRenderBody = isRenderBody;
	}
	bool IsRenderBody() const
	{
		return m_isRenderBody;
	}

protected:
	explicit Character(Faction faction);

	// 調整値。Start() で1回だけ呼ぶ。
	virtual CharacterConfig MakeConfig() const = 0;
	// SetInputSource(nullptr) のときに使う入力元。
	virtual IInputSource& GetDefaultInputSource() = 0;
	// 共通の部品を Init したあとに呼ぶ。自分だけの部品の Init を書く。
	virtual void OnStart()
	{
	}
	// 入力元に渡す情報に、派生クラスだけが知っていることを足す（一人称か、など）。
	virtual void FillInputContext(InputSourceContext& context) const
	{
	}
	// ③ 動く。CharacterMover の Move 系を1回だけ呼ぶ（凍結中も重力のために呼ぶ）。
	virtual void UpdateAction(float deltaTime) = 0;
	// スキルを使う部品（SkillController）に渡す入力を書き換える。攻撃のモーションに合わせて撃つ、などに使う。
	// input には、このフレームに受け取った入力の写しが入っている（m_input は変えない）。
	virtual void FilterSkillInput(CharacterInput& input) const
	{
	}
	// モデルを更新したあと（ボーンに付けたものを動かす、など）。
	virtual void OnLateUpdate(float deltaTime)
	{
	}
	// 体を描いたあと、自分だけのものを描く。
	virtual void OnRender(RenderContext& rc)
	{
	}
	// 揺れ物の調整値。開発用の画面（ImGui）で変えるときに使う。
	SpringBonesSettings& GetSpringBonesSettings()
	{
		return m_springBones.GetSettings();
	}
	// ジャンプ中の度合い（0〜1）を、体の連動と揺れ物に渡す。モデルを更新する前（UpdateAction）に呼ぶ。
	void SetJumpWeight(float jumpWeight)
	{
		m_bodyLink.SetJumpWeight(jumpWeight);
		m_springBones.SetJumpWeight(jumpWeight);
	}
	// 攻撃中の度合い（0〜1）を、体の連動に渡す。モデルを更新する前（UpdateAction）に呼ぶ。
	void SetAttackWeight(float attackWeight)
	{
		m_bodyLink.SetAttackWeight(attackWeight);
	}

	CharacterInput m_input;
	CharacterMover m_mover;
	CharacterModel m_model;
	StatusEffects m_statusEffects;
	HitTarget m_hitTarget;
	SkillController m_skillController;

private:
	void UpdateInput(float deltaTime);
	// 動けるなら、入力を SkillController に渡してスキルを使う。動けなければ溜めを捨てる。
	void UpdateSkill();
	// スキル（魔法など）を撃つ位置と向き。照準があればそちら、なければ足元から castHeight の高さで正面（水平）へ。
	void MakeCastPose(Vector3& origin, Vector3& direction) const;
	// 照準があるとき、撃つ位置を offset だけずらし、向きを照準の先の点へ向け直す（一人称で、右手から撃つ）。
	void ApplyAimOffset(const Vector3& offset, Vector3& origin, Vector3& direction) const;
	// キャラコンの位置・回転を Transform に書く。
	void SyncTransform();

	BodyLean m_bodyLean;
	SpringBones m_springBones;
	BodyLink m_bodyLink;
	CharacterCastSettings m_castSettings;
	IInputSource* m_inputSource = nullptr;  // nullptr なら GetDefaultInputSource()。
	Faction m_faction = Faction::Neutral;
	bool m_isRenderBody = true;
};
