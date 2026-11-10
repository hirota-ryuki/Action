#pragma once

// ステージの地形の1区画。
// 近い用・遠い用の2つのモデルを持ち、カメラからの距離でどちらか1つだけを描く。
// 当たり判定は近い用のモデルの形で作る。
class StageChunk
{
public:
	// model: 近い用のモデル、farModel: 遠い用のモデル（ポリゴンを減らしたもの）。center: 区画の真ん中（ゲームの座標）。
	void Init(const char* model, const char* farModel, const Vector3& center);
	void Render(RenderContext& rc);

	const Vector3& GetCenter() const
	{
		return m_center;
	}

private:
	ModelRender m_nearModel;
	ModelRender m_farModel;
	PhysicsStaticObject m_physicsStaticObject;
	Vector3 m_center = Vector3::Zero;
};
