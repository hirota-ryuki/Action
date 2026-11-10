#pragma once

#include "tkFile/ModelReducer/LodBuilder.h"

namespace nsK2EngineLow {
	/// <summary>
	/// モデルを軽くした tkm（LOD）を用意する入口。
	/// </summary>
	/// <remarks>
	/// モデル（.fbx か .tkm）と、LOD ごとの減らし方を渡すと、xxx_lod0.tkm・xxx_lod1.tkm … のパスを返す。
	/// ファイルを作るのは、まだない・もとのモデルの方が新しい・減らし方が変わった、のどれかのときだけ。
	/// 減らし方は xxx_lod.txt に書いておき、次からはそれと比べる。作った tkm は git に入れるので、
	/// 起動のたびに作ることはない。
	/// もとの xxx.tkm は変えない（ほかのモデルの読み込みには影響しない）。
	/// </remarks>
	class ModelReducer {
	public:
		using Level = LodBuilder::Level;
		/// <param name="filePath">モデルのパス（.fbx か .tkm）。</param>
		/// <param name="levels">LOD ごとの減らし方。0 番目がいちばん細かい。</param>
		/// <returns>LOD ごとの tkm のパス。作れなかったときは空。</returns>
		static std::vector<std::string> ResolveLodTkmFilePaths(const char* filePath, const std::vector<Level>& levels);
	};
}
