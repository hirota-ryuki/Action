#include "k2EngineLowPreCompile.h"
#include "tkFile/ModelReducer/ModelReducer.h"
#include "tkFile/FbxRuntimeImporter.h"

#include <fstream>
#include <sstream>

namespace nsK2EngineLow {
	namespace {
		// 減らし方の書き方を変えたら上げる（前に作った LOD を作り直させるため）。
		constexpr int SETTINGS_VERSION = 1;

		bool GetWriteTime(const std::string& path, ULONGLONG& outTime)
		{
			WIN32_FILE_ATTRIBUTE_DATA data;
			if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data)) {
				return false;
			}
			outTime = (static_cast<ULONGLONG>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime;
			return true;
		}

		std::string ReadText(const std::string& path)
		{
			std::ifstream file(path);
			std::stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		}

		std::string MakeSettingsText(const std::vector<ModelReducer::Level>& levels)
		{
			std::ostringstream ss;
			ss << "version " << SETTINGS_VERSION << "\n";
			for (const auto& level : levels) {
				ss << "ratio " << level.ratio << " maxError " << level.maxError << " prune " << (level.prune ? 1 : 0) << "\n";
			}
			return ss.str();
		}

		// 古い tkm（3ds Max から出したもの）は 16 ビットのインデックスのことがあるので、32 ビットにそろえる。
		void ConvertIndexTo32(TkmFile::SMesh& mesh)
		{
			if (!mesh.indexBuffer32Array.empty() || mesh.indexBuffer16Array.empty()) {
				return;
			}
			for (const auto& ib16 : mesh.indexBuffer16Array) {
				TkmFile::SIndexBuffer32 ib32;
				ib32.indices.assign(ib16.indices.begin(), ib16.indices.end());
				mesh.indexBuffer32Array.push_back(std::move(ib32));
			}
			mesh.indexBuffer16Array.clear();
		}

		size_t CountTriangles(const std::vector<TkmFile::SMesh>& parts)
		{
			size_t n = 0;
			for (const auto& mesh : parts) {
				for (const auto& ib : mesh.indexBuffer32Array) {
					n += ib.indices.size() / 3;
				}
			}
			return n;
		}
	}

	std::vector<std::string> ModelReducer::ResolveLodTkmFilePaths(const char* filePath, const std::vector<Level>& levels)
	{
		std::vector<std::string> lodPaths;
		const std::string basePath = FbxRuntimeImporter::ResolveToTkmFilePath(filePath);
		const std::string stem = basePath.substr(0, basePath.find_last_of('.'));
		for (size_t i = 0; i < levels.size(); i++) {
			lodPaths.push_back(stem + "_lod" + std::to_string(i) + ".tkm");
		}
		const std::string settingsPath = stem + "_lod.txt";
		const std::string settingsText = MakeSettingsText(levels);

		// 作り直しが要るかを調べる。
		ULONGLONG baseTime = 0;
		if (!GetWriteTime(basePath, baseTime)) {
			return {};
		}
		bool isNeeded = ReadText(settingsPath) != settingsText;
		for (const auto& path : lodPaths) {
			ULONGLONG lodTime = 0;
			if (!GetWriteTime(path, lodTime) || lodTime < baseTime) {
				isNeeded = true;
			}
		}
		if (!isNeeded) {
			return lodPaths;
		}

		// もとの tkm を読み（テクスチャは読まない）、LOD を作って保存する。
		TkmFile baseTkm;
		if (!baseTkm.Load(basePath.c_str(), false, false, true)) {
			return {};
		}
		std::vector<TkmFile::SMesh> parts = baseTkm.GetMeshParts();
		for (auto& mesh : parts) {
			ConvertIndexTo32(mesh);
		}
		auto lods = LodBuilder::Build(parts, levels);
		for (size_t i = 0; i < lods.size(); i++) {
			const size_t numTriangle = CountTriangles(lods[i]);
			TkmFile lodTkm;
			lodTkm.SetMeshPartsForRuntimeImport(std::move(lods[i]));
			if (!lodTkm.Save(lodPaths[i].c_str())) {
				return {};
			}
			char msg[512];
			sprintf_s(msg, "[ModelReducer] %s: %zu 三角形\n", lodPaths[i].c_str(), numTriangle);
			OutputDebugStringA(msg);
		}
		std::ofstream(settingsPath) << settingsText;
		return lodPaths;
	}
}
