#pragma once

// 三人称カメラが、壁・柱・床に埋まらないようにする。
// 頭の中心からカメラへ小さな球をすべらせ、当たったらカメラを当たる手前まで寄せる。
// - 寄せるのは同じフレームで、すぐに。離れて戻るのは、ゆっくり（なめらかに）。
// - プレイヤー・敵の体には当たらない（体が前を通るたびにカメラが揺れないように）。
// GameCamera が持ち、三人称のときだけ Solve を呼ぶ。
class CameraCollision
{
public:
	CameraCollision();
	// 前に寄せていた分を忘れる。次の Solve では、当たった所にすぐ置く。
	void Reset();
	// start（頭の中心）から position（置きたいカメラの位置）へ球をすべらせ、壁の手前に寄せたカメラの位置を返す。
	Vector3 Solve(const Vector3& start, const Vector3& position, float deltaTime);
	// 最後の Solve で返したカメラの位置の、start からの距離（cm）。
	float GetDistance() const
	{
		return m_distance;
	}

private:
	btSphereShape m_sphere;   // すべらせる球。
	float m_rate = 1.0f;      // start から置きたい位置までのうち、今カメラを置いている割合（0〜1）。
	bool m_isReset = true;    // true なら、次の Solve で m_rate をなめらかにせず、当たった割合にする。
	float m_distance = 0.0f;  // 最後の Solve で返したカメラの位置の、start からの距離（cm）。
};
