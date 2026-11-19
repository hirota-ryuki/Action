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

	// clipNo のクリップを t 秒の位置で止め、層を重ねた姿勢を、モデルの根元から見た行列にして pose に入れる。
	void SamplePose(ModelRender& modelRender, int clipNo, float t, std::vector<Matrix>& pose)
	{
		modelRender.SetAnimationSpeed(0.0f);
		modelRender.PlayAnimationAtTime(clipNo, t);
		// ModelRender::Update は、骨のワールド行列を作ってから（このとき層が重なる）アニメーションを進める。
		// 1回目で t 秒の姿勢を作り、2回目でその姿勢から骨のワールド行列を作る。
		modelRender.Update();
		modelRender.Update();
		Matrix toModel;
		toModel.Inverse(modelRender.GetWorldMatrix(0));
		Skeleton& skeleton = modelRender.GetSkeleton();
		pose.resize(skeleton.GetNumBones());
		for (int boneNo = 0; boneNo < skeleton.GetNumBones(); boneNo++) {
			pose[boneNo] = skeleton.GetBone(boneNo)->GetWorldMatrix() * toModel;
		}
	}

	int FrameCount(float length)
	{
		return max(1, static_cast<int>(roundf(length * PoseBaker::BAKE_FPS))) + 1;
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
		const SetupFrameFunc& setupFrame)
	{
		ModelRender& modelRender = model.GetModelRender();
		Skeleton& skeleton = modelRender.GetSkeleton();
		const float length = model.GetClipLength(clipNo);
		const int frameCount = FrameCount(length);

		const size_t slash = outPath.find_last_of("/\\");
		if (slash != std::string::npos) {
			MakeDirectories(outPath.substr(0, slash));
		}
		FILE* fp = nullptr;
		if (fopen_s(&fp, outPath.c_str(), "w") != 0 || fp == nullptr) {
			return false;
		}
		fprintf(fp, "{\n\"clip\": \"%s\",\n\"fps\": %g,\n\"loop\": %s,\n\"bones\": [\n",
			clipName.c_str(), BAKE_FPS, isLoop ? "true" : "false");
		for (int boneNo = 0; boneNo < skeleton.GetNumBones(); boneNo++) {
			const Bone* bone = skeleton.GetBone(boneNo);
			fprintf(fp, "{\"name\": \"%s\", \"parent\": %d, \"bind\": ", ToUtf8(bone->GetName()).c_str(), bone->GetParentBoneNo());
			WriteMatrix(fp, bone->GetBindPoseMatrix());
			fprintf(fp, "}%s\n", boneNo + 1 < skeleton.GetNumBones() ? "," : "");
		}
		fprintf(fp, "],\n\"frames\": [\n");

		layers.SetBaking(true);
		std::vector<Matrix> pose;
		for (int f = 0; f < frameCount; f++) {
			const float t = min(length, f / BAKE_FPS);
			if (setupFrame) {
				setupFrame(t);
			}
			SamplePose(modelRender, clipNo, t, pose);
			fprintf(fp, "[");
			for (int boneNo = 0; boneNo < static_cast<int>(pose.size()); boneNo++) {
				fprintf(fp, boneNo == 0 ? "" : ",");
				WriteMatrix(fp, pose[boneNo]);
			}
			fprintf(fp, "]%s\n", f + 1 < frameCount ? "," : "");
		}
		layers.SetBaking(false);
		modelRender.SetAnimationSpeed(1.0f);
		fprintf(fp, "]\n}\n");
		fclose(fp);
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
