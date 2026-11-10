#pragma once

#include <vector>

#include "Objects/StageRender/PropTypes.h"

// 描く物を、カメラからの距離で LOD（細かさの段）ごとに振り分ける（A2）。
// switchDistances[i] は LOD i に切り替わる距離。0 番目は 0（いちばん近い）。
class LodSelector
{
public:
	explicit LodSelector(std::vector<float> switchDistances);
	int GetNumLod() const
	{
		return static_cast<int>(m_switchDistances.size());
	}
	// in を LOD ごとの一覧 out[LOD 番号] に分ける。out は前の中身を消してから詰める。
	void Split(const PropList& in, const PropCullContext& context, std::vector<PropList>& out) const;

private:
	std::vector<float> m_switchDistances;
};
