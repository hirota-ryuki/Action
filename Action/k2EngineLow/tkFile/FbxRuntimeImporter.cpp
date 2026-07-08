#include "k2EngineLowPreCompile.h"
#include "tkFile/FbxRuntimeImporter.h"
#include <cctype>

namespace nsK2EngineLow {
	namespace {
		std::string ToLower(std::string s)
		{
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
			return s;
		}
		std::string ToStdString(const ufbx_string& str)
		{
			return std::string(str.data, str.length);
		}
		std::string GetFileName(const std::string& path)
		{
			auto pos = path.find_last_of("/\\");
			if (pos == std::string::npos) {
				return path;
			}
			return path.substr(pos + 1);
		}
		std::string GetDirectory(const std::string& path)
		{
			auto pos = path.find_last_of("/\\");
			if (pos == std::string::npos) {
				return "";
			}
			return path.substr(0, pos);
		}
		std::string GetExtension(const std::string& path)
		{
			std::string fileName = GetFileName(path);
			auto pos = fileName.find_last_of('.');
			if (pos == std::string::npos) {
				return "";
			}
			return fileName.substr(pos);
		}
		std::string GetStem(const std::string& path)
		{
			std::string fileName = GetFileName(path);
			auto pos = fileName.find_last_of('.');
			if (pos == std::string::npos) {
				return fileName;
			}
			return fileName.substr(0, pos);
		}
		std::string JoinPath(const std::string& dir, const std::string& name)
		{
			if (dir.empty()) {
				return name;
			}
			char last = dir.back();
			if (last == '\\' || last == '/') {
				return dir + name;
			}
			// TkmFile::BuildMaterial側は最後の'/'(無ければ'\\')でファイル名部分を置換するため、
			// '\\'と'/'が混在すると区切り位置がずれる。区切り文字は常に'/'に統一する。
			return dir + "/" + name;
		}
		bool FileExists(const std::string& path)
		{
			DWORD attrs = GetFileAttributesA(path.c_str());
			return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
		}
		bool GetLastWriteTime(const std::string& path, FILETIME& outTime)
		{
			WIN32_FILE_ATTRIBUTE_DATA data;
			if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data)) {
				return false;
			}
			outTime = data.ftLastWriteTime;
			return true;
		}
		std::string GetAbsolutePath(const std::string& path)
		{
			char buf[MAX_PATH];
			DWORD len = GetFullPathNameA(path.c_str(), MAX_PATH, buf, nullptr);
			if (len == 0 || len >= MAX_PATH) {
				return path;
			}
			return std::string(buf, len);
		}
		// テクスチャを探すディレクトリの候補一覧。
		// 「Model\Textures」のようにfbxと同じフォルダ内のTexturesサブフォルダを優先し、
		// 見つからなければfbx自体のフォルダも探す。
		std::vector<std::string> GetTextureSearchDirs(const std::string& fbxDirectory)
		{
			return { JoinPath(fbxDirectory, "Textures"), fbxDirectory };
		}
		// ディレクトリ内をファイル名(大文字小文字無視)で検索し、見つかったフルパスを返す。見つからなければ空文字。
		std::string FindFileCaseInsensitive(const std::string& directory, const std::string& wantedFileNameLower)
		{
			std::string result;
			std::string searchPattern = JoinPath(directory, "*");
			WIN32_FIND_DATAA findData;
			HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);
			if (hFind == INVALID_HANDLE_VALUE) {
				return result;
			}
			do {
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}
				if (ToLower(findData.cFileName) == wantedFileNameLower) {
					result = JoinPath(directory, findData.cFileName);
					break;
				}
			} while (FindNextFileA(hFind, &findData));
			FindClose(hFind);
			return result;
		}
	}

	void FbxRuntimeImporter::ReportError(const std::string& message, bool isOutputErrorCodeTTY)
	{
		if (isOutputErrorCodeTTY) {
			printf("%s\n", message.c_str());
		}
		else {
			MessageBoxA(nullptr, message.c_str(), "エラー", MB_OK);
		}
	}
	bool FbxRuntimeImporter::IsFbxExtension(const std::string& filePath)
	{
		return ToLower(GetExtension(filePath)) == ".fbx";
	}
	std::string FbxRuntimeImporter::ResolveToTkmFilePath(const char* filePath, bool isOutputErrorCodeTTY)
	{
		std::string path = filePath;
		if (!IsFbxExtension(path)) {
			// tkmファイルパスがそのまま渡された。ここでは何もしない。
			return path;
		}

		std::string tkmPathStr = GetStem(path);
		tkmPathStr = JoinPath(GetDirectory(path), tkmPathStr + ".tkm");

		if (IsConversionNeeded(path, tkmPathStr)) {
			ConvertFbxToTkm(path, tkmPathStr, isOutputErrorCodeTTY);
		}
		return tkmPathStr;
	}
	bool FbxRuntimeImporter::IsConversionNeeded(const std::string& fbxFilePath, const std::string& tkmFilePath)
	{
		if (!FileExists(tkmFilePath)) {
			return true;
		}
		FILETIME fbxTime, tkmTime;
		if (!GetLastWriteTime(fbxFilePath, fbxTime) || !GetLastWriteTime(tkmFilePath, tkmTime)) {
			return true;
		}
		return CompareFileTime(&fbxTime, &tkmTime) > 0;
	}
	bool FbxRuntimeImporter::ConvertFbxToTkm(
		const std::string& fbxFilePath,
		const std::string& tkmFilePath,
		bool isOutputErrorCodeTTY
	) {
		ufbx_load_opts opts = {};
		ufbx_error error;
		ufbx_scene* scene = ufbx_load_file(fbxFilePath.c_str(), &opts, &error);
		if (scene == nullptr) {
			std::string msg = "FBXの読み込みに失敗しました。";
			msg += fbxFilePath;
			msg += " : ";
			msg += error.description.data;
			ReportError(msg, isOutputErrorCodeTTY);
			return false;
		}

		std::string fbxDirectory = GetDirectory(fbxFilePath);
		MaterialTextureTable materialTextureTable = LoadMaterialTextureTable(fbxDirectory);

		std::vector<TkmFile::SMesh> meshParts;
		BuildMeshParts(scene, meshParts, fbxDirectory, materialTextureTable, isOutputErrorCodeTTY);

		ufbx_free_scene(scene);

		if (meshParts.empty()) {
			ReportError("FBXに変換可能なメッシュが見つかりませんでした。" + fbxFilePath, isOutputErrorCodeTTY);
			return false;
		}

		TkmFile tkmFile;
		tkmFile.SetMeshPartsForRuntimeImport(std::move(meshParts));
		return tkmFile.Save(tkmFilePath.c_str());
	}
	void FbxRuntimeImporter::BuildMeshParts(
		ufbx_scene* scene,
		std::vector<TkmFile::SMesh>& outMeshParts,
		const std::string& fbxDirectory,
		const MaterialTextureTable& materialTextureTable,
		bool isOutputErrorCodeTTY
	) {
		outMeshParts.reserve(scene->meshes.count);

		for (size_t meshNo = 0; meshNo < scene->meshes.count; meshNo++) {
			ufbx_mesh* mesh = scene->meshes.data[meshNo];

			if (mesh->instances.count == 0) {
				// どのノードからも参照されていないメッシュ。
				continue;
			}
			ufbx_node* node = mesh->instances.data[0];
			if (mesh->instances.count > 1 && isOutputErrorCodeTTY) {
				printf("警告: メッシュ %s は複数ノードにインスタンス化されていますが、最初のインスタンスのみ変換します。\n", mesh->name.data);
			}
			const ufbx_matrix& geoToWorld = node->geometry_to_world;

			TkmFile::SMesh dstMesh;
			dstMesh.isFlatShading = false;

			size_t numMaterialSlots = mesh->materials.count > 0 ? mesh->materials.count : 1;
			// material_partsはマテリアルごとに1件ずつ対応するはずだが、念のため範囲外アクセスを防ぐ。
			if (numMaterialSlots > mesh->material_parts.count) {
				numMaterialSlots = mesh->material_parts.count;
			}
			size_t maxTriIndices = mesh->max_face_triangles * 3;
			std::vector<uint32_t> triIndexBuf(maxTriIndices > 3 ? maxTriIndices : 3);

			for (size_t matNo = 0; matNo < numMaterialSlots; matNo++) {
				TkmFile::SMaterial dstMat;
				BuildMaterial(
					mesh->materials.count > 0 ? mesh->materials.data[matNo] : nullptr,
					dstMat,
					fbxDirectory,
					materialTextureTable,
					isOutputErrorCodeTTY
				);

				TkmFile::SIndexBuffer32 dstIb;

				const ufbx_mesh_part& part = mesh->material_parts.data[matNo];
				for (size_t f = 0; f < part.face_indices.count; f++) {
					uint32_t faceIx = part.face_indices.data[f];
					ufbx_face face = mesh->faces.data[faceIx];
					uint32_t numTris = ufbx_triangulate_face(triIndexBuf.data(), triIndexBuf.size(), mesh, face);

					for (uint32_t i = 0; i < numTris * 3; i++) {
						uint32_t corner = triIndexBuf[i];

						ufbx_vec3 p = ufbx_get_vertex_vec3(&mesh->vertex_position, corner);
						ufbx_vec3 n = mesh->vertex_normal.exists
							? ufbx_get_vertex_vec3(&mesh->vertex_normal, corner)
							: ufbx_vec3{ 0, 0, 0 };
						ufbx_vec2 uv = mesh->vertex_uv.exists
							? ufbx_get_vertex_vec2(&mesh->vertex_uv, corner)
							: ufbx_vec2{ 0, 0 };

						ufbx_vec3 wp = ufbx_transform_position(&geoToWorld, p);
						ufbx_vec3 wn = ufbx_transform_direction(&geoToWorld, n);

						TkmFile::SVertex v;
						v.pos = Vector3((float)wp.x, (float)wp.y, (float)wp.z);
						v.normal = Vector3((float)wn.x, (float)wn.y, (float)wn.z);
						v.tangent = Vector3::Zero;
						v.binormal = Vector3::Zero;
						v.uv = Vector2((float)uv.x, (float)uv.y);
						v.indices[0] = v.indices[1] = v.indices[2] = v.indices[3] = 0;
						v.skinWeights = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

						uint32_t newIndex = (uint32_t)dstMesh.vertexBuffer.size();
						dstMesh.vertexBuffer.push_back(v);
						dstIb.indices.push_back(newIndex);
					}
				}

				if (dstIb.indices.empty()) {
					// 実際には1枚もポリゴンが割り当てられていないマテリアルスロット。
					// 空のインデックスバッファを渡すと後段の描画処理が範囲外アクセスするため除外する。
					continue;
				}

				dstMesh.materials.push_back(dstMat);
				dstMesh.indexBuffer32Array.push_back(std::move(dstIb));
			}

			if (dstMesh.vertexBuffer.empty()) {
				// このメッシュは結果的に1枚もポリゴンを持たない(装飾用の空メッシュなど)。
				continue;
			}

			outMeshParts.push_back(std::move(dstMesh));
		}
	}
	void FbxRuntimeImporter::BuildMaterial(
		const ufbx_material* srcMat,
		TkmFile::SMaterial& outMat,
		const std::string& fbxDirectory,
		const MaterialTextureTable& materialTextureTable,
		bool isOutputErrorCodeTTY
	) {
		outMat.albedoMapFileName.clear();
		outMat.normalMapFileName.clear();
		outMat.specularMapFileName.clear();
		outMat.reflectionMapFileName.clear();
		outMat.refractionMapFileName.clear();

		if (srcMat != nullptr) {
			// ビルトインのpbr/fbxマップを優先し、見つからなければmtoon/glTF系の
			// シェーダー固有プロパティ名(例: "gltf_tex_diffuse")を走査する。
			const ufbx_texture* albedo = srcMat->pbr.base_color.texture;
			if (albedo == nullptr) albedo = srcMat->fbx.diffuse_color.texture;
			if (albedo == nullptr) albedo = FindTextureByPropertyKeyword(srcMat, { "diffuse", "basecolor", "albedo" }, {});

			const ufbx_texture* normal = srcMat->pbr.normal_map.texture;
			if (normal == nullptr) normal = srcMat->fbx.normal_map.texture;
			if (normal == nullptr) normal = FindTextureByPropertyKeyword(srcMat, { "normal" }, { "mask" });

			const ufbx_texture* specular = srcMat->pbr.specular_color.texture;
			if (specular == nullptr) specular = srcMat->fbx.specular_color.texture;
			if (specular == nullptr) specular = FindTextureByPropertyKeyword(srcMat, { "specular" }, { "mask" });

			std::string materialName = ToStdString(srcMat->name);

			outMat.albedoMapFileName = ResolveAndConvertTexture(albedo, fbxDirectory, isOutputErrorCodeTTY);
			outMat.normalMapFileName = ResolveAndConvertTexture(normal, fbxDirectory, isOutputErrorCodeTTY);
			outMat.specularMapFileName = ResolveAndConvertTexture(specular, fbxDirectory, isOutputErrorCodeTTY);

			// ufbxがFBXから直接テクスチャ接続を辿れない場合(例:mtoon/glTF系のカスタムプロパティが
			// 単なる文字列値として保存されている等)、3ds Max用のMaterialTextureTable.txtを併用する。
			// 「gltf_tex_diffuse2」(背景用)や「MatCapNormal」のような紛らわしい類似プロパティに
			// 誤ってマッチしないよう、まず完全一致のプロパティ名で探し、無ければ緩い部分一致で探す。
			auto findExactInTable = [&](const std::string& exactPropName) -> std::string {
				auto matIt = materialTextureTable.find(materialName);
				if (matIt == materialTextureTable.end()) return "";
				auto propIt = matIt->second.find(exactPropName);
				return propIt != matIt->second.end() ? propIt->second : "";
			};
			if (outMat.albedoMapFileName.empty()) {
				std::string tableAlbedo = findExactInTable("gltf_tex_diffuse");
				if (tableAlbedo.empty()) {
					tableAlbedo = FindTextureNameInTable(materialTextureTable, materialName, { "diffuse" }, { "diffuse2" });
				}
				outMat.albedoMapFileName = ResolveAndConvertTextureByFileName(tableAlbedo, fbxDirectory, isOutputErrorCodeTTY);
			}
			if (outMat.normalMapFileName.empty()) {
				std::string tableNormal = findExactInTable("mtoon_tex_normal");
				if (tableNormal.empty()) {
					tableNormal = FindTextureNameInTable(materialTextureTable, materialName, { "normal" }, { "mask", "gltf", "matcap" });
				}
				outMat.normalMapFileName = ResolveAndConvertTextureByFileName(tableNormal, fbxDirectory, isOutputErrorCodeTTY);
			}
		}

		if (outMat.albedoMapFileName.empty()) {
			// FBXにもMaterialTextureTableにもテクスチャ参照が無い、または参照先が見つからない場合の
			// 最終フォールバック。サフィックス規約に頼らず、フォルダ内の画像ファイルをそのまま採用する。
			std::string fallback = FindFallbackTextureInDirectory(fbxDirectory, isOutputErrorCodeTTY);
			if (!fallback.empty() && ConvertTextureToDDS(fallback, fbxDirectory, isOutputErrorCodeTTY)) {
				outMat.albedoMapFileName = GetFileName(fallback);
			}
		}
	}
	const ufbx_texture* FbxRuntimeImporter::FindTextureByPropertyKeyword(
		const ufbx_material* srcMat,
		const std::vector<std::string>& keywords,
		const std::vector<std::string>& excludeKeywords
	) {
		for (size_t i = 0; i < srcMat->textures.count; i++) {
			const ufbx_material_texture& matTex = srcMat->textures.data[i];
			if (matTex.texture == nullptr) {
				continue;
			}
			std::string propName = ToLower(ToStdString(matTex.shader_prop));
			propName += " ";
			propName += ToLower(ToStdString(matTex.material_prop));

			bool excluded = false;
			for (const auto& ex : excludeKeywords) {
				if (propName.find(ex) != std::string::npos) {
					excluded = true;
					break;
				}
			}
			if (excluded) {
				continue;
			}

			for (const auto& keyword : keywords) {
				if (propName.find(keyword) != std::string::npos) {
					return matTex.texture;
				}
			}
		}
		return nullptr;
	}
	std::string FbxRuntimeImporter::FindTextureNameInTable(
		const MaterialTextureTable& materialTextureTable,
		const std::string& materialName,
		const std::vector<std::string>& keywords,
		const std::vector<std::string>& excludeKeywords
	) {
		auto it = materialTextureTable.find(materialName);
		if (it == materialTextureTable.end()) {
			return "";
		}
		for (const auto& propAndFile : it->second) {
			const std::string& propName = propAndFile.first; // 既に小文字化済み。
			bool excluded = false;
			for (const auto& ex : excludeKeywords) {
				if (propName.find(ex) != std::string::npos) {
					excluded = true;
					break;
				}
			}
			if (excluded) {
				continue;
			}
			for (const auto& keyword : keywords) {
				if (propName.find(keyword) != std::string::npos) {
					return propAndFile.second;
				}
			}
		}
		return "";
	}
	MaterialTextureTable FbxRuntimeImporter::LoadMaterialTextureTable(const std::string& fbxDirectory)
	{
		MaterialTextureTable table;
		std::string tablePath = JoinPath(fbxDirectory, "MaterialTextureTable.txt");
		FILE* fp = fopen(tablePath.c_str(), "r");
		if (fp == nullptr) {
			return table;
		}

		std::string currentMaterial;
		char lineBuf[1024];
		while (fgets(lineBuf, sizeof(lineBuf), fp) != nullptr) {
			std::string line = lineBuf;
			while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
				line.pop_back();
			}

			// "  Material: MI_xxx__Instance_" のような行からマテリアル名を取得。
			auto materialPos = line.find("Material:");
			auto dashPos = line.find("- [");
			if (materialPos != std::string::npos && dashPos == std::string::npos) {
				currentMaterial = line.substr(materialPos + 9);
				// 前後の空白を除去。
				size_t start = currentMaterial.find_first_not_of(" \t");
				size_t end = currentMaterial.find_last_not_of(" \t");
				currentMaterial = (start == std::string::npos) ? "" : currentMaterial.substr(start, end - start + 1);
				continue;
			}

			// "    - [propName] -> textureFileName.png" のような行を解析。
			if (dashPos != std::string::npos && !currentMaterial.empty()) {
				auto propStart = dashPos + 3;
				auto propEnd = line.find(']', propStart);
				auto arrowPos = line.find("->", propEnd == std::string::npos ? propStart : propEnd);
				if (propEnd != std::string::npos && arrowPos != std::string::npos) {
					std::string propName = ToLower(line.substr(propStart, propEnd - propStart));
					std::string texFileName = line.substr(arrowPos + 2);
					size_t start = texFileName.find_first_not_of(" \t");
					size_t end = texFileName.find_last_not_of(" \t");
					texFileName = (start == std::string::npos) ? "" : texFileName.substr(start, end - start + 1);
					if (!texFileName.empty()) {
						table[currentMaterial][propName] = texFileName;
					}
				}
			}
		}
		fclose(fp);
		return table;
	}
	std::string FbxRuntimeImporter::ResolveAndConvertTextureByFileName(
		const std::string& fileName,
		const std::string& fbxDirectory,
		bool isOutputErrorCodeTTY
	) {
		if (fileName.empty()) {
			return "";
		}

		std::string srcTexPath;
		std::string wanted = ToLower(fileName);
		for (const auto& dir : GetTextureSearchDirs(fbxDirectory)) {
			std::string candidate = JoinPath(dir, fileName);
			if (FileExists(candidate)) {
				srcTexPath = candidate;
				break;
			}
			std::string found = FindFileCaseInsensitive(dir, wanted);
			if (!found.empty()) {
				srcTexPath = found;
				break;
			}
		}
		if (srcTexPath.empty()) {
			return "";
		}
		if (!ConvertTextureToDDS(srcTexPath, fbxDirectory, isOutputErrorCodeTTY)) {
			return "";
		}
		return GetFileName(srcTexPath);
	}
	std::string FbxRuntimeImporter::ResolveAndConvertTexture(
		const ufbx_texture* tex,
		const std::string& fbxDirectory,
		bool isOutputErrorCodeTTY
	) {
		if (tex == nullptr) {
			return "";
		}

		std::string refName;
		if (tex->filename.length > 0) {
			refName = GetFileName(ToStdString(tex->filename));
		}
		else if (tex->relative_filename.length > 0) {
			refName = GetFileName(ToStdString(tex->relative_filename));
		}
		if (refName.empty()) {
			return "";
		}

		std::string srcTexPath;
		std::string wanted = ToLower(refName);
		for (const auto& dir : GetTextureSearchDirs(fbxDirectory)) {
			std::string candidate = JoinPath(dir, refName);
			if (FileExists(candidate)) {
				srcTexPath = candidate;
				break;
			}
			// 大文字小文字無視で探す。
			std::string found = FindFileCaseInsensitive(dir, wanted);
			if (!found.empty()) {
				srcTexPath = found;
				break;
			}
		}
		if (srcTexPath.empty()) {
			return "";
		}

		if (!ConvertTextureToDDS(srcTexPath, fbxDirectory, isOutputErrorCodeTTY)) {
			return "";
		}
		return GetFileName(srcTexPath);
	}
	std::string FbxRuntimeImporter::FindFallbackTextureInDirectory(const std::string& fbxDirectory, bool isOutputErrorCodeTTY)
	{
		static const std::vector<std::string> exts = { ".png", ".tga", ".jpg", ".jpeg", ".bmp", ".dds" };

		for (const auto& directory : GetTextureSearchDirs(fbxDirectory)) {
			std::vector<std::string> candidates;

			std::string searchPattern = JoinPath(directory, "*");
			WIN32_FIND_DATAA findData;
			HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						continue;
					}
					std::string ext = ToLower(GetExtension(findData.cFileName));
					if (std::find(exts.begin(), exts.end(), ext) != exts.end()) {
						candidates.push_back(JoinPath(directory, findData.cFileName));
					}
				} while (FindNextFileA(hFind, &findData));
				FindClose(hFind);
			}

			if (candidates.empty()) {
				continue;
			}
			if (candidates.size() > 1 && isOutputErrorCodeTTY) {
				printf("警告: %s 内に複数の画像候補が見つかりました。最初の1つを使用します: %s\n",
					directory.c_str(), candidates[0].c_str());
			}
			return candidates[0];
		}
		return "";
	}
	bool FbxRuntimeImporter::ConvertTextureToDDS(const std::string& srcTexPath, const std::string& outputDir, bool isOutputErrorCodeTTY)
	{
		if (ToLower(GetExtension(srcTexPath)) == ".dds") {
			// 既にDDS。変換不要。
			return true;
		}

		std::string dst = JoinPath(outputDir, GetStem(srcTexPath) + ".dds");
		if (FileExists(dst)) {
			FILETIME srcTime, dstTime;
			if (GetLastWriteTime(srcTexPath, srcTime) && GetLastWriteTime(dst, dstTime)) {
				if (CompareFileTime(&dstTime, &srcTime) >= 0) {
					// キャッシュ有効。
					return true;
				}
			}
		}

		std::string texconvPath = FindTexconvExePath(isOutputErrorCodeTTY);
		if (texconvPath.empty()) {
			return false;
		}

		// texconvは相対パスの入力ファイルを渡すと、出力ファイル名にその相対パスをそのまま
		// 連結してしまい書き込みに失敗するため、入出力とも絶対パスに変換して渡す。
		std::string absSrcTexPath = GetAbsolutePath(srcTexPath);
		std::string absOutputDir = GetAbsolutePath(outputDir);

		// 既存の3dsMaxエクスポータ(tools/3dsMaxScripts/makefile)と同一オプション。
		std::string cmdLine = "\"" + texconvPath + "\""
			" -vflip -f BC7_UNORM -srgb -y"
			" -o \"" + absOutputDir + "\""
			" \"" + absSrcTexPath + "\"";

		STARTUPINFOA si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		std::vector<char> cmdLineBuf(cmdLine.begin(), cmdLine.end());
		cmdLineBuf.push_back('\0');

		BOOL ok = CreateProcessA(
			nullptr, cmdLineBuf.data(), nullptr, nullptr, FALSE,
			CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi
		);
		if (!ok) {
			ReportError("texconv.exeの起動に失敗しました。", isOutputErrorCodeTTY);
			return false;
		}
		WaitForSingleObject(pi.hProcess, 30000);
		DWORD exitCode = 1;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		if (exitCode != 0) {
			ReportError("texconv.exeによるDDS変換に失敗しました: " + srcTexPath, isOutputErrorCodeTTY);
			return false;
		}
		return FileExists(dst);
	}
	std::string FbxRuntimeImporter::FindTexconvExePath(bool isOutputErrorCodeTTY)
	{
		static const char* candidates[] = {
			"texconv.exe",
			"..\\tools\\3dsMaxScripts\\texconv.exe",
			"..\\..\\tools\\3dsMaxScripts\\texconv.exe",
		};
		for (auto* c : candidates) {
			if (FileExists(c)) {
				return GetAbsolutePath(c);
			}
		}
		ReportError("texconv.exeが見つかりません。", isOutputErrorCodeTTY);
		return "";
	}
}
