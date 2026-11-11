#include "stdafx.h"
#include "ItemConfig.h"
#include "System/JsonFile.h"

// アイテムの調整値を Assets/data/item.json から読む。数値の意味は item.json の方に書いてある。
// ここに書いてある値は、JSON にキーがないときの既定値（item.json を作ったときと同じ値）。

namespace
{
	using Json = JsonFile::Json;

	constexpr const char* JSON_PATH = "item.json";

	const Json& GetJson()
	{
		static const Json json = JsonFile::Load(JSON_PATH);
		return json;
	}

	void Log(const std::string& message)
	{
		const std::string line = "[ItemConfig] " + message + "\n";
		OutputDebugStringA(line.c_str());
	}

	// 入れ子の1段（"pickup" など）を取り出す。ないときは空の {} を返すので、中のキーはどれも既定値になる。
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

	// section の key を読む。ないとき・型が違うときは defaultValue を返し、出力ウィンドウに出す。
	template <class T>
	T Read(const Json& section, const char* sectionName, const char* key, T defaultValue)
	{
		auto it = section.find(key);
		if (it == section.end()) {
			Log(std::string(JSON_PATH) + " に " + sectionName + "." + key + " がないので、既定値を使います。");
			return defaultValue;
		}
		try {
			return it->get<T>();
		}
		catch (const nlohmann::json::exception&) {
			Log(std::string(JSON_PATH) + " の " + sectionName + "." + key + " の型が違うので、既定値を使います。");
			return defaultValue;
		}
	}
}

ItemLook ItemConfig::MakePotionLook()
{
	const Json look = GetSection("look");
	const Json potion = GetSection("potion");
	ItemLook result;
	result.modelPath = Read<std::string>(potion, "potion", "modelPath", "Assets/modelData/item/Potion/Potion.fbx");
	result.upAxis = enModelUpAxisY;  // Meshy の FBX は Y が上。
	result.modelScale = Read(potion, "potion", "modelScale", 0.5f);
	result.floatHeight = Read(potion, "potion", "floatHeight", 30.0f);
	result.floatAmplitude = Read(look, "look", "floatAmplitude", 5.0f);
	result.floatSeconds = Read(look, "look", "floatSeconds", 2.0f);
	result.spinDegPerSecond = Read(look, "look", "spinDegPerSecond", 90.0f);
	return result;
}

PlayerItemSettings ItemConfig::MakePlayerItemSettings()
{
	const Json pickup = GetSection("pickup");
	const Json potion = GetSection("potion");
	PlayerItemSettings result;
	result.pickupRadius = Read(pickup, "pickup", "radius", 150.0f);
	result.pickupHoldDelaySeconds = Read(pickup, "pickup", "holdDelaySeconds", 0.3f);
	result.pickupHoldIntervalSeconds = Read(pickup, "pickup", "holdIntervalSeconds", 0.05f);
	result.potionStartCount = Read(potion, "potion", "startCount", 2);
	result.potionHealAmount = Read(potion, "potion", "healAmount", 30.0f);
	result.drinkSeconds = Read(potion, "potion", "drinkSeconds", 0.6f);
	result.drinkMoveSpeedRate = Read(potion, "potion", "drinkMoveSpeedRate", 0.5f);
	return result;
}

std::vector<Vector3> ItemConfig::MakePotionSpawnPositions()
{
	std::vector<Vector3> result;
	const Json& json = GetJson();
	auto it = json.find("potionSpawns");
	if (it == json.end() || !it->is_array()) {
		Log(std::string(JSON_PATH) + " に \"potionSpawns\" がないので、ポーションを置きません。");
		return result;
	}
	for (const Json& point : *it) {
		if (!point.is_array() || point.size() < 2) {
			Log(std::string(JSON_PATH) + " の potionSpawns に [x, z] でない値があるので、飛ばします。");
			continue;
		}
		try {
			result.push_back({ point[0].get<float>(), 0.0f, point[1].get<float>() });
		}
		catch (const nlohmann::json::exception&) {
			Log(std::string(JSON_PATH) + " の potionSpawns に数でない値があるので、飛ばします。");
		}
	}
	return result;
}
