#include "stdafx.h"
#include "Objects/StageRender/ScreenSizeFilter.h"

#include <algorithm>

ScreenSizeFilter::ScreenSizeFilter(float minScreenRatio)
	: m_minScreenRatio(minScreenRatio)
{
}

void ScreenSizeFilter::Apply(PropList& list, const PropCullContext& context) const
{
	const Vector3 camera = context.cameraPosition;
	list.erase(std::remove_if(list.begin(), list.end(), [&](const PropInstance* p) {
		Vector3 diff = p->position;
		diff.Subtract(camera);
		const float distance = diff.Length();
		if (distance <= p->radius) {
			return false;  // カメラが中にいる
		}
		// 画面の高さを 2 としたときの、直径の見える大きさ ÷ 2 = 画面の高さに対する割合。
		const float screenRatio = p->radius / distance * context.projectionScaleY;
		return screenRatio < m_minScreenRatio;
	}), list.end());
}
