#include "stdafx.h"
#include "CameraCollision.h"

namespace
{
	// すべらせる球の半径（cm）。Near の面（カメラの前の、描き始める面）が壁に入らないだけの余白。
	constexpr float SPHERE_RADIUS = 15.0f;
	// 壁から離れたときに、元の距離へ戻る速さ。残りの差が、この秒数で 1/e（約 37%）になる。
	constexpr float RETURN_SECONDS = 0.3f;

	// 球をすべらせて、いちばん手前で当たったものを残す。プレイヤー・敵の体は飛ばす。
	struct SweepCallback : public btCollisionWorld::ClosestConvexResultCallback
	{
		SweepCallback() :
			btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f))
		{
		}

		btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override
		{
			if (convexResult.m_hitCollisionObject->getUserIndex() == enCollisionAttr_Character) {
				// キャラクターには当たらなかったことにする。
				return 1.0f;
			}
			return btCollisionWorld::ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
		}
	};
}

CameraCollision::CameraCollision() :
	m_sphere(SPHERE_RADIUS)
{
}

void CameraCollision::Reset()
{
	m_isReset = true;
}

Vector3 CameraCollision::Solve(const Vector3& start, const Vector3& position, float deltaTime)
{
	const Vector3 toCamera = position - start;
	const float length = toCamera.Length();
	if (length <= FLT_EPSILON) {
		m_distance = 0.0f;
		return position;
	}

	// 頭の中心からカメラを置きたい位置へ、球をすべらせる。
	btTransform from;
	btTransform to;
	from.setIdentity();
	to.setIdentity();
	from.setOrigin(btVector3(start.x, start.y, start.z));
	to.setOrigin(btVector3(position.x, position.y, position.z));
	SweepCallback callback;
	PhysicsWorld::GetInstance()->ConvexSweepTest(&m_sphere, from, to, callback);
	// どこまで進めたかの割合。当たらなければ 1（置きたい位置まで行ける）。
	const float hitRate = callback.hasHit() ? callback.m_closestHitFraction : 1.0f;

	if (m_isReset || hitRate < m_rate) {
		// 壁の中が映らないように、寄せるのはすぐに。
		m_rate = hitRate;
		m_isReset = false;
	}
	else {
		// 戻るのはゆっくり。経過時間を使い、FPS によらず同じ速さにする。
		m_rate += (hitRate - m_rate) * (1.0f - expf(-deltaTime / RETURN_SECONDS));
	}

	// 当たった点ではなく、頭の中心とカメラを結ぶ線の上に置く（柱の角で横へずれないように）。
	m_distance = length * m_rate;
	return start + toCamera * m_rate;
}
