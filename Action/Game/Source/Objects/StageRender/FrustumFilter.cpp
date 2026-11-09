#include "stdafx.h"
#include "Objects/StageRender/FrustumFilter.h"

#include <algorithm>

void FrustumFilter::Apply(PropList& list, const PropCullContext& context) const
{
	list.erase(std::remove_if(list.begin(), list.end(), [&](const PropInstance* p) {
		for (const Vector4& plane : context.frustumPlanes) {
			const float d = plane.x * p->position.x + plane.y * p->position.y + plane.z * p->position.z + plane.w;
			if (d < -p->radius) {
				return true;
			}
		}
		return false;
	}), list.end());
}
