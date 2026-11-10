#include "stdafx.h"
#include "Objects/StageRender/GridSpatialIndex.h"

#include <cmath>
#include <map>
#include <utility>

namespace
{
	// 箱が平面の外側にまるごとあるか。平面の法線の向きにいちばん出ている角で調べる。
	bool IsBoxOutside(const Vector4& plane, const Vector3& boxMin, const Vector3& boxMax)
	{
		const float x = plane.x >= 0.0f ? boxMax.x : boxMin.x;
		const float y = plane.y >= 0.0f ? boxMax.y : boxMin.y;
		const float z = plane.z >= 0.0f ? boxMax.z : boxMin.z;
		return plane.x * x + plane.y * y + plane.z * z + plane.w < 0.0f;
	}

	// 点から箱までのいちばん近い距離の2乗。
	float DistanceSqToBox(const Vector3& p, const Vector3& boxMin, const Vector3& boxMax)
	{
		const float dx = (std::max)((std::max)(boxMin.x - p.x, 0.0f), p.x - boxMax.x);
		const float dy = (std::max)((std::max)(boxMin.y - p.y, 0.0f), p.y - boxMax.y);
		const float dz = (std::max)((std::max)(boxMin.z - p.z, 0.0f), p.z - boxMax.z);
		return dx * dx + dy * dy + dz * dz;
	}
}

GridSpatialIndex::GridSpatialIndex(float cellSize)
	: m_cellSize(cellSize)
{
}

void GridSpatialIndex::Build(const std::vector<PropInstance>& instances)
{
	// マスの番号（x, z）ごとに集める。
	std::map<std::pair<int, int>, Cell> cells;
	for (const PropInstance& instance : instances) {
		const int ix = static_cast<int>(std::floor(instance.position.x / m_cellSize));
		const int iz = static_cast<int>(std::floor(instance.position.z / m_cellSize));
		const Vector3 r(instance.radius, instance.radius, instance.radius);
		Vector3 lo = instance.position;
		lo.Subtract(r);
		Vector3 hi = instance.position;
		hi.Add(r);
		auto found = cells.find({ ix, iz });
		if (found == cells.end()) {
			Cell cell;
			cell.boxMin = lo;
			cell.boxMax = hi;
			found = cells.emplace(std::make_pair(ix, iz), std::move(cell)).first;
		}
		Cell& cell = found->second;
		cell.boxMin.Min(lo);
		cell.boxMax.Max(hi);
		cell.instances.push_back(&instance);
	}
	m_cells.clear();
	for (auto& c : cells) {
		m_cells.push_back(std::move(c.second));
	}
}

void GridSpatialIndex::Query(const PropCullContext& context, PropList& out) const
{
	const float drawDistanceSq = context.model->drawDistance * context.model->drawDistance;
	for (const Cell& cell : m_cells) {
		if (DistanceSqToBox(context.cameraPosition, cell.boxMin, cell.boxMax) > drawDistanceSq) {
			continue;
		}
		bool isOutside = false;
		for (const Vector4& plane : context.frustumPlanes) {
			if (IsBoxOutside(plane, cell.boxMin, cell.boxMax)) {
				isOutside = true;
				break;
			}
		}
		if (isOutside) {
			continue;
		}
		out.insert(out.end(), cell.instances.begin(), cell.instances.end());
	}
}
