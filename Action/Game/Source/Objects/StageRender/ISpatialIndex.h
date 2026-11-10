#pragma once

#include "Objects/StageRender/PropTypes.h"

// 部品を場所で分けておき、「カメラから見えそうな所にある物」をまとめて取り出す仕組みの形。
// 格子（GridSpatialIndex）や四分木など、分け方を差し替えられるようにする。
class ISpatialIndex
{
public:
	virtual ~ISpatialIndex() = default;
	// 部品の一覧から、分け方を作る。instances は呼ぶ側が持ち続ける（ポインターで覚えるため）。
	virtual void Build(const std::vector<PropInstance>& instances) = 0;
	// 描く距離の内側で、視錐台にかかる場所の部品を out に足す。
	virtual void Query(const PropCullContext& context, PropList& out) const = 0;
};
