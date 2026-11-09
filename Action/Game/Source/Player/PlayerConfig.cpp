#include "stdafx.h"
#include "PlayerConfig.h"
#include "System/JsonFile.h"

// プレイヤーの調整値を Assets/data/player.json から読んで、各部品の設定の構造体に詰める。
// 数値の意味・決めた理由のコメントは player.json の方に書いてある。
// ここに書いてある値は、JSON にキーがないときの既定値（player.json を作ったときと同じ値）。
// JSON のキー名や入れ子の形を知っているのは、このファイルだけにする（Player は JSON を知らない）。

namespace
{
	using Json = JsonFile::Json;

	constexpr const char* JSON_PATH = "player.json";
	// アニメーションのコマ数を秒にするときの値。クリップはどれも 30fps で作っている。
	constexpr float ANIM_FPS = 30.0f;

	// player.json は最初に呼ばれたときに1回だけ読み、あとは取っておいたものを使う。
	const Json& GetJson()
	{
		static const Json json = JsonFile::Load(JSON_PATH);
		return json;
	}

	void Log(const std::string& message)
	{
		const std::string line = "[PlayerConfig] " + message + "\n";
		OutputDebugStringA(line.c_str());
	}

	// 入れ子の1段（"move" など）を取り出す。ないときは空の {} を返すので、中のキーはどれも既定値になる。
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

	// section の key を読む。ないとき・型が違うときは defaultValue を返し、出力ウィンドウに出す（書き間違いに気づくため）。
	template <class T>
	T Read(const Json& section, const char* sectionName, const char* key, T defaultValue)
	{
		auto it = section.find(key);
		if (it == section.end()) {
			Log(std::string(JSON_PATH) + " に " + sectionName + "." + key + " がないので、既定値を使います。");
			return defaultValue;
		}
		// 数値のところに文字列が書いてあるなど、型が違うときも既定値にする（落とさない）。
		try {
			return it->get<T>();
		}
		catch (const nlohmann::json::exception&) {
			Log(std::string(JSON_PATH) + " の " + sectionName + "." + key + " の型が違うので、既定値を使います。");
			return defaultValue;
		}
	}

	// コマ数で書いてある値を読み、秒にして返す。
	float ReadFramesAsSeconds(const Json& section, const char* sectionName, const char* key, float defaultFrames)
	{
		return Read(section, sectionName, key, defaultFrames) / ANIM_FPS;
	}

	// [x, y, z] の形の値を Vector3 で読む。
	Vector3 ReadVector3(const Json& section, const char* sectionName, const char* key, const Vector3& defaultValue)
	{
		const std::vector<float> values = Read(section, sectionName, key, std::vector<float>{});
		if (values.size() != 3) {
			if (!values.empty()) {
				Log(std::string(JSON_PATH) + " の " + sectionName + "." + key + " は [x, y, z] の3つで書いてください。既定値を使います。");
			}
			return defaultValue;
		}
		return Vector3(values[0], values[1], values[2]);
	}

	CharacterMoveSettings MakeMoveSettings()
	{
		const Json section = GetSection("move");
		CharacterMoveSettings settings;
		settings.walkSpeed = Read(section, "move", "walkSpeed", 170.0f);
		// CharacterMover は「歩く速さの何倍で走るか」で持つので、走る速さ ÷ 歩く速さにする。
		const float runSpeed = Read(section, "move", "runSpeed", 480.0f);
		settings.runSpeedMultiplier = settings.walkSpeed > 0.0f ? runSpeed / settings.walkSpeed : 1.0f;
		settings.jumpSpeed = Read(section, "move", "jumpSpeed", 400.0f);
		settings.gravity = Read(section, "move", "gravity", 980.0f);
		settings.capsuleRadius = Read(section, "move", "capsuleRadius", 20.0f);
		settings.capsuleHeight = Read(section, "move", "capsuleHeight", 100.0f);
		settings.turnSpeedDeg = Read(section, "move", "turnSpeedDeg", 2160.0f);
		settings.turnInPlaceAngleDeg = Read(section, "move", "turnInPlaceAngleDeg", 45.0f);
		settings.acceleration = Read(section, "move", "acceleration", 2400.0f);
		settings.deceleration = Read(section, "move", "deceleration", 3000.0f);
		return settings;
	}

