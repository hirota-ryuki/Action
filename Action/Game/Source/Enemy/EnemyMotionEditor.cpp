#include "stdafx.h"
#include "EnemyMotionEditor.h"
#include "CharacterModel.h"
#include "SpringBones.h"
#include <imgui/imgui.h>

namespace
{
	constexpr const char* KEYS_PATH = "black_knight_pose_keys.json";  // Assets/data/ から数えたパス
	constexpr float ANIM_FPS = 30.0f;  // クリップはどれも 30fps で作っている
	// FBX に焼くスクリプトと設定（ゲームの作業フォルダ Action/Game から数えたパス）
	constexpr const wchar_t* BAKE_SCRIPT = L"..\\tools\\BlenderScripts\\retarget_black_knight.py";
	constexpr const wchar_t* BAKE_CONFIG = L"..\\tools\\BlenderScripts\\black_knight_anims.json";

	std::wstring Widen(const char* s)
	{
		return std::wstring(s, s + strlen(s));
	}

	Vector3 GetTranslation(const Matrix& m)
	{
		return Vector3(m.m[3][0], m.m[3][1], m.m[3][2]);
	}

	// axis まわりに + へ回したとき、from が toward の側へ動くなら 1、逆なら -1。
	// 座標系の右手・左手の違いに左右されずに、角度の向きの決まり（EnemyPoseKeys.h）に合わせるために使う。
	float SignToward(const Vector3& axis, const Vector3& from, const Vector3& toward)
	{
		Matrix m;
		m.MakeRotationAxis(axis, 0.1f);
		Vector3 v = from;
		m.Apply3x3(v);
		return (v - from).Dot(toward) >= 0.0f ? 1.0f : -1.0f;
	}

	Matrix MakeRotationAround(const Vector3& center, const Vector3& axis, float angle)
	{
		Matrix toOrigin, rotation, back;
		toOrigin.MakeTranslation(center * -1.0f);
		rotation.MakeRotationAxis(axis, angle);
		back.MakeTranslation(center);
		return toOrigin * rotation * back;
	}

	// 体の左右・前後・縦の軸と、それぞれの + の向き。
	struct BodyAxes
	{
		Vector3 axis[3];  // 左、前、上
		float sign[3];
	};

	// 骨の付け根を中心に、[左右の軸, 前後の軸, 縦の軸] の順に回す（その骨から先もまとめて）。
	void RotateBone(Bone& bone, const BodyAxes& axes, const Vector3& deg)
	{
		const float values[3] = { deg.x, deg.y, deg.z };
		for (int i = 0; i < 3; i++) {
			if (values[i] == 0.0f) {
				continue;
			}
			const Vector3 center = GetTranslation(bone.GetWorldMatrix());
			SpringBones::RotateBoneTree(bone, MakeRotationAround(center, axes.axis[i], Math::DegToRad(values[i]) * axes.sign[i]));
		}
	}

	std::wstring FindBlender()
	{
		wchar_t env[MAX_PATH];
		if (GetEnvironmentVariableW(L"BLENDER_EXE", env, MAX_PATH) > 0) {
			return env;
		}
		// C:\Program Files\Blender Foundation\Blender x.y\blender.exe のうち、名前が最後（新しい版）のもの
		std::wstring best;
		const std::wstring root = L"C:\\Program Files\\Blender Foundation\\";
		WIN32_FIND_DATAW data;
		HANDLE find = FindFirstFileW((root + L"*").c_str(), &data);
		if (find == INVALID_HANDLE_VALUE) {
			return L"";
		}
		do {
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && data.cFileName[0] != L'.') {
				const std::wstring exe = root + data.cFileName + L"\\blender.exe";
				if (GetFileAttributesW(exe.c_str()) != INVALID_FILE_ATTRIBUTES && exe > best) {
					best = exe;
				}
			}
		} while (FindNextFileW(find, &data));
		FindClose(find);
		return best;
	}

	std::wstring FullPath(const wchar_t* path)
	{
		wchar_t buf[MAX_PATH];
		GetFullPathNameW(path, MAX_PATH, buf, nullptr);
		return buf;
	}
}

