#include "stdafx.h"
#include "JsonFile.h"

#include <fstream>

// JSON ファイルを読む係。設計は docs/ゲーム/システム/JSON読み込みの設計.md を参照。
// 中身の解釈は nlohmann/json に任せ、ここではパスの決め方・ファイルを開く・失敗したときの扱いだけを受け持つ。
namespace JsonFile
{
	// このファイルの中だけで使う関数。
	namespace
	{
		// 呼ぶ側が渡したパスを、実際に開くパスにする。
		// "Assets/"（または "Assets\"）で始まるならそのまま使う。
		// それ以外は Assets/data/ から数えたパスとみなし、DATA_DIR を頭に付ける。
		// 例: "player.json" → "Assets/data/player.json"
		std::string ResolvePath(const std::string& path)
		{
			// rfind(…, 0) は「先頭から始まっているか」を調べる書き方。
			if (path.rfind("Assets/", 0) == 0 || path.rfind("Assets\\", 0) == 0) {
				return path;
			}
			return std::string(DATA_DIR) + path;
		}

		// 失敗した理由を Visual Studio の出力ウィンドウに出す。
		// 頭に [JsonFile] を付けて、ほかのログと見分けられるようにする。
		void LogError(const std::string& message)
		{
			const std::string line = "[JsonFile] " + message + "\n";
			OutputDebugStringA(line.c_str());
		}
	}

	bool TryLoad(const std::string& path, Json& out)
	{
		// 先に空のオブジェクトを入れておく。途中で失敗して return しても、out は空の {} になる。
		// null ではなく {} にするのは、null に value() を使うと例外になるが、{} なら既定値が返るため。
		out = Json::object();

		// ファイルを開く。ないとき・開けないときはここで失敗にする。
		const std::string fullPath = ResolvePath(path);
		std::ifstream file(fullPath);
		if (!file) {
			LogError("ファイルを開けません: " + fullPath);
			return false;
		}

		// 中身を読む。引数の意味は次のとおり。
		//   nullptr : 読みながら値を選り分ける関数（使わない）
		//   false   : 書き方が壊れていても例外を投げない。代わりに is_discarded() が true の値を返す
		//   true    : // と /* */ のコメントを許す（調整値の横に説明を書けるようにする）
		Json parsed = Json::parse(file, nullptr, false, true);
		if (parsed.is_discarded()) {
			LogError("JSON の書き方が壊れています: " + fullPath);
			return false;
		}

		// 読めたときだけ out を差し替える。move で中身をコピーせずに渡す。
		out = std::move(parsed);
		return true;
	}

	Json Load(const std::string& path)
	{
		// 成功・失敗は気にせず、結果だけを返す。
		// 失敗したときは TryLoad が result を空の {} にしているので、呼ぶ側は value() で既定値を取れる。
		Json result;
		TryLoad(path, result);
		return result;
	}
}
