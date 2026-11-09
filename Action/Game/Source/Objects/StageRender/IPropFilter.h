#pragma once

#include "Objects/StageRender/PropTypes.h"

// 描く候補の一覧から、要らない物を消すフィルターの形。
// フィルターはほかのフィルターのことを知らない。StageProps が設定の順に上から呼ぶだけ。
class IPropFilter
{
public:
	virtual ~IPropFilter() = default;
	virtual void Apply(PropList& list, const PropCullContext& context) const = 0;
};