void EnemyMotionEditor::Init(CharacterModel* model, const std::vector<ClipEntry>& clips)
{
	m_model = model;
	m_clips = clips;
	m_keys.Load(KEYS_PATH);
	Skeleton& skeleton = m_model->GetModelRender().GetSkeleton();
	for (int i = 0; i < EnemyPoseKeys::enBone_Num; i++) {
		m_boneNos[i] = skeleton.FindBoneID(Widen(EnemyPoseKeys::GetBoneName(i)).c_str());
	}
	m_swordBoneNo = skeleton.FindBoneID(Widen(EnemyPoseKeys::SWORD_BONE_NAME).c_str());
	m_legBoneL = m_boneNos[EnemyPoseKeys::enBone_UpperLegL];
	m_legBoneR = m_boneNos[EnemyPoseKeys::enBone_UpperLegR];
	// 最初は攻撃1（突き）を選んでおく
	for (int i = 0; i < static_cast<int>(m_clips.size()); i++) {
		if (strcmp(m_clips[i].name, "Attack1") == 0) {
			m_clipIndex = i;
		}
	}
}

EnemyMotionEditor::~EnemyMotionEditor()
{
	if (m_bakeProcess != nullptr) {
		CloseHandle(m_bakeProcess);
	}
}

int EnemyMotionEditor::GetFrameCount() const
{
	const float length = m_model->GetClipLength(GetClip().baseClipNo);
	return static_cast<int>(length * ANIM_FPS + 0.5f) + 1;
}

float EnemyMotionEditor::GetCurrentFrame() const
{
	return m_time * ANIM_FPS + 1.0f;
}

void EnemyMotionEditor::SelectClip(int index)
{
	m_clipIndex = index;
	m_time = 0.0f;
}

void EnemyMotionEditor::UpdatePlayback(float deltaTime)
{
	const float length = m_model->GetClipLength(GetClip().baseClipNo);
	if (m_isPlaying) {
		m_time += deltaTime * m_playSpeed;
		if (m_time > length) {
			m_time = 0.0f;  // くり返し流す
		}
	}
	m_time = max(0.0f, min(length, m_time));
	// 時間はここで決めるので、アニメーション自体は進めない
	ModelRender& render = m_model->GetModelRender();
	render.SetAnimationSpeed(0.0f);
	render.PlayAnimationAtTime(GetClip().baseClipNo, m_time);
}

void EnemyMotionEditor::ApplyToSkeleton(Skeleton& skeleton, const Quaternion& modelRotation, float modelScale) const
{
	if (!m_isEditing || m_legBoneL < 0 || m_legBoneR < 0) {
		return;
	}
	const EnemyPoseKeys::Pose pose = m_keys.Evaluate(GetClip().name, GetCurrentFrame());
	if (pose.IsZero()) {
		return;
	}
	// 体の前・上・左。左は、左右の太ももの付け根の並びで向きを決める。
	Vector3 front = Vector3::AxisZ;
	modelRotation.Apply(front);
	front.y = 0.0f;
	front.Normalize();
	const Vector3 up = Vector3::AxisY;
	Vector3 left;
	left.Cross(up, front);
	const Vector3 legs = GetTranslation(skeleton.GetBone(m_legBoneL)->GetWorldMatrix())
		- GetTranslation(skeleton.GetBone(m_legBoneR)->GetWorldMatrix());
	if (left.Dot(legs) < 0.0f) {
		left = left * -1.0f;
	}
	// + の向き: 左右の軸は前が下へ、前後の軸は上が右へ、縦の軸は前が左へ（EnemyPoseKeys.h）
	BodyAxes axes;
	axes.axis[0] = left;
	axes.axis[1] = front;
	axes.axis[2] = up;
	axes.sign[0] = SignToward(left, front, up * -1.0f);
	axes.sign[1] = SignToward(front, up, left * -1.0f);
	axes.sign[2] = SignToward(up, front, left);

	for (int i = 0; i < EnemyPoseKeys::enBone_Num; i++) {
		if (m_boneNos[i] >= 0) {
			RotateBone(*skeleton.GetBone(m_boneNos[i]), axes, pose.boneDeg[i]);
		}
	}
	if (m_swordBoneNo >= 0) {
		Bone& sword = *skeleton.GetBone(m_swordBoneNo);
		RotateBone(sword, axes, pose.swordDeg);
		const Vector3 move = (left * pose.swordPos.x + front * pose.swordPos.y + up * pose.swordPos.z) * modelScale;
		Matrix translation;
		translation.MakeTranslation(move);
		SpringBones::RotateBoneTree(sword, translation);
	}
}

