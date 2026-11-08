#include "stdafx.h"
#include "WeaponAttachment.h"

void WeaponAttachment::Init(ModelRender& ownerModel, const WeaponAttachmentDesc& desc)
{
	m_ownerModel = &ownerModel;
	m_offset = desc.offset;
	m_scale = desc.scale;

	Quaternion rotationX;
	rotationX.SetRotationDegX(desc.rotationOffsetDeg.x);
	Quaternion rotationY;
	rotationY.SetRotationDegY(desc.rotationOffsetDeg.y);
	Quaternion rotationZ;
	rotationZ.SetRotationDegZ(desc.rotationOffsetDeg.z);
	m_rotationOffset = rotationX * rotationY * rotationZ;

	m_boneNo = -1;
	for (const wchar_t* boneName : desc.boneNames) {
		m_boneNo = ownerModel.FindBoneID(boneName);
		if (m_boneNo >= 0) {
			break;
		}
	}

	m_model.Init(desc.modelPath.c_str());
	m_model.SetShadowCasterFlag(desc.isShadowCaster);
	Update();
}

void WeaponAttachment::Update()
{
	if (m_ownerModel == nullptr || m_boneNo < 0) {
		return;
	}
	Bone* bone = m_ownerModel->GetBone(m_boneNo);
	if (bone == nullptr) {
		return;
	}

	Vector3 position;
	Vector3 boneScale;
	Quaternion rotation;
	bone->CalcWorldTRS(position, rotation, boneScale);

	// ボーンの向きに合わせてずらし、武器のモデルの向きを補正する。
	Vector3 offset = m_offset;
	rotation.Apply(offset);
	position += offset;
	rotation = m_rotationOffset * rotation;

	m_model.SetTRS(position, rotation, m_scale);
	m_model.Update();
}

void WeaponAttachment::Draw(RenderContext& rc)
{
	m_model.Draw(rc);
}