	// ダッシュの数値を settings に入れる。
	void ReadDashSettings(PlayerStateSettings& settings)
	{
		const Json section = GetSection("dash");
		settings.dashDistance = Read(section, "dash", "distance", 390.0f);
		// distance を、蹴って低く滑る区間（kickFrames コマ）で進む速さにする。
		const float kickSeconds = ReadFramesAsSeconds(section, "dash", "kickFrames", 7.0f);
		settings.dashSpeed = kickSeconds > 0.0f ? settings.dashDistance / kickSeconds : 0.0f;
		settings.dashEndSpeed = Read(section, "dash", "endSpeed", 480.0f);
		settings.dashAnimSpeed = Read(section, "dash", "animSpeed", 1.0f);
		settings.dashAnimBlendSeconds = Read(section, "dash", "animBlendSeconds", 0.05f);
		settings.dashCrouchSeconds = ReadFramesAsSeconds(section, "dash", "crouchFrames", 4.0f);
		settings.dashCrouchBlendSeconds = Read(section, "dash", "crouchBlendSeconds", 0.1f);
		settings.dashCrouchSpeed = Read(section, "dash", "crouchSpeed", 200.0f);
		settings.dashKickStartTime = ReadFramesAsSeconds(section, "dash", "kickStartFrame", 4.0f);
		settings.dashLandSeconds = ReadFramesAsSeconds(section, "dash", "landFrames", 5.0f);
		settings.dashExitBlendSeconds = Read(section, "dash", "exitBlendSeconds", 0.15f);
		settings.dashToRunStartTime = ReadFramesAsSeconds(section, "dash", "toRunStartFrame", 5.0f);
		settings.dashToWalkStartTime = ReadFramesAsSeconds(section, "dash", "toWalkStartFrame", 0.0f);
		settings.dashLandIntoRun = Read(section, "dash", "landIntoRun", true);
		settings.dashCanChain = Read(section, "dash", "canChain", false);
		settings.dashEndWithStop = Read(section, "dash", "endWithStop", true);
		settings.dashKeepRun = Read(section, "dash", "keepRun", true);
	}

	// 走りから止まる数値を settings に入れる。
	void ReadStopSettings(PlayerStateSettings& settings)
	{
		const Json section = GetSection("stop");
		settings.stopSeconds = ReadFramesAsSeconds(section, "stop", "frames", 30.0f);
		settings.stopSlideSeconds = ReadFramesAsSeconds(section, "stop", "slideFrames", 14.0f);
		settings.stopBlendSeconds = Read(section, "stop", "blendSeconds", 0.1f);
		settings.stopExitBlendSeconds = Read(section, "stop", "exitBlendSeconds", 0.3f);
		settings.stopAfterRunSeconds = Read(section, "stop", "afterRunSeconds", 0.2f);
		settings.runAnimSeconds = ReadFramesAsSeconds(section, "stop", "runAnimFrames", 14.0f);
		settings.stopLRunTime = ReadFramesAsSeconds(section, "stop", "stopLRunFrame", 9.0f);
		settings.stopRRunTime = ReadFramesAsSeconds(section, "stop", "stopRRunFrame", 2.0f);
	}

	PlayerStateSettings BuildStateSettings()
	{
		PlayerStateSettings settings;
		ReadDashSettings(settings);
		ReadStopSettings(settings);

		const Json move = GetSection("move");
		settings.moveThreshold = Read(move, "move", "moveThreshold", 0.1f);

		const Json animation = GetSection("animation");
		settings.animBlendSeconds = Read(animation, "animation", "blendSeconds", 0.2f);
		settings.walkAnimMoveSpeed = Read(animation, "animation", "walkMoveSpeed", 130.0f);
		return settings;
	}

