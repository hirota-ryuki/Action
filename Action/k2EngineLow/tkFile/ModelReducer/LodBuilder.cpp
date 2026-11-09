#include "k2EngineLowPreCompile.h"
#include "tkFile/ModelReducer/LodBuilder.h"
#include "tkFile/ModelReducer/VertexOrderOptimizer.h"

namespace nsK2EngineLow {
	std::vector<std::vector<TkmFile::SMesh>> LodBuilder::Build(
		const std::vector<TkmFile::SMesh>& meshParts,
		const std::vector<Level>& levels)
	{
		std::vector<std::vector<TkmFile::SMesh>> lods;
		for (const Level& level : levels) {
			std::vector<TkmFile::SMesh> parts;
			for (const auto& mesh : meshParts) {
				if (level.ratio < 1.0f) {
					parts.push_back(VertexOrderOptimizer::Process(MeshSimplifier::Process(mesh, level)));
				}
				else {
					parts.push_back(VertexOrderOptimizer::Process(mesh));
				}
			}
			lods.push_back(std::move(parts));
		}
		return lods;
	}
}
