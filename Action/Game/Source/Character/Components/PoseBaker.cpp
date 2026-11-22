#include "stdafx.h"
#include "PoseBaker.h"

#include "CharacterModel.h"
#include "PoseLayers.h"

// 焼き込みは開発用なので、Debug ビルドだけに入れる。
#ifdef _DEBUG

namespace
{
	std::string ToUtf8(const wchar_t* text)
	{
		const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		std::string result(size > 0 ? size - 1 : 0, '\0');
		if (size > 1) {
			WideCharToMultiByte(CP_UTF8, 0, text, -1, &result[0], size, nullptr, nullptr);
		}
		return result;
	}

	void WriteMatrix(FILE* fp, const Matrix& m)
	{
		fprintf(fp, "[");
		for (int r = 0; r < 4; r++) {
			for (int c = 0; c < 4; c++) {
				fprintf(fp, "%s%.6g", (r == 0 && c == 0) ? "" : ",", m.m[r][c]);
			}
		}
		fprintf(fp, "]");
	}

	// 今の骨のワールド行列を、モデルの根元から見た行列にして pose に入れる。
	void ReadPose(ModelRender& modelRender, std::vector<Matrix>& pose)
	{
		Matrix toModel;
		toModel.Inverse(modelRender.GetWorldMatrix(0));
		Skeleton& skeleton = modelRender.GetSkeleton();
		pose.resize(skeleton.GetNumBones());
		for (int boneNo = 0; boneNo < skeleton.GetNumBones(); boneNo++) {
			pose[boneNo] = skeleton.GetBone(boneNo)->GetWorldMatrix() * toModel;
		}
	}

	// clipNo のクリップを t 秒の位置で止め、層を重ねた姿勢を、モデルの根元から見た行列にして pose に入れる。
	void SamplePose(ModelRender& modelRender, int clipNo, float t, std::vector<Matrix>& pose)
	{
		modelRender.SetAnimationSpeed(0.0f);
		modelRender.PlayAnimationAtTime(clipNo, t);
		// ModelRender::Update は、骨のワールド行列を作ってから（このとき層が重なる）アニメーションを進める。
		// 1回目で t 秒の姿勢を作り、2回目でその姿勢から骨のワールド行列を作る。
		modelRender.Update();
		modelRender.Update();
		ReadPose(modelRender, pose);
	}

	int FrameCount(float length)
	{
		return max(1, static_cast<int>(roundf(length * PoseBaker::BAKE_FPS))) + 1;
	}

	// JSON を開き、クリップの情報と骨（名前・親・バインドポーズ）を書いて、"frames" の中身を書く手前まで進める。
	FILE* BeginJson(Skeleton& skeleton, const std::string& clipName, bool isLoop, const std::string& outPath)
	{
		const size_t slash = outPath.find_last_of("/\\");
		if (slash != std::string::npos) {
			PoseBaker::MakeDirectories(outPath.substr(0, slash));
		}
		FILE* fp = nullptr;
		if (fopen_s(&fp, outPath.c_str(), "w") != 0 || fp == nullptr) {
			return nullptr;
		}
		fprintf(fp, "{\n\"clip\": \"%s\",\n\"fps\": %g,\n\"loop\": %s,\n\"bones\": [\n",
			clipName.c_str(), PoseBaker::BAKE_FPS, isLoop ? "true" : "false");
		for (int boneNo = 0; boneNo < skeleton.GetNumBones(); boneNo++) {
			const Bone* bone = skeleton.GetBone(boneNo);
			fprintf(fp, "{\"name\": \"%s\", \"parent\": %d, \"bind\": ", ToUtf8(bone->GetName()).c_str(), bone->GetParentBoneNo());
			WriteMatrix(fp, bone->GetBindPoseMatrix());
			fprintf(fp, "}%s\n", boneNo + 1 < skeleton.GetNumBones() ? "," : "");
		}
		fprintf(fp, "],\n\"frames\": [\n");
		return fp;
	}

