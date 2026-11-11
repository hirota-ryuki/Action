#include "stdafx.h"
#include "EnemyPoseKeys.h"
#include "System/JsonFile.h"
#include <fstream>

namespace
{
	struct BoneInfo
	{
		const char* name;
		const char* label;
	};
	constexpr BoneInfo BONES[EnemyPoseKeys::enBone_Num] = {
		{ "J_Bip_L_Shoulder", u8"左肩" },
		{ "J_Bip_L_UpperArm", u8"左の上腕（肩の関節）" },
		{ "J_Bip_L_LowerArm", u8"左の前腕（ひじ）" },
		{ "J_Bip_R_Shoulder", u8"右肩" },
		{ "J_Bip_R_UpperArm", u8"右の上腕（肩の関節）" },
		{ "J_Bip_R_LowerArm", u8"右の前腕（ひじ）" },
		{ "J_Bip_L_UpperLeg", u8"左の太もも（足の付け根）" },
		{ "J_Bip_L_LowerLeg", u8"左のすね（ひざ）" },
		{ "J_Bip_R_UpperLeg", u8"右の太もも（足の付け根）" },
		{ "J_Bip_R_LowerLeg", u8"右のすね（ひざ）" },
	};

	const std::vector<EnemyPoseKeys::Key> NO_KEYS;

	bool IsZero(const Vector3& v)
	{
		return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
	}

	JsonFile::Json ToJson(const Vector3& v)
	{
		return JsonFile::Json::array({ v.x, v.y, v.z });
	}

	Vector3 FromJson(const JsonFile::Json& j)
	{
		if (!j.is_array() || j.size() != 3) {
			return Vector3::Zero;
		}
		return Vector3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
	}

	Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
	{
		return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
	}

	EnemyPoseKeys::Pose LerpPose(const EnemyPoseKeys::Pose& a, const EnemyPoseKeys::Pose& b, float t)
	{
		EnemyPoseKeys::Pose out;
		for (int i = 0; i < EnemyPoseKeys::enBone_Num; i++) {
			out.boneDeg[i] = Lerp(a.boneDeg[i], b.boneDeg[i], t);
		}
		out.swordDeg = Lerp(a.swordDeg, b.swordDeg, t);
		out.swordPos = Lerp(a.swordPos, b.swordPos, t);
		return out;
	}
}

const char* EnemyPoseKeys::GetBoneName(int bone)
{
	return BONES[bone].name;
}

const char* EnemyPoseKeys::GetBoneLabel(int bone)
{
	return BONES[bone].label;
}

bool EnemyPoseKeys::Pose::IsZero() const
{
	for (const auto& deg : boneDeg) {
		if (!::IsZero(deg)) {
			return false;
		}
	}
	return ::IsZero(swordDeg) && ::IsZero(swordPos);
}

void EnemyPoseKeys::Load(const char* path)
{
	m_clips.clear();
	JsonFile::Json json;
	if (!JsonFile::TryLoad(path, json)) {
		return;
	}
	auto clips = json.find("clips");
	if (clips == json.end() || !clips->is_object()) {
		return;
	}
	for (auto it = clips->begin(); it != clips->end(); ++it) {
		std::vector<Key>& keys = m_clips[it.key()];
		for (const auto& jk : it.value()) {
			Key key;
			key.frame = jk.value("frame", 1);
			auto bones = jk.find("bones");
			if (bones != jk.end() && bones->is_object()) {
				for (int i = 0; i < enBone_Num; i++) {
					auto b = bones->find(BONES[i].name);
					if (b != bones->end()) {
						key.pose.boneDeg[i] = FromJson(*b);
					}
				}
			}
			auto sword = jk.find("sword");
			if (sword != jk.end() && sword->is_object()) {
				if (sword->contains("rot")) {
					key.pose.swordDeg = FromJson((*sword)["rot"]);
				}
				if (sword->contains("pos")) {
					key.pose.swordPos = FromJson((*sword)["pos"]);
				}
			}
			keys.push_back(key);
		}
		std::sort(keys.begin(), keys.end(), [](const Key& a, const Key& b) { return a.frame < b.frame; });
	}
}

