/*!
* @brief	アニメータークラス。
*/
#include "k2EngineLowPreCompile.h"
#include "Animation.h"
#include "Model.h"

namespace nsK2EngineLow {
	namespace {
		//DCCツールでのアニメーション上での1フレームの経過時間(単位は秒)
		const float DELTA_SEC_PER_FRAME_ON_DCC_TOOL = 1.0f / 30.0f;
	}

	void Animation::Init(
		Skeleton& skeleton,
		AnimationClip* animClips,
		int numAnimClip)
	{
		m_skeleton = &skeleton;
		for (int i = 0; i < numAnimClip; i++) {
			m_animationClips.push_back(&animClips[i]);
		}
		//footstepボーンの番号を調べる。
		int footstepBoneNo = -1;
		int numBone = m_skeleton->GetNumBones();
		for (int boneNo = 0; boneNo < numBone; boneNo++) {
			auto bone = m_skeleton->GetBone(boneNo);
			if (wcscmp(bone->GetName(), L"footstep") == 0) {
				//footstepボーンが見つかった。
				footstepBoneNo = boneNo;
				break;
			}
		}

		for (auto& ctr : m_animationPlayController) {
			ctr.Init(m_skeleton, footstepBoneNo);
		}

		Play(0);
		m_isInited = true;
	}
	/*!
		* @brief	ローカルポーズの更新。
		*/
	void Animation::UpdateLocalPose(float deltaTime)
	{
		m_interpolateTime += deltaTime;
		if (m_interpolateTime >= 1.0f) {
			//補間完了。
			//現在の最終アニメーションコントローラへのインデックスが開始インデックスになる。
			m_startAnimationPlayController = GetLastAnimationControllerIndex();
			m_numAnimationPlayController = 1;
			m_interpolateTime = 1.0f;
		}
		//
		for (int i = 0; i < m_numAnimationPlayController - 1; i++) {
			int index = GetAnimationControllerIndex(m_startAnimationPlayController, i);
			m_animationPlayController[index].Update(deltaTime, this, false);
		}
		//最後のポーズだけ進めていく。
		int lastIndex = GetLastAnimationControllerIndex();
		m_animationPlayController[lastIndex].Update(deltaTime, this, true);

	}
	Vector3 Animation::CalcFootstepDeltaValueInWorldSpace(Quaternion rotation, Vector3 scale) const
	{

		auto footstepDeltaValueInWorldSpace = m_footstepDeltaValue;

		Matrix mBias = Matrix::Identity;
		mBias.MakeRotationX(Math::PI * -0.5f);
		mBias.Apply(footstepDeltaValueInWorldSpace);

		//フットステップの移動量を拡大する。
		footstepDeltaValueInWorldSpace.x *= scale.x;
		footstepDeltaValueInWorldSpace.y *= scale.y;
		footstepDeltaValueInWorldSpace.z *= scale.z;
		//フットステップの移動量を回す。
		rotation.Apply(footstepDeltaValueInWorldSpace);
		//フットステップの移動量をオイラー積分する。
		float t = m_deltaTimeOnUpdate / DELTA_SEC_PER_FRAME_ON_DCC_TOOL;
		footstepDeltaValueInWorldSpace *= t;
		return footstepDeltaValueInWorldSpace;
	}
	void Animation::UpdateGlobalPose()
	{
		//グローバルポーズ計算用のメモリをスタックから確保。
		int numBone = m_skeleton->GetNumBones();
		Quaternion* qGlobalPose = (Quaternion*)alloca(sizeof(Quaternion) * numBone);
		Vector3* vGlobalPose = (Vector3*)alloca(sizeof(Vector3) * numBone);
		Vector3* vGlobalScale = (Vector3*)alloca(sizeof(Vector3) * numBone);
		m_footstepDeltaValue = g_vec3Zero;
		for (int i = 0; i < numBone; i++) {
			qGlobalPose[i] = Quaternion::Identity;
			vGlobalPose[i] = Vector3::Zero;
			vGlobalScale[i] = Vector3::One;
		}
		//グローバルポーズを計算していく。
		int startIndex = m_startAnimationPlayController;
		for (int i = 0; i < m_numAnimationPlayController; i++) {
			int index = GetAnimationControllerIndex(startIndex, i);
			float intepolateRate = m_animationPlayController[index].GetInterpolateRate();
			const auto& localBoneMatrix = m_animationPlayController[index].GetBoneLocalMatrix();
			auto deltaValueFootStep = m_animationPlayController[index].GetFootStepDeltaValueOnUpdate();
			//footstepの移動量の補完
			m_footstepDeltaValue.Lerp(intepolateRate, m_footstepDeltaValue, deltaValueFootStep);
			for (int boneNo = 0; boneNo < numBone; boneNo++) {
				//平行移動の補完
				Matrix m = localBoneMatrix[boneNo];
				vGlobalPose[boneNo].Lerp(
					intepolateRate,
					vGlobalPose[boneNo],
					*(Vector3*)m.m[3]
				);
				//平行移動成分を削除。
				m.m[3][0] = 0.0f;
				m.m[3][1] = 0.0f;
				m.m[3][2] = 0.0f;

				//拡大成分の補間。
				Vector3 vBoneScale;
				vBoneScale.x = (*(Vector3*)m.m[0]).Length();
				vBoneScale.y = (*(Vector3*)m.m[1]).Length();
				vBoneScale.z = (*(Vector3*)m.m[2]).Length();

				vGlobalScale[boneNo].Lerp(
					intepolateRate,
					vGlobalScale[boneNo],
					vBoneScale
				);
				//拡大成分を除去。
				m.m[0][0] /= vBoneScale.x;
				m.m[0][1] /= vBoneScale.x;
				m.m[0][2] /= vBoneScale.x;

				m.m[1][0] /= vBoneScale.y;
				m.m[1][1] /= vBoneScale.y;
				m.m[1][2] /= vBoneScale.y;

				m.m[2][0] /= vBoneScale.z;
				m.m[2][1] /= vBoneScale.z;
				m.m[2][2] /= vBoneScale.z;

				//回転の補完
				Quaternion qBone;
				qBone.SetRotation(m);
				qGlobalPose[boneNo].Slerp(intepolateRate, qGlobalPose[boneNo], qBone);
			}
		}
		if (IsLayer()) {
			//層のときは、下の層が書いた姿勢の上に、骨ごとの重みで重ねる。
			ApplyAsLayer(vGlobalScale, qGlobalPose, vGlobalPose);
		}
		else
		//グローバルポーズをスケルトンに反映させていく。
		for (int boneNo = 0; boneNo < numBone; boneNo++) {

			//拡大行列を作成。
			Matrix scaleMatrix;
			scaleMatrix.MakeScaling(vGlobalScale[boneNo]);
			//回転行列を作成。
			Matrix rotMatrix;
			rotMatrix.MakeRotationFromQuaternion(qGlobalPose[boneNo]);
			//平行移動行列を作成。
			Matrix transMat;
			transMat.MakeTranslation(vGlobalPose[boneNo]);

			//全部を合成して、ボーン行列を作成。
			Matrix boneMatrix;
			boneMatrix = scaleMatrix * rotMatrix;
			boneMatrix = boneMatrix * transMat;

			m_skeleton->SetBoneLocalMatrix(
				boneNo,
				boneMatrix
			);

		}

		//最終アニメーション以外は補間完了していたら除去していく。
		int numAnimationPlayController = m_numAnimationPlayController;
		for (int i = 1; i < m_numAnimationPlayController; i++) {
			int index = GetAnimationControllerIndex(startIndex, i);
			if (m_animationPlayController[index].GetInterpolateRate() > 0.99999f) {
				//補間が終わっているのでアニメーションの開始位置を前にする。
				m_startAnimationPlayController = index;
				numAnimationPlayController = m_numAnimationPlayController - i;
			}
		}
		m_numAnimationPlayController = numAnimationPlayController;
	}

