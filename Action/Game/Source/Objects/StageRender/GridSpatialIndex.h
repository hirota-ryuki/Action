#pragma once

#include "Objects/StageRender/ISpatialIndex.h"

// 地面を同じ大きさのマス（XZ 平面）に分ける。マスごとに部品が入る箱を覚えておき、
// 箱が描く距離の外か、視錐台の外ならマスごと飛ばす。中の部品を1つずつ調べるのは、合格したマスだけ。
class GridSpatialIndex : public ISpatialIndex
{
public:
	explicit GridSpatialIndex(float cellSize);
	void Build(const std::vector<PropInstance>& instances) override;
	void Query(const PropCullContext& context, PropList& out) const override;

private:
	struct Cell
	{
		Vector3 boxMin;   // 中の部品がまるごと入る箱
		Vector3 boxMax;
		std::vector<const PropInstance*> instances;
	};
	float m_cellSize;
	std::vector<Cell> m_cells;
};
