#pragma once

#include "Actor.h"

class PlayerStatus;

// 落ちているアイテムの見た目。派生クラスが MakeLook() で返す。
struct ItemLook
{
	std::string modelPath;
	EnModelUpAxis upAxis = enModelUpAxisY;
	float modelScale = 1.0f;
	float floatHeight = 30.0f;       // 地面から、モデルの原点までの高さ。
	float floatAmplitude = 5.0f;     // 上下にゆれる幅（片側）。
	float floatSeconds = 2.0f;       // 上下にゆれる1往復の秒数。
	float spinDegPerSecond = 90.0f;  // 1秒に回る角度。
};

// 落ちているアイテムの基底クラス。回りながら上下にゆれる。
// 置く・探す・消すのは ItemManager がする（Item は自分で自分を消さない）。拾うかどうかはプレイヤーのステートが決める。
// 派生クラスは、名前・一覧に出す絵・見た目・拾ったときの効き目だけを書く。
// 位置（GetPosition）は地面の上の点。モデルはそこから floatHeight だけ浮かせて描く。
class Item : public Actor
{
public:
	bool Start() final;
	void Update() final;
	void Render(RenderContext& rc) final;

	// 拾える一覧に出す名前と絵。
	virtual const wchar_t* GetName() const = 0;
	virtual const char* GetIconPath() const = 0;
	// 拾われたときの効き目。ItemManager::PickUp から呼ばれる。
	virtual void OnPickUp(PlayerStatus& status) = 0;

protected:
	virtual ItemLook MakeLook() const = 0;

private:
	// 置いた位置の真下の地面に合わせる。地面が見つからなければそのまま。
	void SnapToGround();

	ModelRender m_model;
	ItemLook m_look;
	float m_time = 0.0f;  // ゆれと回転に使う、置いてからの時間。
};
