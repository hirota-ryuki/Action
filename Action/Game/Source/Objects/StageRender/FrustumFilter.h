#pragma once

#include "Objects/StageRender/IPropFilter.h"

// 画面（視錐台）にまったく入らない物を消す（B1）。
// 部品を「足もとを中心にした球」とみなし、視錐台の6面のどれかの外にまるごとあれば消す。
class FrustumFilter : public IPropFilter
{
public:
	void Apply(PropList& list, const PropCullContext& context) const override;
};