	// JSON のクリップの名前と、クリップの番号・既定値の対応表。番号の順に並べる。
	struct ClipEntry
	{
		const char* name;
		EnPlayerAnimClip clip;
		AnimClipDesc defaultDesc;
	};
	const ClipEntry CLIP_TABLE[] = {
		{ "Idle",  enPlayerAnimClip_Idle,  { "Assets/modelData/player/Animation/Idle.fbx",  true } },
		{ "Walk",  enPlayerAnimClip_Walk,  { "Assets/modelData/player/Animation/Walk.fbx",  true } },
		{ "Run",   enPlayerAnimClip_Run,   { "Assets/modelData/player/Animation/Run.fbx",   true } },
		{ "Jump",  enPlayerAnimClip_Jump,  { "Assets/modelData/player/Animation/Run.fbx",   true } },
		{ "Dash",  enPlayerAnimClip_Dash,  { "Assets/modelData/player/Animation/Dash.fbx",  false } },
		{ "StopL", enPlayerAnimClip_StopL, { "Assets/modelData/player/Animation/StopL.fbx", false } },
		{ "StopR", enPlayerAnimClip_StopR, { "Assets/modelData/player/Animation/StopR.fbx", false } },
	};
	static_assert(sizeof(CLIP_TABLE) / sizeof(CLIP_TABLE[0]) == enPlayerAnimClip_Num,
		"EnPlayerAnimClip を足したら、CLIP_TABLE と player.json の animation.clips にも足してください。");

	// "Z" / "Y" を EnModelUpAxis にする。
	EnModelUpAxis ReadUpAxis(const Json& section, EnModelUpAxis defaultValue)
	{
		const std::string defaultName = defaultValue == enModelUpAxisZ ? "Z" : "Y";
		const std::string name = Read(section, "model", "upAxis", defaultName);
		if (name == "Z") {
			return enModelUpAxisZ;
		}
		if (name == "Y") {
			return enModelUpAxisY;
		}
		Log(std::string(JSON_PATH) + " の model.upAxis は \"Y\" か \"Z\" で書いてください。既定値を使います。");
		return defaultValue;
	}

	std::vector<AnimClipDesc> MakeClips()
	{
		const Json animation = GetSection("animation");
		const Json clips = Read(animation, "animation", "clips", Json::object());

		std::vector<AnimClipDesc> result(enPlayerAnimClip_Num);
		for (const ClipEntry& entry : CLIP_TABLE) {
			AnimClipDesc desc = entry.defaultDesc;
			auto it = clips.find(entry.name);
			if (it == clips.end() || !it->is_object()) {
				Log(std::string(JSON_PATH) + " の animation.clips に \"" + entry.name + "\" がないので、既定のパスを使います。");
			}
			else {
				const std::string sectionName = std::string("animation.clips.") + entry.name;
				desc.path = Read(*it, sectionName.c_str(), "path", desc.path);
				desc.isLoop = Read(*it, sectionName.c_str(), "loop", desc.isLoop);
			}
			result[entry.clip] = desc;
		}

		// 対応表にない名前（書き間違いなど）は使われないので、気づけるように出す。
		for (auto it = clips.begin(); it != clips.end(); ++it) {
			bool known = false;
			for (const ClipEntry& entry : CLIP_TABLE) {
				if (it.key() == entry.name) {
					known = true;
					break;
				}
			}
			if (!known) {
				Log(std::string(JSON_PATH) + " の animation.clips の \"" + it.key() + "\" は、対応するクリップがないので使いません。");
			}
		}
		return result;
	}

	CharacterModelDesc MakeModelDesc()
	{
		const Json section = GetSection("model");
		CharacterModelDesc desc;
		desc.modelPath = Read(section, "model", "path", std::string("Assets/modelData/player/Model/SK_Player.FBX"));
		desc.upAxis = ReadUpAxis(section, enModelUpAxisZ);
		desc.scale = Read(section, "model", "scale", 1.128f);
		desc.clips = MakeClips();
		return desc;
	}

