/// <summary>
/// FBXファイルをランタイムでtkmファイルへ変換する機能。
/// </summary>
/// <remarks>
/// フェーズ1: 静的メッシュ+マテリアル+テクスチャの自動割り当てのみに対応。
/// スキン/スケルトン/アニメーションのFBXからの変換は非対応。
/// (.tks/.tkaが別途fbxと同名で用意されていれば、既存の仕組みでそのまま読み込まれる。)
/// テクスチャはfbxと同じフォルダ内の「Textures」サブフォルダを優先的に探し、
/// 見つからなければfbx自体のフォルダも探す。
/// </remarks>
#pragma once

#include "tkFile/TkmFile.h"
#include "ExEngine/ufbx/ufbx.h"

namespace nsK2EngineLow {

	/// <summary>
	/// マテリアル名 -> (プロパティ名(小文字) -> テクスチャファイル名) のテーブル。
	/// 3ds Max用に別途出力される「MaterialTextureTable.txt」を読み込んだ結果。
	/// </summary>
	using MaterialTextureTable = std::map<std::string, std::map<std::string, std::string>>;

	/// <summary>
	/// FBXランタイムインポータ。
	/// </summary>
	class FbxRuntimeImporter : public Noncopyable {
	public:
		/// <summary>
		/// filePathの拡張子が.fbxの場合、必要であればtkmファイルへのランタイム変換を行い、
		/// 後続処理で使用すべきtkmファイルパスを返す。
		/// filePathの拡張子が.fbx以外の場合は何もせずfilePathをそのまま返す。
		/// </summary>
		/// <param name="filePath">呼び出し元が指定したファイルパス(.tkmまたは.fbx)。</param>
		/// <param name="isOutputErrorCodeTTY">エラーをTTYに出力する？falseならメッセージボックス。</param>
		/// <returns>後続処理で使用すべきtkmファイルパス。</returns>
		static std::string ResolveToTkmFilePath(
			const char* filePath,
			bool isOutputErrorCodeTTY = false
		);
	private:
		/// <summary>
		/// .fbx拡張子か判定。
		/// </summary>
		static bool IsFbxExtension(const std::string& filePath);
		/// <summary>
		/// 変換が必要か判定(tkmが存在しない、またはfbxの方が新しい場合はtrue)。
		/// </summary>
		static bool IsConversionNeeded(const std::string& fbxFilePath, const std::string& tkmFilePath);
		/// <summary>
		/// FBX→tkm変換の実処理。
		/// </summary>
		static bool ConvertFbxToTkm(
			const std::string& fbxFilePath,
			const std::string& tkmFilePath,
			bool isOutputErrorCodeTTY
		);
		/// <summary>
		/// ufbxのシーンからメッシュパーツを構築する。
		/// </summary>
		static void BuildMeshParts(
			ufbx_scene* scene,
			std::vector<TkmFile::SMesh>& outMeshParts,
			const std::string& fbxDirectory,
			const MaterialTextureTable& materialTextureTable,
			bool isOutputErrorCodeTTY
		);
		/// <summary>
		/// ufbxのマテリアルからtkmマテリアルを構築する(テクスチャの解決/DDS変換込み)。
		/// </summary>
		static void BuildMaterial(
			const ufbx_material* srcMat,
			TkmFile::SMaterial& outMat,
			const std::string& fbxDirectory,
			const MaterialTextureTable& materialTextureTable,
			bool isOutputErrorCodeTTY
		);
		/// <summary>
		/// テクスチャを解決し、DDSへ変換したうえでtkmに記録すべきファイル名を返す。
		/// </summary>
		static std::string ResolveAndConvertTexture(
			const ufbx_texture* tex,
			const std::string& fbxDirectory,
			bool isOutputErrorCodeTTY
		);
		/// <summary>
		/// ファイル名(拡張子付き、パスなし)を直接指定してテクスチャを解決し、DDSへ変換する。
		/// MaterialTextureTableのようにufbxからは辿れないテクスチャ参照を解決するために使う。
		/// </summary>
		static std::string ResolveAndConvertTextureByFileName(
			const std::string& fileName,
			const std::string& fbxDirectory,
			bool isOutputErrorCodeTTY
		);
		/// <summary>
		/// mtoon/glTF系のようにビルトインのpbr/fbxマップに載らないシェーダー固有プロパティ名
		/// (例: "gltf_tex_diffuse")からテクスチャを探す。keywordsのいずれかを含み、
		/// excludeKeywordsのいずれも含まないプロパティ名を採用する。
		/// </summary>
		static const ufbx_texture* FindTextureByPropertyKeyword(
			const ufbx_material* srcMat,
			const std::vector<std::string>& keywords,
			const std::vector<std::string>& excludeKeywords
		);
		/// <summary>
		/// materialTextureTable内のsrcMatに対応するエントリから、
		/// keywordsのいずれかを含むプロパティ名のテクスチャファイル名を探す。
		/// </summary>
		static std::string FindTextureNameInTable(
			const MaterialTextureTable& materialTextureTable,
			const std::string& materialName,
			const std::vector<std::string>& keywords,
			const std::vector<std::string>& excludeKeywords
		);
		/// <summary>
		/// fbxと同じフォルダにある「MaterialTextureTable.txt」(3ds Max用に別途生成される
		/// マテリアル-テクスチャ対応表)を読み込む。無ければ空のテーブルを返す。
		/// ufbxがFBXから直接テクスチャ接続を辿れない場合のフォールバックとして使う。
		/// </summary>
		static MaterialTextureTable LoadMaterialTextureTable(const std::string& fbxDirectory);
		/// <summary>
		/// fbxのTexturesサブフォルダ/fbx自体のフォルダから画像ファイルを探す(FBXにテクスチャ参照が無い/見つからない場合のフォールバック)。
		/// </summary>
		static std::string FindFallbackTextureInDirectory(const std::string& fbxDirectory, bool isOutputErrorCodeTTY);
		/// <summary>
		/// 画像ファイルをDDSへ変換する(texconv.exeを利用)。既にDDSなら何もしない。
		/// </summary>
		static bool ConvertTextureToDDS(const std::string& srcTexPath, const std::string& outputDir, bool isOutputErrorCodeTTY);
		/// <summary>
		/// texconv.exeの実体を探す。
		/// </summary>
		static std::string FindTexconvExePath(bool isOutputErrorCodeTTY);
		/// <summary>
		/// エラーを報告する(TTYまたはメッセージボックス)。
		/// </summary>
		static void ReportError(const std::string& message, bool isOutputErrorCodeTTY);
	};
}
