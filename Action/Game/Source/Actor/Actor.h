#pragma once

// ワールドに置くものの位置・回転・大きさ。
struct Transform
{
	Vector3 position = Vector3::Zero;
	Quaternion rotation = Quaternion::Identity;
	Vector3 scale = Vector3::One;
};

// ワールドに置くものの基底クラス。位置（Transform）だけを持つ。
// モデルを持つかどうかは派生クラスが決める（範囲・トリガー・音など、モデルのない Actor もあるため）。
// Transform が位置の正。動かす部品（キャラコンなど）を持つ派生クラスは、動いた結果を毎フレーム Transform に書く。
class Actor : public IGameObject
{
public:
	const Transform& GetTransform() const
	{
		return m_transform;
	}
	const Vector3& GetPosition() const
	{
		return m_transform.position;
	}
	// 派生クラスで、動かす部品の位置も合わせて変える。
	virtual void SetPosition(const Vector3& position)
	{
		m_transform.position = position;
	}
	const Quaternion& GetRotation() const
	{
		return m_transform.rotation;
	}
	// 正面の向き（回転を Z 軸にかけたもの）。
	Vector3 GetForward() const
	{
		Vector3 forward = Vector3::AxisZ;
		m_transform.rotation.Apply(forward);
		return forward;
	}

protected:
	Transform m_transform;
};
