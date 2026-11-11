#pragma once

// アニメーションクリップ1つ分の指定。
struct AnimClipDesc
{
	// .tka か .fbx。.fbx ならモデルのスケルトンにボーン名でリターゲットして .tka に変換してから読む。
	std::string path;
	bool isLoop = true;
};

// キャラクターの見た目の指定。キャラクターごとの違いはここに書くデータだけにする。
struct CharacterModelDesc
{
	std::string modelPath;                      // .tkm か .fbx。
	EnModelUpAxis upAxis = enModelUpAxisY;      // 3ds Max で作ったモデルは Z。
	float scale = 1.0f;
	bool isShadowCaster = true;
	// 添字がアニメーションクリップの番号になる（PlayAnimation に渡す番号）。
	std::vector<AnimClipDesc> clips;
};

// キャラクターのモデルとアニメーション。Player・Enemy で共通に使う。
// モデルやアニメーションの読み込みの決まり（FBX の変換・リターゲット・クリップを先に読むこと）はここに閉じ込める。
// 持ち主は Start() で Init() を呼び、毎フレーム SetTransform() → Update() を呼ぶ。
class CharacterModel
{
public:
	void Init(const CharacterModelDesc& desc);

	// 位置と回転を反映する。大きさは Init で指定したもの。
	void SetTransform(const Vector3& position, const Quaternion& rotation)
	{
		m_modelRender.SetTRS(position, rotation, m_scale);
	}
	void Update()
	{
		m_modelRender.Update();
	}
	void Draw(RenderContext& rc)
	{
		m_modelRender.Draw(rc);
	}
	void PlayAnimation(int animClip, float interpolateTime = 0.0f)
	{
		m_modelRender.PlayAnimation(animClip, interpolateTime);
	}
	// ループしないアニメーションが、まだ最後まで流れていないか。
	bool IsPlayingAnimation() const
	{
		return m_modelRender.IsPlayingAnimation();
	}

	// animClip 番の枠に、別のアニメーション（.tka か .fbx）を読み直す。モーションの見本帳など、開発用。
	// 読み直した枠は、続けて PlayAnimationAtTime で再生し直すこと（前の再生の途中の状態は使えなくなる）。
	// 読めなかったら false を返し、枠は前のまま。
	bool ReloadAnimationClip(int animClip, const std::string& path, bool isLoop);
	// 指定した時間のポーズで止める（補間なし）。時間は秒。
	void PlayAnimationAtTime(int animClip, float time)
	{
		m_modelRender.PlayAnimationAtTime(animClip, time);
	}
	// クリップの長さ（最後のキーフレームの時間。秒）。
	float GetAnimationClipLength(int animClip) const;

	// アニメーション速度・ボーン・状態異常の色など、ModelRender を直接触る部品向け。
	ModelRender& GetModelRender()
	{
		return m_modelRender;
	}

private:
	std::unique_ptr<AnimationClip[]> m_animClips;  // ModelRender が指し続けるので、Init 後は動かさない。
	int m_numAnimClips = 0;
	std::string m_modelPath;                        // FBX のアニメーションを読み直すとき、スケルトン（.tks）を探すのに使う。
	ModelRender m_modelRender;
	Vector3 m_scale = Vector3::One;
};