	// 1コマ分の姿勢を書く。isLast なら、あとに区切りの , を付けない。
	void WriteFrame(FILE* fp, const std::vector<Matrix>& pose, bool isLast)
	{
		fprintf(fp, "[");
		for (int boneNo = 0; boneNo < static_cast<int>(pose.size()); boneNo++) {
			fprintf(fp, boneNo == 0 ? "" : ",");
			WriteMatrix(fp, pose[boneNo]);
		}
		fprintf(fp, "]%s\n", isLast ? "" : ",");
	}

	void EndJson(FILE* fp)
	{
		fprintf(fp, "]\n}\n");
		fclose(fp);
	}

	// 0〜length 秒を細かく区切って姿勢を作り、骨が動いた道のりの合計がコマごとに同じになる時刻を、コマの数だけ返す。
	// 最初と最後の時刻は 0 と length のまま（繰り返すクリップの、最後と最初のつなぎ目は変わらない）。
	std::vector<float> MakeEvenSpeedTimes(
		ModelRender& modelRender, int clipNo, float length, int frameCount, const PoseBaker::SetupFrameFunc& setupFrame)
	{
		constexpr int SUB_STEPS = 8;  // 1コマを何回に分けて道のりを測るか。
		const int stepCount = (frameCount - 1) * SUB_STEPS;
		std::vector<float> distances(stepCount + 1, 0.0f);
		std::vector<Matrix> pose, prevPose;
		for (int i = 0; i <= stepCount; i++) {
			const float t = length * i / stepCount;
			if (setupFrame) {
				setupFrame(t);
			}
			SamplePose(modelRender, clipNo, t, pose);
			float moved = 0.0f;
			if (i > 0) {
				for (size_t boneNo = 0; boneNo < pose.size(); boneNo++) {
					const Vector3 a(prevPose[boneNo].m[3][0], prevPose[boneNo].m[3][1], prevPose[boneNo].m[3][2]);
					const Vector3 b(pose[boneNo].m[3][0], pose[boneNo].m[3][1], pose[boneNo].m[3][2]);
					moved += (b - a).Length();
				}
				distances[i] = distances[i - 1] + moved;
			}
			prevPose = pose;
		}
		std::vector<float> times(frameCount, 0.0f);
		const float total = distances[stepCount];
		int step = 0;
		for (int f = 0; f < frameCount; f++) {
			if (total <= 0.0f) {
				times[f] = length * f / (frameCount - 1);
				continue;
			}
			const float target = total * f / (frameCount - 1);
			while (step < stepCount - 1 && distances[step + 1] < target) {
				step++;
			}
			const float span = distances[step + 1] - distances[step];
			const float rate = span > 0.0f ? min(1.0f, max(0.0f, (target - distances[step]) / span)) : 0.0f;
			times[f] = length * (step + rate) / stepCount;
		}
		times.front() = 0.0f;
		times.back() = length;
		return times;
	}
}

namespace PoseBaker
{
	void MakeDirectories(const std::string& path)
	{
		for (size_t i = 1; i <= path.size(); i++) {
			if (i == path.size() || path[i] == '/' || path[i] == '\\') {
				CreateDirectoryA(path.substr(0, i).c_str(), nullptr);
			}
		}
	}

	bool Bake(
		CharacterModel& model,
		PoseLayers& layers,
		int clipNo,
		const std::string& clipName,
		bool isLoop,
		const std::string& outPath,
		const SetupFrameFunc& setupFrame,
		bool isEvenSpeed)
	{
		ModelRender& modelRender = model.GetModelRender();
		const float length = model.GetClipLength(clipNo);
		const int frameCount = FrameCount(length);
		FILE* fp = BeginJson(modelRender.GetSkeleton(), clipName, isLoop, outPath);
		if (fp == nullptr) {
			return false;
		}

		layers.SetBaking(true);
		std::vector<float> times;
		if (isEvenSpeed) {
			times = MakeEvenSpeedTimes(modelRender, clipNo, length, frameCount, setupFrame);
		}
		std::vector<Matrix> pose;
		for (int f = 0; f < frameCount; f++) {
			const float t = isEvenSpeed ? times[f] : min(length, f / BAKE_FPS);
			if (setupFrame) {
				setupFrame(t);
			}
			SamplePose(modelRender, clipNo, t, pose);
			WriteFrame(fp, pose, f + 1 == frameCount);
		}
		layers.SetBaking(false);
		modelRender.SetAnimationSpeed(1.0f);
		EndJson(fp);
		return true;
	}

