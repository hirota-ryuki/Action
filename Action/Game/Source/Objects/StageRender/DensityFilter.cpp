#include "stdafx.h"
#include "Objects/StageRender/DensityFilter.h"

#include <algorithm>
#include <cmath>

DensityFilter::DensityFilter(std::vector<std::string> kinds, float startDistance, float minKeepRatio)
	: m_kinds(std::move(kinds))
	, m_startDistance(startDistance)
	, m_minKeepRatio(minKeepRatio)
{
}

void DensityFilter::Apply(PropList& list, const PropCullContext& context) const
{
	if (std::find(m_kinds.begin(), m_kinds.end(), context.model->kind) == m_kinds.end()) {
		return;
	}
	const float range = (std::max)(context.model->drawDistance - m_startDistance, 1.0f);
	const Vector3 camera = context.cameraPosition;
	list.erase(std::remove_if(list.begin(), list.end(), [&](const PropInstance* p) {
		Vector3 diff = p->position;
		diff.Subtract(camera);
		const float distance = diff.Length();
		if (distance <= m_startDistance) {
			return false;
		}
		const float t = (std::min)((distance - m_startDistance) / range, 1.0f);
		const float keepRatio = 1.0f + (m_minKeepRatio - 1.0f) * t;
		return p->keepValue >= keepRatio;
	}), list.end());
}
