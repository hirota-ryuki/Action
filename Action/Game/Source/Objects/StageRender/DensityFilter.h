#pragma once

#include <string>
#include <vector>

#include "Objects/StageRender/IPropFilter.h"

// 遠くの物を間引く（B2）。草のように数が多く、1本ずつは目立たない物に使う。
// startDistance までは全部残し、そこから描く距離に向けて、残す割合を minKeepRatio まで下げる。
// どれを残すかは、部品ごとに決まった乱数（keepValue）で決めるので、毎フレーム同じ物が残る（ちらつかない）。
class DensityFilter : public IPropFilter
{
public:
	DensityFilter(std::vector<std::string> kinds, float startDistance, float minKeepRatio);
	void Apply(PropList& list, const PropCullContext& context) const override;

private:
	std::vector<std::string> m_kinds;  // 間引く部品の種類（"grass" など）
	float m_startDistance;
	float m_minKeepRatio;
};
