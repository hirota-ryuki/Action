#include "k2EngineLowPreCompile.h"
#include "tkFile/ModelReducer/MeshSimplifier.h"

#include <meshoptimizer/meshoptimizer.h>

namespace nsK2EngineLow {
	namespace {
		// 形のずれと比べて、UV・法線のずれをどれだけ重く見るか。
		constexpr float UV_WEIGHT = 1.0f;
		constexpr float NORMAL_WEIGHT = 0.5f;
		// 1つの頂点から渡す属性の数（UV 2 + 法線 3）。
		constexpr size_t NUM_ATTRIBUTE = 5;
	}

	TkmFile::SMesh MeshSimplifier::Process(const TkmFile::SMesh& in, const Settings& settings)
	{
		TkmFile::SMesh out;
		out.isFlatShading = in.isFlatShading;
		out.materials = in.materials;
		out.vertexBuffer = in.vertexBuffer;

		const size_t numVertex = in.vertexBuffer.size();
		const float* positions = &in.vertexBuffer[0].pos.x;

		// UV と法線を、頂点ごとに並べた配列にする。
		std::vector<float> attributes(numVertex * NUM_ATTRIBUTE);
		for (size_t v = 0; v < numVertex; v++) {
			const auto& vertex = in.vertexBuffer[v];
			float* a = &attributes[v * NUM_ATTRIBUTE];
			a[0] = vertex.uv.x;
			a[1] = vertex.uv.y;
			a[2] = vertex.normal.x;
			a[3] = vertex.normal.y;
			a[4] = vertex.normal.z;
		}
		const float weights[NUM_ATTRIBUTE] = { UV_WEIGHT, UV_WEIGHT, NORMAL_WEIGHT, NORMAL_WEIGHT, NORMAL_WEIGHT };
		const unsigned int options = settings.prune ? meshopt_SimplifyPrune : 0;

		// マテリアルごとに減らす。頂点は全マテリアルで共有なので、そのまま渡せる。
		for (const auto& ib : in.indexBuffer32Array) {
			const size_t target = static_cast<size_t>(ib.indices.size() / 3 * settings.ratio) * 3;
			TkmFile::SIndexBuffer32 dst;
			dst.indices.resize(ib.indices.size());
			const size_t numIndex = meshopt_simplifyWithAttributes(
				dst.indices.data(), ib.indices.data(), ib.indices.size(),
				positions, numVertex, sizeof(TkmFile::SVertex),
				attributes.data(), NUM_ATTRIBUTE * sizeof(float), weights, NUM_ATTRIBUTE,
				nullptr, target, settings.maxError, options, nullptr);
			dst.indices.resize(numIndex);
			if (dst.indices.empty()) {
				// 全部消えたマテリアルは、空のインデックスバッファにすると描画で困るので、もとのまま残す。
				dst.indices = ib.indices;
			}
			out.indexBuffer32Array.push_back(std::move(dst));
		}
		return out;
	}
}
