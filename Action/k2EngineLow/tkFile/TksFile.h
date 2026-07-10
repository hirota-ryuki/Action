/// <summary>
/// tksファイル
/// </summary>
/// <remarks>
/// tksファイルは独自のスケルトンフォーマットです。
/// このクラスを利用することでtksファイルを扱うことができます。
/// </remarks>
#pragma once

namespace nsK2EngineLow {

	class TksFile : public Noncopyable {
	public:
		
		/// <summary>
		/// ボーン。
		/// </summary>
		struct SBone {
			std::unique_ptr<char[]> name;	//骨の名前。
			int parentNo;					//親の番号。
			float bindPose[4][3];			//バインドポーズ。
			float invBindPose[4][3];		//バインドポーズの逆数。
			int no;							//ボーンの番号。
		};
		/// <summary>
		/// TKSファイルをロードする。
		/// </summary>
		/// <param name="filePath"></param>
		bool Load(const char* filePath);
		/// <summary>
		/// TKSファイルを保存する。
		/// </summary>
		/// <param name="filePath">保存先のファイルパス。</param>
		bool Save(const char* filePath);
		/// <summary>
		/// ボーンを直接設定する(Load()を介さずランタイムで構築したスケルトンをSave()するための経路)。
		/// </summary>
		void SetBonesForRuntimeImport(std::vector<SBone>&& bones)
		{
			m_bones = std::move(bones);
			m_numBone = static_cast<int>(m_bones.size());
		}
		/// <summary>
		/// ボーンに対してクエリを行う。
		/// </summary>
		/// <param name="query">クエリ関数</param>
		void QueryBone(std::function<void(SBone& bone)> query)
		{
			for (auto& bone : m_bones) {
				query(bone);
			}
		}
	private:
		int m_numBone = 0;			//骨の数。
		std::vector<SBone> m_bones;	//骨のリスト。
	};
}