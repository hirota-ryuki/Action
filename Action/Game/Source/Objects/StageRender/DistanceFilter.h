#pragma once

#include "Objects/StageRender/IPropFilter.h"

// 部品の種類ごとの「描く距離」より遠い物を消す。
// 区画の分け方はマスごとにしか距離を見ないので、マスの中の1つずつはここで見る。
class DistanceFilter : public IPropFilter
{
public:
	void Apply(PropList& list, const PropCullContext& context) const override;
};
