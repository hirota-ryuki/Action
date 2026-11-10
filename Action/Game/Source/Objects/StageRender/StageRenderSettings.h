#pragma once

#include <map>
#include <memory>
#include <vector>

#include "tkFile/ModelReducer/ModelReducer.h"
#include "Objects/StageRender/IPropFilter.h"
#include "Objects/StageRender/ISpatialIndex.h"
#include "Objects/StageRender/LodSelector.h"

// ステージの部品をどう描くか（区画の分け方・フィルターの並びと入り切り・LOD）の設定。
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

	// LOD を使うか。使わないときは、もとのモデル1つで描く。
	bool IsLodEnabled() const
	{
		return m_isLodEnabled;
	}
	// 部品の種類（tree など）の LOD ごとの減らし方。書いていない種類は空（LOD を使わない）。
	std::vector<nsK2EngineLow::ModelReducer::Level> GetLodLevels(const std::string& kind) const;
	// 部品の種類の LOD の振り分け係。
	LodSelector CreateLodSelector(const std::string& kind) const;

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
	struct LodLevelSetting
	{
		nsK2EngineLow::ModelReducer::Level level;
		float switchDistance = 0.0f;  // この LOD に切り替わる距離（cm）
	};
	float m_cellSize = 800.0f;  // 格子の1マスの幅（cm）
	std::vector<FilterSetting> m_filters;
	bool m_isLodEnabled = false;
	std::map<std::string, std::vector<LodLevelSetting>> m_lodLevels;  // 種類 → LOD ごとの設定
};