	MagicCaster::Settings MakeCasterSettings()
	{
		const Json section = GetSection("caster");
		MagicCaster::Settings settings;
		settings.castHeight = Read(section, "caster", "height", 140.0f);
		settings.castForward = Read(section, "caster", "forward", 60.0f);
		return settings;
	}

	BodyLeanSettings MakeLeanSettings()
	{
		const Json section = GetSection("lean");
		BodyLeanSettings settings;
		settings.isEnabled = Read(section, "lean", "enabled", true);
		settings.accelLeanScale = Read(section, "lean", "accelLeanScale", 0.12f);
		settings.maxAccelLeanDeg = Read(section, "lean", "maxAccelLeanDeg", 8.0f);
		settings.velocitySmoothSeconds = Read(section, "lean", "velocitySmoothSeconds", 0.15f);
		settings.leanSmoothSeconds = Read(section, "lean", "leanSmoothSeconds", 0.1f);
		return settings;
	}

	SpringBonesSettings MakeSpringBonesSettings()
	{
		const Json section = GetSection("springBone");
		SpringBonesSettings settings;
		settings.isEnabled = Read(section, "springBone", "enabled", true);
		settings.legRadius = Read(section, "springBone", "legRadius", 8.0f);
		auto groups = section.find("groups");
		if (groups == section.end() || !groups->is_object()) {
			Log(std::string(JSON_PATH) + " に springBone.groups がないので、揺れ物を使いません。");
			settings.isEnabled = false;
			return settings;
		}
		for (auto it = groups->begin(); it != groups->end(); ++it) {
			const std::string sectionName = "springBone.groups." + it.key();
			const Json& group = it.value();
			SpringBoneGroupSettings groupSettings;
			const std::string match = Read(group, sectionName.c_str(), "match", std::string());
			groupSettings.match = std::wstring(match.begin(), match.end());
			groupSettings.frequency = Read(group, sectionName.c_str(), "frequency", 3.0f);
			groupSettings.dampingRatio = Read(group, sectionName.c_str(), "dampingRatio", 0.5f);
			groupSettings.maxAngleDeg = Read(group, sectionName.c_str(), "maxAngleDeg", 30.0f);
			settings.groups.push_back(groupSettings);
		}
		return settings;
	}
}

CharacterConfig PlayerConfig::MakeCharacterConfig()
{
	CharacterConfig config;
	config.move = MakeMoveSettings();
	config.model = MakeModelDesc();
	config.caster = MakeCasterSettings();
	config.lean = MakeLeanSettings();
	config.springBones = MakeSpringBonesSettings();

	const Json hit = GetSection("hit");
	config.hitRadius = Read(hit, "hit", "radius", 45.0f);
	config.hitCenterHeight = Read(hit, "hit", "centerHeight", 70.0f);

	config.maxHp = Read(GetJson(), "(先頭)", "maxHp", 100.0f);
	return config;
}

PlayerStateSettings PlayerConfig::MakeStateSettings()
{
	return BuildStateSettings();
}

WeaponAttachmentDesc PlayerConfig::MakeSwordDesc()
{
	const Json section = GetSection("sword");
	WeaponAttachmentDesc desc;
	desc.modelPath = Read(section, "sword", "path", std::string("Assets/modelData/weapon/sowrd/sowrd.tkm"));
	// 右手ボーンの名前はモデルのセットアップによって異なるので、2つの名前で探す。
	// ボーンの名前はモデルの作りで決まるので、JSON には出さない。
	desc.boneNames = { L"Character1_RightHand", L"J_R_ForeArm_00_tw" };
	desc.offset = ReadVector3(section, "sword", "offset", Vector3(5.0f, -3.0f, 10.0f));
	desc.rotationOffsetDeg = ReadVector3(section, "sword", "rotationOffsetDeg", Vector3(0.0f, 0.0f, -90.0f));
	desc.scale = ReadVector3(section, "sword", "scale", Vector3(1.35f, 1.35f, 1.35f));
	desc.isShadowCaster = Read(section, "sword", "castShadow", true);
	return desc;
}
