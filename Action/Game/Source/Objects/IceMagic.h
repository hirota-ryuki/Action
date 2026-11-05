#pragma once

class Enemy;

// プレイヤーが撃つ氷魔法の弾。
// まっすぐ飛び、敵か地形に当たるか射程が尽きると着弾エフェクトを出す。
// 敵に当たった場合はその敵を一定時間凍らせる。
// 着弾後も軌跡と着弾エフェクトが消え切るまで残り、消えたら自分を削除する。
class IceMagic : public IGameObject
{
public:
	IceMagic();
	~IceMagic();
	bool Start();
	void Update();

	// NewGO 直後、Start() より前に呼ぶ。direction は正規化していなくてよい。
	void Init(const Vector3& position, const Vector3& direction);

private:
	enum class EnState
	{
		Flying,      // 飛翔中。
		Vanishing,   // 着弾済み。エフェクトが消え切るのを待つ。
	};

	void UpdateFlying();
	void UpdateVanishing();
	// from→to の移動中に当たる敵を探す。見つからなければ nullptr。
	Enemy* FindHitEnemy(const Vector3& from, const Vector3& to, Vector3& hitPosition) const;
	void Explode(const Vector3& hitPosition);
	void ApplyBulletTransform();

	Effect m_bulletEffect;
	Effect m_hitEffect;
	Vector3 m_position = Vector3::Zero;
	Vector3 m_direction = Vector3::AxisZ;     // 進行方向（正規化済み）。
	Quaternion m_rotation = Quaternion::Identity;
	float m_flightDistance = 0.0f;           // ここまでに飛んだ距離。射程判定に使う。
	EnState m_state = EnState::Flying;
	bool m_isValid = false;                   // エフェクトを読み込めたか。
};
