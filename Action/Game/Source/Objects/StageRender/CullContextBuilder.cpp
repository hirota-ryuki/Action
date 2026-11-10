#include "stdafx.h"
#include "Objects/StageRender/CullContextBuilder.h"

#include <cmath>

namespace
{
	// 行列の j 列目の4つ（m[0][j]〜m[3][j]）を足し引きした平面を作り、法線の長さを 1 にする。
	Vector4 MakePlane(const Matrix& m, int colA, float signB, int colB)
	{
		float p[4];
		for (int i = 0; i < 4; i++) {
			p[i] = m.m[i][colA] + (colB >= 0 ? signB * m.m[i][colB] : 0.0f);
		}
		const float length = std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
		return Vector4(p[0] / length, p[1] / length, p[2] / length, p[3] / length);
	}
}

PropCullContext CullContextBuilder::Build(Camera& camera)
{
	PropCullContext context;
	context.cameraPosition = camera.GetPosition();
	context.projectionScaleY = 1.0f / std::tan(camera.GetViewAngle() * 0.5f);

	// 位置 (x, y, z, 1) × ビュー×プロジェクション行列 = クリップ空間の (X, Y, Z, W)。
	// -W <= X <= W、-W <= Y <= W、0 <= Z <= W が画面の中なので、W±X などが 0 以上の側が内側になる。
	const Matrix& m = camera.GetViewProjectionMatrix();
	context.frustumPlanes[0] = MakePlane(m, 3, +1.0f, 0);  // 左   W + X
	context.frustumPlanes[1] = MakePlane(m, 3, -1.0f, 0);  // 右   W - X
	context.frustumPlanes[2] = MakePlane(m, 3, +1.0f, 1);  // 下   W + Y
	context.frustumPlanes[3] = MakePlane(m, 3, -1.0f, 1);  // 上   W - Y
	context.frustumPlanes[4] = MakePlane(m, 2, 0.0f, -1);  // 手前 Z
	context.frustumPlanes[5] = MakePlane(m, 3, -1.0f, 2);  // 奥   W - Z
	return context;
}
