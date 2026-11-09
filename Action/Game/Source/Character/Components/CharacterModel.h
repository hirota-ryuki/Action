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

	// アニメーション速度・ボーン・状態異常の色など、ModelRender を直接触る部品向け。
	ModelRender& GetModelRender()
	{
		return m_modelRender;
	}

private:
	std::unique_ptr<AnimationClip[]> m_animClips;  // ModelRender が指し続けるので、Init 後は動かさない。
	ModelRender m_modelRender;
	Vector3 m_scale = Vector3::One;
};
