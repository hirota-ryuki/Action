#include "stdafx.h"
#include "CharacterModel.h"
#include "tkFile/FbxRuntimeImporter.h"

namespace
{
	bool EndsWithFbx(const std::string& path)
	{
		if (path.size() < 4) {
			return false;
		}
		std::string ext = path.substr(path.size() - 4);
		for (char& c : ext) {
			c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		}
		return ext == ".fbx";
	}

	// tkm と同じフォルダ・同じ名前の .tks。FBX→tkm 変換で一緒に作られる（ModelRender も同じ規則で読む）。
	std::string ToTksPath(const std::string& tkmPath)
	{
		const size_t dot = tkmPath.find_last_of('.');
		return tkmPath.substr(0, dot) + ".tks";
	}

	// クリップの長さ（いちばん遅いキーフレームの時間。秒）。キーフレームがなければ 0。
	float GetClipLength(const AnimationClip& clip)
	{
		float length = 0.0f;
		for (const auto& keys : clip.GetKeyFramePtrListArray()) {
			if (!keys.empty()) {
				length = max(length, keys.back()->time);
			}
		}
		return length;
	}
}

bool CharacterModel::ReloadAnimationClip(int animClip, const std::string& path, bool isLoop)
{
	if (animClip < 0 || animClip >= m_numAnimClips) {
		return false;
	}
	std::string tkaPath = path;
	if (EndsWithFbx(tkaPath)) {
		const std::string tksPath = ToTksPath(nsK2EngineLow::FbxRuntimeImporter::ResolveToTkmFilePath(m_modelPath.c_str()));
		tkaPath = nsK2EngineLow::FbxRuntimeImporter::ResolveAnimationFbxToTka(path.c_str(), tksPath.c_str());
	}
	if (tkaPath.empty()) {
		return false;  // 変換に失敗した。エラーは変換側で表示される。
	}
	// キーフレームが1つもないクリップを枠に入れると、再生したときに落ちる。先に別の場所で読んで確かめる。
	{
		AnimationClip check;
		check.Load(tkaPath.c_str());
		if (GetClipLength(check) <= 0.0f) {
			return false;
		}
	}
	// ModelRender は枠のアドレスを指し続けるので、同じ場所に作り直す（キーフレームの一覧を足し込まないように）。
	AnimationClip& clip = m_animClips[animClip];
	clip.~AnimationClip();
	new (&clip) AnimationClip();
	clip.Load(tkaPath.c_str());
	clip.SetLoopFlag(isLoop);
	return true;
}

float CharacterModel::GetAnimationClipLength(int animClip) const
{
	if (animClip < 0 || animClip >= m_numAnimClips) {
		return 0.0f;
	}
	return GetClipLength(m_animClips[animClip]);
}

void CharacterModel::Init(const CharacterModelDesc& desc)
{
	m_scale = Vector3(desc.scale, desc.scale, desc.scale);
	m_modelPath = desc.modelPath;

	const int numClips = static_cast<int>(desc.clips.size());
	m_numAnimClips = numClips;
	m_animClips = std::make_unique<AnimationClip[]>(numClips);

	// FBX のアニメーションをリターゲットするにはモデルのスケルトン(.tks)が要る。
	// .tks は FBX→tkm 変換で作られるので、ModelRender::Init より前に変換だけ済ませておく。
	std::string tksPath;

	for (int i = 0; i < numClips; i++) {
		const AnimClipDesc& clip = desc.clips[i];
		std::string tkaPath = clip.path;
		if (EndsWithFbx(tkaPath)) {
			if (tksPath.empty()) {
				tksPath = ToTksPath(nsK2EngineLow::FbxRuntimeImporter::ResolveToTkmFilePath(desc.modelPath.c_str()));
			}
			// 変換結果はファイルの更新日時でキャッシュされるので、同じ FBX を何度渡しても変換は1回。
			tkaPath = nsK2EngineLow::FbxRuntimeImporter::ResolveAnimationFbxToTka(clip.path.c_str(), tksPath.c_str());
		}
		if (tkaPath.empty()) {
			continue;  // 変換に失敗した。エラーは変換側で表示される。
		}
		m_animClips[i].Load(tkaPath.c_str());
		m_animClips[i].SetLoopFlag(clip.isLoop);
	}

	// ModelRender::Init は中で 0 番のクリップを再生するので、クリップは必ずこれより前に読み込む。
	m_modelRender.Init(desc.modelPath.c_str(), numClips > 0 ? m_animClips.get() : nullptr, numClips, desc.upAxis);
	m_modelRender.SetShadowCasterFlag(desc.isShadowCaster);
}
