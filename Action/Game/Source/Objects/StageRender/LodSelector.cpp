#include "stdafx.h"
#include "Objects/StageRender/LodSelector.h"

LodSelector::LodSelector(std::vector<float> switchDistances)
	: m_switchDistances(std::move(switchDistances))
{
	if (m_switchDistances.empty()) {
		m_switchDistances.push_back(0.0f);
	}
}

void LodSelector::Split(const PropList& in, const PropCullContext& context, std::vector<PropList>& out) const
{
	const int numLod = GetNumLod();
	out.resize(numLod);
	for (auto& list : out) {
		list.clear();
	}
	for (const PropInstance* p : in) {
		Vector3 diff = p->position;
		diff.Subtract(context.cameraPosition);
		const float distanceSq = diff.LengthSq();
		// 遠い段から順に、切り替わる距離を超えているかを見る。
		int lod = 0;
		for (int i = numLod - 1; i > 0; i--) {
			if (distanceSq >= m_switchDistances[i] * m_switchDistances[i]) {
				lod = i;
				break;
			}
		}
		out[lod].push_back(p);
	}
}