bool EnemyMotionEditor::DrawPoseEditor(EnemyPoseKeys::Pose& pose)
{
	bool changed = false;
	ImGui::TextUnformatted(u8"3つの値: 左右の軸まわり / 前後の軸まわり / 縦の軸まわり（度）");
	ImGui::TextUnformatted(u8"+ の向き: 前が下へ倒れる / 上が体の右へ倒れる / 前が体の左へ回る");
	for (int i = 0; i < EnemyPoseKeys::enBone_Num; i++) {
		ImGui::PushID(i);
		changed |= ImGui::DragFloat3(EnemyPoseKeys::GetBoneLabel(i), &pose.boneDeg[i].x, 0.5f, -180.0f, 180.0f, "%.1f");
		ImGui::SameLine();
		if (ImGui::SmallButton(u8"0")) {
			pose.boneDeg[i] = Vector3::Zero;
			changed = true;
		}
		ImGui::PopID();
	}
	ImGui::Separator();
	ImGui::PushID("sword");
	changed |= ImGui::DragFloat3(u8"剣の傾き（度）", &pose.swordDeg.x, 0.5f, -180.0f, 180.0f, "%.1f");
	changed |= ImGui::DragFloat3(u8"剣の位置 左/前/上（cm）", &pose.swordPos.x, 0.2f, -100.0f, 100.0f, "%.1f");
	if (ImGui::SmallButton(u8"剣を 0 に戻す")) {
		pose.swordDeg = Vector3::Zero;
		pose.swordPos = Vector3::Zero;
		changed = true;
	}
	ImGui::PopID();
	return changed;
}

