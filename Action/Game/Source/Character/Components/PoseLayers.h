#pragma once

// アニメーションの姿勢に、あとから骨の調整を重ねる部品。
// 体の連動・揺れ物・照準へのひねり・腕の見せ方など、骨のワールド行列に手を加える処理を「層」として、足した順に重ねる。
// 層ごとに、焼き込みに入れるか（isBakeable）を持つ。焼き込み（PoseBaker）は、入れる層だけを重ねた姿勢を書き出す。
// 揺れ物（体の動きで変わる）や、腕以外を見えなくする処理は、焼き込みに入れない。
// 持ち主は、モデルを Init したあとに Init() を呼び、Add() で層を足す。毎フレームの呼び出しは、この部品が
// Skeleton のワールド行列を計算したあとの処理（SetOnPostUpdateWorldMatrix）に登録して行う。
class PoseLayers
{
public:
	using ApplyFunc = std::function<void(Skeleton&)>;

	// model はスケルトンを読み込んだあとのもの。
	void Init(ModelRender& model);
	// 層を足す。足した順に重ねる。isBakeable が false の層は、焼き込みのときに飛ばす。
	void Add(ApplyFunc apply, bool isBakeable = true);
	// 焼き込みの間は true にする。焼き込みに入れない層を飛ばす。
	void SetBaking(bool isBaking)
	{
		m_isBaking = isBaking;
	}
	// true の間は、どの層も重ねない（焼き込んだクリップだけの姿勢と比べるときに使う）。
	void SetSuspended(bool isSuspended)
	{
		m_isSuspended = isSuspended;
	}

	// 骨とその子孫のワールド行列に、rotationAroundCenter を掛ける。
	static void RotateBoneTree(Bone& bone, const Matrix& rotationAroundCenter);
	// 骨とその子孫を、center を中心に、axis（向き）のまわりに deg 度回す。
	static void RotateBoneTreeAround(Bone& bone, const Vector3& center, Vector3 axis, float deg);
	// 骨のワールド行列の位置。
	static Vector3 BonePosition(const Skeleton& skeleton, int boneNo);

private:
	struct Layer
	{
		ApplyFunc apply;
		bool isBakeable = true;
	};

	// アニメーションの姿勢ができたあとに、層を順に重ねる。
	void Update(Skeleton& skeleton);

	std::vector<Layer> m_layers;
	bool m_isBaking = false;
	bool m_isSuspended = false;
};
