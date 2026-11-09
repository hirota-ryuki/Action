#pragma once

#include "tkFile/TkmFile.h"

namespace nsK2EngineLow {
	/// <summary>
	/// 頂点とインデックスを、GPU が速く描ける順番に並べ替える（見た目は変わらない）。
	/// </summary>
	/// <remarks>
	/// 1. マテリアルごとに三角形の順番を、頂点キャッシュに当たりやすい順にする（meshopt_optimizeVertexCache）。
	/// 2. 手前の三角形が先に描かれやすい順にして、重ね塗りを減らす（meshopt_optimizeOverdraw）。
	/// 3. 頂点バッファを、インデックスで使われる順に並べ直し、使われていない頂点を消す（meshopt_optimizeVertexFetchRemap）。
	/// 簡略化（MeshSimplifier）のあとに通すと、減った頂点の掃除も兼ねる。
	/// </remarks>
	class VertexOrderOptimizer {
	public:
		/// <summary>
		/// 並べ替えたメッシュを返す。入力は変えない。
		/// </summary>
		static TkmFile::SMesh Process(const TkmFile::SMesh& in);
	};
}
