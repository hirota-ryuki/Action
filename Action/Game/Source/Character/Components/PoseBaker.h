#pragma once

class CharacterModel;
class PoseLayers;

// 骨の調整（PoseLayers）を重ねた姿勢を、クリップのコマごとに JSON に書き出す（焼き込み）。
// JSON は tools/BlenderScripts/pose_bake_to_fbx.py で FBX にし、ゲームでふつうのクリップとして読む。
// 姿勢はモデルの根元から見た行列（モデルの位置・向き・大きさを外したもの）で書くので、
// 焼き込んだときにキャラクターやカメラがどこを向いていても、同じクリップになる。
// 開発用（Debug ビルドで、焼き込みの指定を付けて起動したときだけ使う）。
namespace PoseBaker
{
	constexpr float BAKE_FPS = 30.0f;  // 書き出すコマの速さ。FBX から tka に直すときも 30fps で読む。

	// 各コマを作る前に呼ぶ処理。t はクリップの中の秒。腕の置き方の混ぜ具合を決める、などに使う。
	using SetupFrameFunc = std::function<void(float t)>;

	// clipNo のクリップを、最初から最後まで BAKE_FPS で止めながら流し、焼き込みに入れる層だけを重ねた姿勢を
	// outPath に書く。clipName は Blender で付ける名前、isLoop はゲームで繰り返すか（JSON に残すだけ）。
	// 書けたら true。焼き込みのあと、再生の速さは 1 に戻す。
	// isEvenSpeed なら、骨の動く速さがどのコマでも同じになるように、コマごとの時刻を割り振り直す
	// （出だしが速く終わりが遅いクリップを繰り返すと、つなぎ目で速さが急に変わって跳んで見えるのを防ぐ）。
	bool Bake(
		CharacterModel& model,
		PoseLayers& layers,
		int clipNo,
		const std::string& clipName,
		bool isLoop,
		const std::string& outPath,
		const SetupFrameFunc& setupFrame = nullptr,
		bool isEvenSpeed = false);

	// fromClipNo のクリップの fromTime 秒の姿勢から、toClipNo のクリップへ blendSeconds 秒で切り替える動き
	// （ゲームの PlayAnimation と同じ補間。切り替えの間も、前のクリップは進み続ける）を、length 秒分書く。
	// setupFrame の t は、切り替え始めてからの秒。繰り返さないクリップとして書く。
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
		const SetupFrameFunc& setupFrame);

	// 焼き込んだクリップ（clipNo）を、層を重ねずに流したときの姿勢と、jsonPath に書いた姿勢の差を測る。
	// 返すのは、全部のコマ・全部の骨で、いちばん大きい位置のずれ（モデルの単位）。読めなければ負の数。
	float Compare(CharacterModel& model, PoseLayers& layers, int clipNo, const std::string& jsonPath);

	// path のフォルダを（途中のフォルダも）作る。
	void MakeDirectories(const std::string& path);
}