	bool BakeTransition(
		CharacterModel& model,
		PoseLayers& layers,
		int fromClipNo,
		float fromTime,
		int toClipNo,
		float blendSeconds,
		float length,
		const std::string& clipName,
		const std::string& outPath,
		const SetupFrameFunc& setupFrame)
	{
		ModelRender& modelRender = model.GetModelRender();
		const int frameCount = FrameCount(length);
		FILE* fp = BeginJson(modelRender.GetSkeleton(), clipName, false, outPath);
		if (fp == nullptr) {
			return false;
		}

		layers.SetBaking(true);
		// 切り替える前のクリップを fromTime 秒の姿勢にしてから、ゲームと同じく PlayAnimation で補間して切り替える。
		modelRender.SetAnimationSpeed(0.0f);
		modelRender.PlayAnimationAtTime(fromClipNo, fromTime);
		modelRender.Update();
		modelRender.PlayAnimation(toClipNo, blendSeconds);
		// Update 1回で 1コマ分（1 / BAKE_FPS 秒）進むように、再生の速さを決める（焼き込みの間はゲームの時間が進まない）。
		const float gameDeltaTime = g_gameTime->GetFrameDeltaTime();
		modelRender.SetAnimationSpeed(gameDeltaTime > 0.0f ? (1.0f / BAKE_FPS) / gameDeltaTime : 0.0f);
		std::vector<Matrix> pose;
		for (int f = 0; f < frameCount; f++) {
			if (setupFrame) {
				setupFrame(f / BAKE_FPS);
			}
			// Update は、ここまで進めた姿勢から骨のワールド行列を作って（層を重ねて）から、1コマ分進める。
			modelRender.Update();
			ReadPose(modelRender, pose);
			WriteFrame(fp, pose, f + 1 == frameCount);
		}
		layers.SetBaking(false);
		modelRender.SetAnimationSpeed(1.0f);
		EndJson(fp);
		return true;
	}

	float Compare(CharacterModel& model, PoseLayers& layers, int clipNo, const std::string& jsonPath)
	{
		// JSON の "frames" から後ろの数を順に読む（コマごとに、骨の数 × 16 個）。
		FILE* fp = nullptr;
		if (fopen_s(&fp, jsonPath.c_str(), "rb") != 0 || fp == nullptr) {
			return -1.0f;
		}
		std::string text;
		char buffer[65536];
		size_t read = 0;
		while ((read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			text.append(buffer, read);
		}
		fclose(fp);
		const size_t framesAt = text.find("\"frames\"");
		if (framesAt == std::string::npos) {
			return -1.0f;
		}
		std::vector<float> values;
		const char* p = text.c_str() + framesAt + 8;
		while (*p != '\0') {
			if ((*p >= '0' && *p <= '9') || *p == '-' || *p == '.') {
				char* end = nullptr;
				values.push_back(strtof(p, &end));
				p = end;
			}
			else {
				p++;
			}
		}

		ModelRender& modelRender = model.GetModelRender();
		const int numBones = modelRender.GetSkeleton().GetNumBones();
		const int frameCount = static_cast<int>(values.size()) / (numBones * 16);
		if (frameCount <= 0) {
			return -1.0f;
		}
		layers.SetSuspended(true);
		float maxError = 0.0f;
		std::vector<Matrix> pose;
		for (int f = 0; f < frameCount; f++) {
			SamplePose(modelRender, clipNo, f / BAKE_FPS, pose);
			for (int boneNo = 0; boneNo < numBones; boneNo++) {
				const float* m = &values[(static_cast<size_t>(f) * numBones + boneNo) * 16];
				const Vector3 expected(m[12], m[13], m[14]);
				const Vector3 actual(pose[boneNo].m[3][0], pose[boneNo].m[3][1], pose[boneNo].m[3][2]);
				maxError = max(maxError, (expected - actual).Length());
			}
		}
		layers.SetSuspended(false);
		modelRender.SetAnimationSpeed(1.0f);
		return maxError;
	}
}

#endif
