#pragma once

#include "Actor.h"
#include "Faction.h"
#include "Status.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "CharacterModel.h"
#include "StatusEffects.h"
#include "HitTarget.h"
#include "MagicCaster.h"
#include "BodyLean.h"
#include "SpringBones.h"
#include "BodyLink.h"

// キャラクターごとの調整値。派生クラスが MakeConfig() で返す。
struct CharacterConfig
{
	CharacterMoveSettings move;
	CharacterModelDesc model;
	MagicCaster::Settings caster;
	float hitRadius = 45.0f;        // 弾に当たる球の半径。
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
	// モデルを更新したあと（ボーンに付けたものを動かす、など）。
	virtual void OnLateUpdate(float deltaTime)
	{
	}
	// アニメーションの姿勢ができた直後（体の連動・揺れ物より前）。骨の向きに手を加えるときに書く。
	virtual void OnPostAnimationPose(Skeleton& skeleton)
	{
	}
	// モデルの向き（体を傾けたあとの見た目の向き）。OnPostAnimationPose で体の前を知るのに使う。
	const Quaternion& GetModelRotation() const
	{
		return m_modelRotation;
	}
	// 体を描いたあと、自分だけのものを描く。
	virtual void OnRender(RenderContext& rc)
	{
	}
	// 体の連動の調整値。開発用の画面（ImGui）で変えるときに使う。
	BodyLinkSettings& GetBodyLinkSettings()
	{
		return m_bodyLink.GetSettings();
	}
	// 体の連動の調整値を、player.json の値に戻す。
	void ResetBodyLinkSettings()
	{
		m_bodyLink.ResetSettings();
	}

	CharacterInput m_input;
	CharacterMover m_mover;
	CharacterModel m_model;
	StatusEffects m_statusEffects;
	HitTarget m_hitTarget;
	MagicCaster m_magicCaster;

private:
	void UpdateInput(float deltaTime);
	// キャラコンの位置・回転を Transform に書く。
	void SyncTransform();

	BodyLean m_bodyLean;
	SpringBones m_springBones;
	BodyLink m_bodyLink;
	IInputSource* m_inputSource = nullptr;  // nullptr なら GetDefaultInputSource()。
	Quaternion m_modelRotation = Quaternion::Identity;
	Faction m_faction = Faction::Neutral;
	bool m_isRenderBody = true;
};
