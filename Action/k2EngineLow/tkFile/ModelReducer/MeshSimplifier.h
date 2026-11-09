#pragma once

#include "tkFile/TkmFile.h"

namespace nsK2EngineLow {
	/// <summary>
	/// 形をなるべく崩さずに、三角形の数を減らす（meshopt_simplifyWithAttributes）。
	/// </summary>
	/// <remarks>
	/// インデックスだけを減らし、頂点バッファはそのまま返す。使われなくなった頂点の掃除は
	/// VertexOrderOptimizer に任せる（部品を1つの仕事に絞るため）。
	/// UV と法線も見て減らすので、テクスチャの継ぎ目や角が崩れにくい。
	/// </remarks>
	class MeshSimplifier {
	public:
		struct Settings {
			float ratio = 0.5f;      // 残す三角形の割合（0〜1）。
			float maxError = 0.02f;  // 許す形のずれ。モデルの大きさを 1 としたときの割合（0.02 = 2%）。
			bool prune = false;      // 小さく離れた部品（草の細い葉など）を、消してでも減らすか。遠い LOD 向け。
		};
		/// <summary>
		/// 三角形を減らしたメッシュを返す。入力は変えない。
		/// </summary>
		static TkmFile::SMesh Process(const TkmFile::SMesh& in, const Settings& settings);
	};
}
