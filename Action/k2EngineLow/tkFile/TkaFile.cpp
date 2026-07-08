#include "k2EngineLowPreCompile.h"
#include "tkFile/TkaFile.h"
#include <errno.h>

namespace nsK2EngineLow {
	void TkaFile::Load(const char* filePath)
	{
		FILE* fp = fopen(filePath, "rb");
		if (fp == nullptr) {
			MessageBoxA(nullptr, "tkaファイルのオープンに失敗しました。", "エラー", MB_OK);
			return;
		}
		//アニメーションクリップのヘッダーをロード。
		AnimClipHeader header;
		fread(&header, sizeof(header), 1, fp);

		if (header.numAnimationEvent > 0) {
			//アニメーションイベントがあるなら、イベント情報をロードする。
			for (auto i = 0; i < (int)header.numAnimationEvent; i++) {
				//イベントが起動する時間を読み込む。
				float invokeTime = 0.0f;
				fread(&invokeTime, sizeof(invokeTime), 1, fp);
				//イｂ根とメイの長さを読み込む。
				std::uint32_t eventNameLength;
				fread(&eventNameLength, sizeof(eventNameLength), 1, fp);
				//イベント名をロードする。
				static char eventName[256];
				static wchar_t wEventName[256];
				fread(eventName, eventNameLength + 1, 1, fp);
				AnimationEvent animEvent;
				animEvent.invokeTime = invokeTime;
				animEvent.eventName = eventName;
				m_animationEvents.push_back(std::move(animEvent));
			}
		}
		//キーフレームの情報をごそっとロード。
		m_keyFrames.resize(header.numKey);

		fread(&m_keyFrames.front(), sizeof(KeyFrame) * header.numKey, 1, fp);

		fclose(fp);

	}
	bool TkaFile::Save(const char* filePath)
	{
		FILE* fp = fopen(filePath, "wb");
		if (fp == nullptr) {
			return false;
		}
		AnimClipHeader header;
		header.numKey = static_cast<std::uint32_t>(m_keyFrames.size());
		header.numAnimationEvent = static_cast<std::uint32_t>(m_animationEvents.size());
		fwrite(&header, sizeof(header), 1, fp);

		for (auto& animEvent : m_animationEvents) {
			fwrite(&animEvent.invokeTime, sizeof(animEvent.invokeTime), 1, fp);
			std::uint32_t eventNameLength = static_cast<std::uint32_t>(animEvent.eventName.size());
			fwrite(&eventNameLength, sizeof(eventNameLength), 1, fp);
			fwrite(animEvent.eventName.c_str(), static_cast<size_t>(eventNameLength) + 1, 1, fp);
		}

		if (!m_keyFrames.empty()) {
			fwrite(&m_keyFrames.front(), sizeof(KeyFrame) * m_keyFrames.size(), 1, fp);
		}

		fclose(fp);
		return true;
	}
}