	void Animation::Progress(float deltaTime)
	{
		if (m_numAnimationPlayController == 0) {
			return;
		}
		m_deltaTimeOnUpdate = deltaTime;
		//ローカルポーズの更新をやっていく。
		UpdateLocalPose(deltaTime);

		//グローバルポーズを計算していく。
		UpdateGlobalPose();
	}

	namespace {
		// 拡大・回転・平行移動から、ボーン行列を作る（UpdateGlobalPose と同じ順に掛ける）。
		Matrix ComposeBoneMatrix(const Vector3& scale, const Quaternion& rotation, const Vector3& translation)
		{
			Matrix scaleMatrix;
			scaleMatrix.MakeScaling(scale);
			Matrix rotMatrix;
			rotMatrix.MakeRotationFromQuaternion(rotation);
			Matrix transMat;
			transMat.MakeTranslation(translation);
			return scaleMatrix * rotMatrix * transMat;
		}
		// ボーン行列を、拡大・回転・平行移動に分ける。
		void DecomposeBoneMatrix(Matrix m, Vector3& scale, Quaternion& rotation, Vector3& translation)
		{
			translation = *(Vector3*)m.m[3];
			m.m[3][0] = 0.0f;
			m.m[3][1] = 0.0f;
			m.m[3][2] = 0.0f;
			scale.x = (*(Vector3*)m.m[0]).Length();
			scale.y = (*(Vector3*)m.m[1]).Length();
			scale.z = (*(Vector3*)m.m[2]).Length();
			for (int i = 0; i < 3; i++) {
				m.m[0][i] /= scale.x;
				m.m[1][i] /= scale.y;
				m.m[2][i] /= scale.z;
			}
			rotation.SetRotation(m);
		}
		// 親の骨から見た行列にする。ルートの骨は、そのまま。
		Matrix ToParentSpace(const Matrix* rootSpaceMatrices, int boneNo, int parentNo)
		{
			if (parentNo < 0) {
				return rootSpaceMatrices[boneNo];
			}
			Matrix invParent;
			invParent.Inverse(rootSpaceMatrices[parentNo]);
			return rootSpaceMatrices[boneNo] * invParent;
		}
	}

