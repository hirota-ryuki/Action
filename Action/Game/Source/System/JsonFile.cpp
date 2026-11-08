#include "stdafx.h"
#include "JsonFile.h"

#include <fstream>

namespace JsonFile
{
	namespace
	{
		std::string ResolvePath(const std::string& path)
		{
			if (path.rfind("Assets/", 0) == 0 || path.rfind("Assets\\", 0) == 0) {
				return path;
			}
			return std::string(DATA_DIR) + path;
		}

		void LogError(const std::string& message)
		{
			const std::string line = "[JsonFile] " + message + "\n";
			OutputDebugStringA(line.c_str());
		}
	}

	bool TryLoad(const std::string& path, Json& out)
	{
		out = Json::object();

		const std::string fullPath = ResolvePath(path);
		std::ifstream file(fullPath);
		if (!file) {
			LogError("ファイルを開けません: " + fullPath);
			return false;
		}

		// 例外を使わずに読む。書き方が壊れていると is_discarded() が true になる。
		Json parsed = Json::parse(file, nullptr, false, true /* コメントを許す */);
		if (parsed.is_discarded()) {
			LogError("JSON の書き方が壊れています: " + fullPath);
			return false;
		}

		out = std::move(parsed);
		return true;
	}

	Json Load(const std::string& path)
	{
		Json result;
		TryLoad(path, result);
		return result;
	}
}
