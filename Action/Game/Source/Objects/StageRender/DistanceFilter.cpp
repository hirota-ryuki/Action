#include "stdafx.h"
#include "Objects/StageRender/DistanceFilter.h"

#include <algorithm>

void DistanceFilter::Apply(PropList& list, const PropCullContext& context) const
{
	const float drawDistanceSq = context.model->drawDistance * context.model->drawDistance;
	const Vector3 camera = context.cameraPosition;
	list.erase(std::remove_if(list.begin(), list.end(), [&](const PropInstance* p) {
		Vector3 diff = p->position;
		diff.Subtract(camera);
		return diff.LengthSq() > drawDistanceSq;
	}), list.end());
}
