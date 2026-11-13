#include "stdafx.h"
#include "MagicSettings.h"
#include "System/JsonFile.h"

// JSON のキー名や入れ子の形を知っているのは、このファイルだけにする（魔法と弾は JSON を知らない）。

namespace
{
	using Json = JsonFile::Json;

	constexpr const char* JSON_PATH = "magic.json";

	void Log(const std::string& message)
	{
		const std::string line = "[MagicSettings] " + message + "\n";
		OutputDebugStringA(line.c_str());
	}

	// 入れ子の1段（"ice" など）を取り出す。ないときは空の {} を返すので、中のキーはどれも既定値になる。
	Json GetSection(const Json& json, const char* name)
	{
		auto it = json.find(name);
		if (it == json.end() || !it->is_object()) {
			Log(std::string(JSON_PATH) + " に \"" + name + "\" がないので、既定値を使います。");
			return Json::object();
		}
		return *it;
	}

	// section の key を読み、value に入れる。ないとき・型が違うときは value をそのままにし、出力ウィンドウに出す。
	void Read(const Json& section, const char* sectionName, const char* key, float& value)
	{
		auto it = section.find(key);
		if (it == section.end() || !it->is_number()) {
			Log(std::string(JSON_PATH) + " の " + sectionName + "." + key + " がない（または数値でない）ので、既定値を使います。");
			return;
		}
		value = it->get<float>();
	}

	IceMagicSettings LoadIce(const Json& json)
	{
		const Json section = GetSection(json, "ice");
		IceMagicSettings settings;
		Read(section, "ice", "cooldown", settings.cooldown);
		Read(section, "ice", "speed", settings.speed);
		Read(section, "ice", "range", settings.range);
		Read(section, "ice", "radius", settings.radius);
		Read(section, "ice", "freezeSeconds", settings.freezeSeconds);
		return settings;
	}

	FireMagicSettings LoadFire(const Json& json)
	{
		const Json section = GetSection(json, "fire");
		FireMagicSettings settings;
		Read(section, "fire", "cooldown", settings.cooldown);
		Read(section, "fire", "maxChargeSeconds", settings.maxChargeSeconds);
		Read(section, "fire", "speed", settings.speed);
		Read(section, "fire", "range", settings.range);
		Read(section, "fire", "minRadius", settings.minRadius);
		Read(section, "fire", "maxRadius", settings.maxRadius);
		Read(section, "fire", "minGroundRadius", settings.minGroundRadius);
		Read(section, "fire", "maxGroundRadius", settings.maxGroundRadius);
		Read(section, "fire", "groundBurnSeconds", settings.groundBurnSeconds);
		return settings;
	}

	const Json& GetJson()
	{
		static const Json json = JsonFile::Load(JSON_PATH);
		return json;
	}
}

namespace MagicSettings
{
	const IceMagicSettings& GetIce()
	{
		static const IceMagicSettings settings = LoadIce(GetJson());
		return settings;
	}

	const FireMagicSettings& GetFire()
	{
		static const FireMagicSettings settings = LoadFire(GetJson());
		return settings;
	}
}
