#pragma once

// 炎エフェクトを指定位置で燃やし続けるクラス。
// 元エフェクトはループしないため、2つの再生枠を交互に使い、
// 前の再生が消え切る前に次の再生を重ねて途切れないようにする。
class FireEffect : public IGameObject
{
public:
	FireEffect();
	~FireEffect();
	bool Start();
	void Update();

	void SetPosition(const Vector3& position)
	{
		m_position = position;
		m_isTransformDirty = true;
	}

	void SetScale(const Vector3& scale)
	{
		m_scale = scale;
		m_isTransformDirty = true;
	}

private:
	void PlayNext();
	void ApplyTransform(Effect& effect);

	static constexpr int kSlotCount = 2;

	Vector3 m_position = Vector3::Zero;
	Vector3 m_scale = Vector3::One;
	Effect m_effects[kSlotCount];
	int m_nextSlot = 0;
	int m_replayIntervalFrames = 0;   // 次の再生を重ねるまでのフレーム数。0以下なら重ねない（元エフェクトが無限ループ）。
	int m_framesSinceLastPlay = 0;
	bool m_isValid = false;           // エフェクトを読み込めたか。
	bool m_isTransformDirty = false;
};
