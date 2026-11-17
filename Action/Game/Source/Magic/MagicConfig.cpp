#include "stdafx.h"
#include "MagicConfig.h"
#include "System/JsonFile.h"

// 魔法の調整値を Assets/data/magic.json から読んで、魔法ごとの構造体に詰める。
// 数値の意味・決めた理由のコメントは magic.json の方に書いてある。
// ここに書いてある値は、JSON にキーがないときの既定値（magic.json を作ったときと同じ値）。
// JSON のキー名や入れ子の形を知っているのは、このファイルだけにする（魔法は JSON を知らない）。

namespace
{
	using Json = JsonFile::Json;

	constexpr const char* JSON_PATH = "magic.json";

	// magic.json は最初に呼ばれたときに1回だけ読み、あとは取っておいたものを使う。
	const Json& GetJson()
	{
		static const Json json = JsonFile::Load(JSON_PATH);
		return json;
	}

	void Log(const std::string& message)
	{
		const std::string line = "[MagicConfig] " + message + "\n";
		OutputDebugStringA(line.c_str());
	}

	// 入れ子の1段（"ice" など）を取り出す。ないときは空の {} を返すので、中のキーはどれも既定値になる。
	Json GetSection(const char* name)
	{
		const Json& json = GetJson();
		auto it = json.find(name);
		if (it == json.end() || !it->is_object()) {
			Log(std::string(JSON_PATH) + " に \"" + name + "\" がないので、既定値を使います。");
			return Json::object();
		}
		return *it;
	}

	// section の key を読む。ないとき・数でないときは defaultValue を返し、出力ウィンドウに出す（書き間違いに気づくため）。
	float Read(const Json& section, const char* sectionName, const char* key, float defaultValue)
	{
		auto it = section.find(key);
		if (it == section.end() || !it->is_number()) {
			Log(std::string(JSON_PATH) + " の " + sectionName + "." + key + " がない（または数でない）ので、既定値を使います。");
			return defaultValue;
		}
		return it->get<float>();
	}

	IceMagicConfig LoadIce()
	{
		const Json section = GetSection("ice");
		IceMagicConfig config;
		config.cooldown = Read(section, "ice", "cooldown", 0.35f);
		config.settings.speed = Read(section, "ice", "speed", 1500.0f);
		config.settings.range = Read(section, "ice", "range", 2000.0f);
		config.settings.radius = Read(section, "ice", "radius", 10.0f);
		config.freezeSeconds = Read(section, "ice", "freezeSeconds", 2.5f);
		return config;
	}

	FireMagicConfig LoadFire()
	{
		const Json section = GetSection("fire");
		FireMagicConfig config;
		config.cooldown = Read(section, "fire", "cooldown", 0.6f);
		config.maxChargeSeconds = Read(section, "fire", "maxChargeSeconds", 1.2f);
		config.speed = Read(section, "fire", "speed", 1300.0f);
		config.range = Read(section, "fire", "range", 2000.0f);
		config.minRadius = Read(section, "fire", "minRadius", 14.0f);
		config.maxRadius = Read(section, "fire", "maxRadius", 45.0f);
		config.minGroundRadius = Read(section, "fire", "minGroundRadius", 130.0f);
		config.maxGroundRadius = Read(section, "fire", "maxGroundRadius", 300.0f);
		config.groundBurnSeconds = Read(section, "fire", "groundBurnSeconds", 4.0f);
		return config;
	}

	SlashMagicConfig LoadSlash()
	{
		const Json section = GetSection("slash");
		SlashMagicConfig config;
		config.settings.speed = Read(section, "slash", "speed", 1100.0f);
		config.settings.range = Read(section, "slash", "range", 1500.0f);
		config.settings.radius = Read(section, "slash", "radius", 45.0f);
		return config;
	}
}

namespace MagicConfig
{
	// どれも最初に呼ばれたときに1回だけ作り、あとは取っておいたものを返す。
	const IceMagicConfig& GetIce()
	{
		static const IceMagicConfig config = LoadIce();
		return config;
	}

	const FireMagicConfig& GetFire()
	{
		static const FireMagicConfig config = LoadFire();
		return config;
	}

	const SlashMagicConfig& GetSlash()
	{
		static const SlashMagicConfig config = LoadSlash();
		return config;
	}
}
