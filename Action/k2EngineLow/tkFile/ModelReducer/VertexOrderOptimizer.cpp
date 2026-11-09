#include "k2EngineLowPreCompile.h"
#include "tkFile/ModelReducer/VertexOrderOptimizer.h"

#include <meshoptimizer/meshoptimizer.h>

namespace nsK2EngineLow {
	namespace {
		// 重ね塗りを減らすために、頂点キャッシュの効率をどこまで下げてよいか（1.05 = 5% まで）。meshoptimizer の推奨値。
		constexpr float OVERDRAW_THRESHOLD = 1.05f;
	}

	TkmFile::SMesh VertexOrderOptimizer::Process(const TkmFile::SMesh& in)
	{
		TkmFile::SMesh out;
		out.isFlatShading = in.isFlatShading;
		out.materials = in.materials;
		const size_t numVertex = in.vertexBuffer.size();
		const size_t positionStride = sizeof(TkmFile::SVertex);
		const float* positions = &in.vertexBuffer[0].pos.x;

		// 1. 2. マテリアルごとに三角形の順番を並べ替える。
		std::vector<std::vector<uint32_t>> indexArrays;
		for (const auto& ib : in.indexBuffer32Array) {
			std::vector<uint32_t> cache(ib.indices.size());
			meshopt_optimizeVertexCache(cache.data(), ib.indices.data(), ib.indices.size(), numVertex);
			std::vector<uint32_t> overdraw(ib.indices.size());
			meshopt_optimizeOverdraw(overdraw.data(), cache.data(), cache.size(), positions, numVertex, positionStride, OVERDRAW_THRESHOLD);
			indexArrays.push_back(std::move(overdraw));
		}

		// 3. 全マテリアルのインデックスをつないで、頂点の新しい番号（remap）を作る。使われない頂点は ~0u になる。
		std::vector<uint32_t> allIndices;
		for (const auto& indices : indexArrays) {
			allIndices.insert(allIndices.end(), indices.begin(), indices.end());
		}
		std::vector<uint32_t> remap(numVertex);
		const size_t numUsedVertex = meshopt_optimizeVertexFetchRemap(remap.data(), allIndices.data(), allIndices.size(), numVertex);

		out.vertexBuffer.resize(numUsedVertex);
		for (size_t v = 0; v < numVertex; v++) {
			if (remap[v] != ~0u) {
				out.vertexBuffer[remap[v]] = in.vertexBuffer[v];
			}
		}
		for (auto& indices : indexArrays) {
			TkmFile::SIndexBuffer32 ib;
			ib.indices.reserve(indices.size());
			for (uint32_t index : indices) {
				ib.indices.push_back(remap[index]);
			}
			out.indexBuffer32Array.push_back(std::move(ib));
		}
		return out;
	}
}
