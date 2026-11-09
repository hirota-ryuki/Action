#include "stdafx.h"
#include "Objects/StageRender/StageRenderSettings.h"

#include "System/JsonFile.h"
#include "Objects/StageRender/GridSpatialIndex.h"
#include "Objects/StageRender/DistanceFilter.h"
#include "Objects/StageRender/FrustumFilter.h"
#include "Objects/StageRender/DensityFilter.h"
#include "Objects/StageRender/ScreenSizeFilter.h"

StageRenderSettings StageRenderSettings::Load(const char* jsonPath)
{
	StageRenderSettings settings;
	JsonFile::Json json;
	if (!JsonFile::TryLoad(jsonPath, json)) {
		// 読めなければ全部入りにする。
		for (const char* type : { "distance", "frustum", "density", "screenSize" }) {
			FilterSetting f;
			f.type = type;
			if (f.type == "density") {
				f.kinds = { "grass" };
			}
			settings.m_filters.push_back(f);
		}
		return settings;
	}

	settings.m_cellSize = json["spatialIndex"].value("cellSize", settings.m_cellSize);
	for (const auto& f : json["filters"]) {
		FilterSetting s;
		s.type = f.value("type", std::string());
		s.isEnabled = f.value("enabled", true);
		if (f.contains("kinds")) {
			for (const auto& k : f["kinds"]) {
				s.kinds.push_back(k.get<std::string>());
			}
		}
		s.startDistance = f.value("startDistance", s.startDistance);
		s.minKeepRatio = f.value("minKeepRatio", s.minKeepRatio);
		s.minScreenRatio = f.value("minScreenRatio", s.minScreenRatio);
		settings.m_filters.push_back(s);
	}
	return settings;
}

std::vector<std::unique_ptr<IPropFilter>> StageRenderSettings::CreateFilters() const
{
	std::vector<std::unique_ptr<IPropFilter>> filters;
	for (const FilterSetting& f : m_filters) {
		if (!f.isEnabled) {
			continue;
		}
		if (f.type == "distance") {
			filters.push_back(std::make_unique<DistanceFilter>());
		}
		else if (f.type == "frustum") {
			filters.push_back(std::make_unique<FrustumFilter>());
		}
		else if (f.type == "density") {
			filters.push_back(std::make_unique<DensityFilter>(f.kinds, f.startDistance, f.minKeepRatio));
		}
		else if (f.type == "screenSize") {
			filters.push_back(std::make_unique<ScreenSizeFilter>(f.minScreenRatio));
		}
	}
	return filters;
}

std::unique_ptr<ISpatialIndex> StageRenderSettings::CreateSpatialIndex() const
{
	return std::make_unique<GridSpatialIndex>(m_cellSize);
}