bool EnemyPoseKeys::Save(const char* path) const
{
	JsonFile::Json clips = JsonFile::Json::object();
	for (const auto& clip : m_clips) {
		const std::vector<Key>& keys = clip.second;
		if (keys.empty()) {
			continue;
		}
		JsonFile::Json list = JsonFile::Json::array();
		for (const Key& key : keys) {
			JsonFile::Json jk;
			jk["frame"] = key.frame;
			JsonFile::Json bones = JsonFile::Json::object();
			for (int i = 0; i < enBone_Num; i++) {
				if (!::IsZero(key.pose.boneDeg[i])) {
					bones[BONES[i].name] = ToJson(key.pose.boneDeg[i]);
				}
			}
			jk["bones"] = bones;
			jk["sword"] = { { "rot", ToJson(key.pose.swordDeg) }, { "pos", ToJson(key.pose.swordPos) } };
			list.push_back(jk);
		}
		clips[clip.first] = list;
	}
	JsonFile::Json json;
	json["comment"] = u8"黒騎士のモーションに足す関節と剣の角度。F1 の開発用の画面（黒騎士のモーション）で打つ。"
		u8"決まりは Game/Source/Enemy/EnemyPoseKeys.h、FBX への焼き込みは tools/BlenderScripts/retarget_black_knight.py";
	json["clips"] = clips;

	std::ofstream file(std::string(JsonFile::DATA_DIR) + path);
	if (!file) {
		return false;
	}
	file << json.dump(2) << "\n";
	return static_cast<bool>(file);
}

EnemyPoseKeys::Pose EnemyPoseKeys::Evaluate(const std::string& clip, float frame) const
{
	const std::vector<Key>& keys = GetKeys(clip);
	if (keys.empty()) {
		return Pose();
	}
	if (frame <= static_cast<float>(keys.front().frame)) {
		return keys.front().pose;
	}
	if (frame >= static_cast<float>(keys.back().frame)) {
		return keys.back().pose;
	}
	for (size_t i = 0; i + 1 < keys.size(); i++) {
		const float f0 = static_cast<float>(keys[i].frame);
		const float f1 = static_cast<float>(keys[i + 1].frame);
		if (frame < f1) {
			float t = (frame - f0) / (f1 - f0);
			t = t * t * (3.0f - 2.0f * t);  // 始めと終わりをゆっくりにする（スクリプトと同じ）
			return LerpPose(keys[i].pose, keys[i + 1].pose, t);
		}
	}
	return keys.back().pose;
}

EnemyPoseKeys::Key* EnemyPoseKeys::FindKey(const std::string& clip, int frame)
{
	auto it = m_clips.find(clip);
	if (it == m_clips.end()) {
		return nullptr;
	}
	for (Key& key : it->second) {
		if (key.frame == frame) {
			return &key;
		}
	}
	return nullptr;
}

void EnemyPoseKeys::SetKey(const std::string& clip, int frame, const Pose& pose)
{
	if (Key* key = FindKey(clip, frame)) {
		key->pose = pose;
		return;
	}
	std::vector<Key>& keys = m_clips[clip];
	Key key;
	key.frame = frame;
	key.pose = pose;
	keys.push_back(key);
	std::sort(keys.begin(), keys.end(), [](const Key& a, const Key& b) { return a.frame < b.frame; });
}

void EnemyPoseKeys::RemoveKey(const std::string& clip, int frame)
{
	auto it = m_clips.find(clip);
	if (it == m_clips.end()) {
		return;
	}
	auto& keys = it->second;
	keys.erase(std::remove_if(keys.begin(), keys.end(), [frame](const Key& k) { return k.frame == frame; }), keys.end());
}

const std::vector<EnemyPoseKeys::Key>& EnemyPoseKeys::GetKeys(const std::string& clip) const
{
	auto it = m_clips.find(clip);
	return it == m_clips.end() ? NO_KEYS : it->second;
}
