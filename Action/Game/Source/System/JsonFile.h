#pragma once

// json.hpp は約2.5万行あるため stdafx.h には入れない。JSON を使う .cpp だけでこのヘッダーを読み込む。
#include <nlohmann/json.hpp>

// JSON ファイルを読み込む係。
// 使い方:
//   #include "System/JsonFile.h"
//   auto j = JsonFile::Load("player.json");     // Assets/data/player.json を読む
//   float speed = j.value("runSpeed", 400.0f);  // キーがなければ 400
namespace JsonFile
{
	using Json = nlohmann::json;

	// JSON の置き場所。Load にこのフォルダーから数えたパスを渡せる。
	constexpr const char* DATA_DIR = "Assets/data/";

	// path が "Assets/" で始まればそのまま、そうでなければ DATA_DIR から数えたパスとして読む。
	// ファイルがない・書き方が壊れているときは、デバッグ出力に理由を出して空のオブジェクトを返す（落ちない）。
	// 空のオブジェクトでも value() は既定値を返すので、呼ぶ側で失敗を気にしなくてよい。
	Json Load(const std::string& path);

	// 読み込みに成功したかを知りたいとき用。成功なら true を返し out に入れる。失敗なら out は空のオブジェクト。
	bool TryLoad(const std::string& path, Json& out);
}
