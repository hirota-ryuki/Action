#pragma once

// ステージの外へ落ちたかを判定し、戻る位置を覚えておく部品。
// 落ちたあと実際に戻す処理（位置・速さ・ステート）は、持ち主が行う。
class FallRespawn
{
public:
	// respawnPosition: 戻る位置。killHeight: この高さを下回ったら落ちたとみなす。
	void Init(const Vector3& respawnPosition, float killHeight)
	{
		m_respawnPosition = respawnPosition;
		m_killHeight = killHeight;
	}
	bool IsFallen(const Vector3& position) const
	{
		return position.y < m_killHeight;
	}
	const Vector3& GetRespawnPosition() const
	{
		return m_respawnPosition;
	}

private:
	Vector3 m_respawnPosition = Vector3::Zero;
	float m_killHeight = -1000.0f;
};
