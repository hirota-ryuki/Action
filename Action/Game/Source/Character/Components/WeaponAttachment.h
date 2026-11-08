#pragma once

// 武器をキャラクターのボーンに付ける指定。
struct WeaponAttachmentDesc
{
	std::string modelPath;
	// 付けるボーンの名前。モデルによって名前が違うので、前から順に探して最初に見つかったものを使う。
	std::vector<const wchar_t*> boneNames;
	Vector3 offset = Vector3::Zero;            // ボーンのローカル空間での位置のずれ。
	Vector3 rotationOffsetDeg = Vector3::Zero; // 武器のモデルの向きの補正（X・Y・Z の順の度）。
	Vector3 scale = Vector3::One;
	bool isShadowCaster = true;
};

// 武器（剣など）のモデルを、持ち主のモデルのボーンに毎フレーム追従させる部品。
// 持ち主は Start() で Init() を呼び、持ち主のモデルを Update したあとに Update()、描画で Draw() を呼ぶ。
class WeaponAttachment
{
public:
	// ownerModel は初期化済みのもの。
	void Init(ModelRender& ownerModel, const WeaponAttachmentDesc& desc);
	void Update();
	void Draw(RenderContext& rc);

private:
	ModelRender* m_ownerModel = nullptr;
	ModelRender m_model;
	int m_boneNo = -1;  // -1 はボーンが見つからなかった状態（武器は動かない）。
	Vector3 m_offset = Vector3::Zero;
	Quaternion m_rotationOffset = Quaternion::Identity;
	Vector3 m_scale = Vector3::One;
};