	void Animation::SetLayerBoneWeights(const std::vector<float>& boneWeights)
	{
		m_layerBoneWeights = boneWeights;
		// 親を子より先に計算できるよう、親から順に並べておく。
		const int numBone = m_skeleton->GetNumBones();
		std::vector<bool> isAdded(numBone, false);
		m_layerBoneOrder.clear();
		for (int boneNo = 0; boneNo < numBone; boneNo++) {
			// 根元までさかのぼってから、根元に近い方から足していく。
			std::vector<int> chain;
			for (int no = boneNo; no >= 0 && !isAdded[no]; no = m_skeleton->GetBone(no)->GetParentBoneNo()) {
				chain.push_back(no);
			}
			for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
				isAdded[*it] = true;
				m_layerBoneOrder.push_back(*it);
			}
		}
	}

	void Animation::ApplyAsLayer(const Vector3* scales, const Quaternion* rotations, const Vector3* translations)
	{
		// 骨の姿勢は、親からではなく、モデルの根元から見た行列で持っている。
		// そのまま差し替えると、下の層で腰が上下しても胸が付いてこないので、
		// 親から見た姿勢に直してから混ぜ、根元から見た姿勢に組み直す。
		const int numBone = m_skeleton->GetNumBones();
		Matrix* baseRoot = (Matrix*)alloca(sizeof(Matrix) * numBone);   // 下の層が書いた姿勢。
		Matrix* layerRoot = (Matrix*)alloca(sizeof(Matrix) * numBone);  // この層の姿勢。
		Matrix* mixedRoot = (Matrix*)alloca(sizeof(Matrix) * numBone);  // 混ぜたあとの姿勢。
		for (int boneNo = 0; boneNo < numBone; boneNo++) {
			baseRoot[boneNo] = m_skeleton->GetBone(boneNo)->GetLocalMatrix();
			layerRoot[boneNo] = ComposeBoneMatrix(scales[boneNo], rotations[boneNo], translations[boneNo]);
		}
		for (int boneNo : m_layerBoneOrder) {
			const int parentNo = m_skeleton->GetBone(boneNo)->GetParentBoneNo();
			const float weight = m_layerBoneWeights[boneNo] * m_layerWeight;
			Matrix mixedLocal = ToParentSpace(baseRoot, boneNo, parentNo);
			if (weight > 0.0f) {
				Vector3 baseScale, layerScale, baseTrans, layerTrans;
				Quaternion baseRot, layerRot;
				DecomposeBoneMatrix(mixedLocal, baseScale, baseRot, baseTrans);
				DecomposeBoneMatrix(ToParentSpace(layerRoot, boneNo, parentNo), layerScale, layerRot, layerTrans);
				const float rate = min(weight, 1.0f);
				baseScale.Lerp(rate, baseScale, layerScale);
				baseRot.Slerp(rate, baseRot, layerRot);
				baseTrans.Lerp(rate, baseTrans, layerTrans);
				mixedLocal = ComposeBoneMatrix(baseScale, baseRot, baseTrans);
			}
			mixedRoot[boneNo] = parentNo < 0 ? mixedLocal : mixedLocal * mixedRoot[parentNo];
			m_skeleton->SetBoneLocalMatrix(boneNo, mixedRoot[boneNo]);
		}
	}
}