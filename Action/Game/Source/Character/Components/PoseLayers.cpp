#include "stdafx.h"
#include "PoseLayers.h"

void PoseLayers::Init(ModelRender& model)
{
	m_layers.clear();
	model.GetSkeleton().SetOnPostUpdateWorldMatrix([this](Skeleton& skeleton) {
		Update(skeleton);
	});
}

void PoseLayers::Add(ApplyFunc apply, bool isBakeable)
{
	Layer layer;
	layer.apply = std::move(apply);
	layer.isBakeable = isBakeable;
	m_layers.push_back(std::move(layer));
}

void PoseLayers::Update(Skeleton& skeleton)
{
	if (m_isSuspended) {
		return;
	}
	for (Layer& layer : m_layers) {
		if (m_isBaking && !layer.isBakeable) {
			continue;
		}
		layer.apply(skeleton);
	}
}

void PoseLayers::RotateBoneTree(Bone& bone, const Matrix& rotationAroundCenter)
{
	bone.SetWorldMatrix(bone.GetWorldMatrix() * rotationAroundCenter);
	for (Bone* child : bone.GetChildren()) {
		RotateBoneTree(*child, rotationAroundCenter);
	}
}

void PoseLayers::RotateBoneTreeAround(Bone& bone, const Vector3& center, Vector3 axis, float deg)
{
	if (fabsf(deg) < 0.01f || axis.LengthSq() <= FLT_EPSILON) {
		return;
	}
	axis.Normalize();
	Quaternion rotation;
	rotation.SetRotationDeg(axis, deg);
	Matrix toOrigin, rotationMatrix, back;
	toOrigin.MakeTranslation(center * -1.0f);
	rotationMatrix.MakeRotationFromQuaternion(rotation);
	back.MakeTranslation(center);
	RotateBoneTree(bone, toOrigin * rotationMatrix * back);
}

Vector3 PoseLayers::BonePosition(const Skeleton& skeleton, int boneNo)
{
	const Matrix& m = skeleton.GetBone(boneNo)->GetWorldMatrix();
	return Vector3(m.m[3][0], m.m[3][1], m.m[3][2]);
}
