#pragma once

#include <memory>
#include <vector>

#include "Objects/StageRender/IPropFilter.h"
#include "Objects/StageRender/ISpatialIndex.h"

// ステージの部品をどう描くか（区画の分け方・フィルターの並びと入り切り）の設定。
// Assets/data/stage/render_settings.json から読む。ビルドし直さずに入り切りして、効果を比べられる。
class StageRenderSettings
{
public:
	// 読めなかったときは、全部入りの既定値になる。
	static StageRenderSettings Load(const char* jsonPath);

	// 設定に書いた順に、入っているフィルターを作る。
	std::vector<std::unique_ptr<IPropFilter>> CreateFilters() const;
	// 部品の種類ごとに1つ、区画の分け方を作る。
	std::unique_ptr<ISpatialIndex> CreateSpatialIndex() const;

private:
	struct FilterSetting
	{
		std::string type;  // distance / frustum / density / screenSize
		bool isEnabled = true;
		std::vector<std::string> kinds;  // density 用
		float startDistance = 1200.0f;   // density 用（cm）
		float minKeepRatio = 0.35f;      // density 用
		float minScreenRatio = 0.004f;   // screenSize 用
	};
	float m_cellSize = 800.0f;  // 格子の1マスの幅（cm）
	std::vector<FilterSetting> m_filters;
};
