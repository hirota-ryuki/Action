#pragma once

#include "tkFile/TkmFile.h"
#include "tkFile/ModelReducer/MeshSimplifier.h"

namespace nsK2EngineLow {
	/// <summary>
	/// 1つのモデルから、細かさの違うモデル（LOD）をいくつか作る。
	/// </summary>
	/// <remarks>
	/// LOD ごとに MeshSimplifier で減らし（割合が 1 なら減らさない）、VertexOrderOptimizer で並べ替える。
	/// 戻り値の [LOD 番号][メッシュパーツ番号]。
	/// </remarks>
	class LodBuilder {
	public:
		using Level = MeshSimplifier::Settings;
		static std::vector<std::vector<TkmFile::SMesh>> Build(
			const std::vector<TkmFile::SMesh>& meshParts,
			const std::vector<Level>& levels);
	};
}
