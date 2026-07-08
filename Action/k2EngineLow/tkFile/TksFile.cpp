#include "k2EngineLowPreCompile.h"
#include "tkFile/TksFile.h"

namespace nsK2EngineLow {
	bool TksFile::Load(const char* filePath)
	{
		auto fp = fopen(filePath, "rb");
		if (fp == nullptr) {
			return false;
		}
		//骨の数を取得。
		fread(&m_numBone, sizeof(m_numBone), 1, fp);
		m_bones.resize(m_numBone);
		for (int i = 0; i < m_numBone; i++) {
			auto& bone = m_bones.at(i);
			size_t nameCount = 0;
			//骨の名前を取得。
			fread(&nameCount, 1, 1, fp);
			bone.name = std::make_unique<char[]>(nameCount + 1);
			fread(bone.name.get(), nameCount + 1, 1, fp);
			//親のIDを取得。
			fread(&bone.parentNo, sizeof(bone.parentNo), 1, fp);
			//バインドポーズを取得。
			fread(bone.bindPose, sizeof(bone.bindPose), 1, fp);
			//バインドポーズの逆数を取得。
			fread(bone.invBindPose, sizeof(bone.invBindPose), 1, fp);
			//ボーンの番号。
			bone.no = i;
		}

		fclose(fp);
		return true;
	}
	bool TksFile::Save(const char* filePath)
	{
		auto fp = fopen(filePath, "wb");
		if (fp == nullptr) {
			return false;
		}
		int numBone = static_cast<int>(m_bones.size());
		fwrite(&numBone, sizeof(numBone), 1, fp);
		for (auto& bone : m_bones) {
			// 骨の名前を書き込む(Load()側は1byteの長さプレフィックスなので255文字まで)。
			uint8_t nameCount = static_cast<uint8_t>(strlen(bone.name.get()));
			fwrite(&nameCount, 1, 1, fp);
			fwrite(bone.name.get(), static_cast<size_t>(nameCount) + 1, 1, fp);
			//親のIDを書き込む。
			fwrite(&bone.parentNo, sizeof(bone.parentNo), 1, fp);
			//バインドポーズを書き込む。
			fwrite(bone.bindPose, sizeof(bone.bindPose), 1, fp);
			//バインドポーズの逆数を書き込む。
			fwrite(bone.invBindPose, sizeof(bone.invBindPose), 1, fp);
		}
		fclose(fp);
		return true;
	}
}