void EnemyMotionEditor::DrawUI()
{
	UpdateBake();
	ImGui::Begin(u8"黒騎士のモーション");
	if (ImGui::Checkbox(u8"編集する（AI を止めて、選んだクリップをくり返し流す）", &m_isEditing)) {
		if (!m_isEditing) {
			m_isStopped = true;
		}
	}
	if (!m_isEditing) {
		ImGui::TextUnformatted(u8"編集中は、キーを焼く前のクリップに、打ったキーをその場で足して見せる。");
		ImGui::End();
		return;
	}

	// クリップを選ぶ
	if (ImGui::BeginCombo(u8"クリップ", GetClip().name)) {
		for (int i = 0; i < static_cast<int>(m_clips.size()); i++) {
			if (ImGui::Selectable(m_clips[i].name, i == m_clipIndex)) {
				SelectClip(i);
			}
		}
		ImGui::EndCombo();
	}

	// 再生・停止・コマ送り
	const int frameCount = GetFrameCount();
	if (ImGui::Button(m_isPlaying ? u8"止める" : u8"流す")) {
		m_isPlaying = !m_isPlaying;
		if (!m_isPlaying) {
			m_time = (std::round(GetCurrentFrame()) - 1.0f) / ANIM_FPS;  // ちょうどのコマで止める
		}
	}
	ImGui::SameLine();
	int frame = static_cast<int>(std::round(GetCurrentFrame()));
	bool frameChanged = false;
	if (ImGui::Button(u8"< 1コマ")) {
		frame = frame <= 1 ? frameCount : frame - 1;
		frameChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"1コマ >")) {
		frame = frame >= frameCount ? 1 : frame + 1;
		frameChanged = true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	ImGui::SliderFloat(u8"再生の速さ", &m_playSpeed, 0.05f, 1.0f, "%.2f");
	if (ImGui::SliderInt(u8"コマ", &frame, 1, frameCount)) {
		frameChanged = true;
	}
	if (frameChanged) {
		m_isPlaying = false;
		m_time = static_cast<float>(frame - 1) / ANIM_FPS;
	}

	// 打ってあるキー（押すとそのコマへ）
	const std::string clipName = GetClip().name;
	const auto& keys = m_keys.GetKeys(clipName);
	ImGui::Text(u8"キーを打ったコマ（%d 個）:", static_cast<int>(keys.size()));
	for (const auto& key : keys) {
		ImGui::SameLine();
		char label[16];
		sprintf_s(label, "%d", key.frame);
		if (ImGui::SmallButton(label)) {
			m_isPlaying = false;
			m_time = static_cast<float>(key.frame - 1) / ANIM_FPS;
		}
	}
	ImGui::Separator();

	// 止めているときだけ角度を変えられる。変えると、そのコマにキーを打つ。
	const int nowFrame = static_cast<int>(std::round(GetCurrentFrame()));
	EnemyPoseKeys::Key* key = m_isPlaying ? nullptr : m_keys.FindKey(clipName, nowFrame);
	EnemyPoseKeys::Pose pose = key ? key->pose : m_keys.Evaluate(clipName, GetCurrentFrame());
	if (m_isPlaying) {
		ImGui::TextUnformatted(u8"止めると、そのコマの角度を変えられます。");
	}
	else {
		ImGui::Text(key ? u8"%d コマ目（キーあり）" : u8"%d コマ目（キーなし。変えるとキーを打つ）", nowFrame);
	}
	ImGui::BeginDisabled(m_isPlaying);
	if (DrawPoseEditor(pose) && !m_isPlaying) {
		m_keys.SetKey(clipName, nowFrame, pose);
		m_isDirty = true;
	}
	if (key != nullptr) {
		if (ImGui::Button(u8"このコマのキーを消す")) {
			m_keys.RemoveKey(clipName, nowFrame);
			m_isDirty = true;
		}
	}
	ImGui::EndDisabled();
	ImGui::Separator();

	// 保存と焼き込み
	if (ImGui::Button(u8"保存")) {
		if (m_keys.Save(KEYS_PATH)) {
			m_isDirty = false;
			m_message = u8"Assets/data/black_knight_pose_keys.json に保存しました。";
		}
		else {
			m_message = u8"保存できませんでした。";
		}
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"読み直す")) {
		m_keys.Load(KEYS_PATH);
		m_isDirty = false;
		m_message = u8"保存してある値に戻しました。";
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(m_bakeProcess != nullptr);
	if (ImGui::Button(u8"FBX に焼く")) {
		if (m_keys.Save(KEYS_PATH)) {
			m_isDirty = false;
			StartBake();
		}
	}
	ImGui::EndDisabled();
	if (m_isDirty) {
		ImGui::TextUnformatted(u8"保存していない変更があります。");
	}
	if (!m_message.empty()) {
		ImGui::TextWrapped("%s", m_message.c_str());
	}
	ImGui::End();
}

void EnemyMotionEditor::StartBake()
{
	const std::wstring blender = FindBlender();
	if (blender.empty()) {
		m_message = u8"Blender が見つかりません（環境変数 BLENDER_EXE に blender.exe の場所を入れると使えます）。";
		return;
	}
	m_bakeClip = GetClip().name;
	// --python-exit-code 1: スクリプトが途中で止まったら、終了コードを 1 にする（ないと失敗しても 0 になる）
	std::wstring command = L"\"" + blender + L"\" -b --factory-startup --python-exit-code 1 --python \"" + FullPath(BAKE_SCRIPT)
		+ L"\" -- \"" + FullPath(BAKE_CONFIG) + L"\" --only " + Widen(m_bakeClip.c_str());
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi = {};
	if (!CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
		m_message = u8"Blender を起動できませんでした。";
		return;
	}
	CloseHandle(pi.hThread);
	m_bakeProcess = pi.hProcess;
	m_message = m_bakeClip + u8" を FBX に焼いています（Blender で 30 秒ほど）…";
}

void EnemyMotionEditor::UpdateBake()
{
	if (m_bakeProcess == nullptr || WaitForSingleObject(m_bakeProcess, 0) != WAIT_OBJECT_0) {
		return;
	}
	DWORD code = 1;
	GetExitCodeProcess(m_bakeProcess, &code);
	CloseHandle(m_bakeProcess);
	m_bakeProcess = nullptr;
	m_message = code == 0
		? m_bakeClip + u8" を FBX に焼きました。ゲームを起動し直すと、ふだんの動きに入ります。"
		: m_bakeClip + u8" を焼けませんでした（Blender の終了コード " + std::to_string(code) + u8"）。";
}
