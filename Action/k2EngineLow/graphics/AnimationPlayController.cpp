/*!
* @brief	アニメーション再生コントローラ。
*/

#include "k2EngineLowPreCompile.h"
#include "AnimationPlayController.h"
#include "AnimationClip.h"
#include "Animation.h"
#include "Skeleton.h"

namespace nsK2EngineLow {
	void AnimationPlayController::Init(Skeleton* skeleton, int footStepBoneNo)
	{
		m_footstepBoneNo = footStepBoneNo;
		int numBones = skeleton->GetNumBones();
		//ボーン行列をバシッと確保。
		m_boneMatrix.resize(numBones);
		m_skeleton = skeleton;
	}
	void AnimationPlayController::ChangeAnimationClip(AnimationClip* clip)
	{
		m_animationClip = clip;
		m_currentKeyFrameNo = 0;
		m_time = 0.0f;
		m_isPlaying = true;
		m_footstepPos = g_vec3Zero;
		m_footstepDeltaValue = g_vec3Zero;
		//アニメーションイベントの発生フラグをすべてfalseにする。
		auto& animEventArray = m_animationClip->GetAnimationEvent();
		for (auto i = 0; i < m_animationClip->GetNumAnimationEvent(); i++) {
			animEventArray[i].SetInvokedFlag(false);
		}
	}
	void AnimationPlayController::InvokeAnimationEvent(Animation* animation)
	{
		auto& animEventArray = m_animationClip->GetAnimationEvent();
		for (auto i = 0; i < m_animationClip->GetNumAnimationEvent(); i++) {
			if (m_time > animEventArray[i].GetInvokeTime()
				&& animEventArray[i].IsInvoked() == false) {
				//アニメーションの起動時間を過ぎている且つ、まだイベント起動していない。
				animation->NotifyAnimationEventToListener(
					m_animationClip->GetName(), animEventArray[i].GetEventName()
				);
				animEventArray[i].SetInvokedFlag(true);
			}
		}
	}

	void AnimationPlayController::StartLoop()
	{
		m_footstepPos = g_vec3Zero;
		m_currentKeyFrameNo = 0;
		m_time = 0.0f;
		//アニメーションイベントをすべて未発生にする。
		auto& animEventArray = m_animationClip->GetAnimationEvent();
		for (auto i = 0; i < m_animationClip->GetNumAnimationEvent(); i++) {
			animEventArray[i].SetInvokedFlag(false);
		}
	}
	void AnimationPlayController::CalcBoneMatrixInRootBoneSpace(Bone& bone, Matrix parentMatrix)
	{
		//ワールド行列を計算する。
		auto& mBoneInRootSpace = m_boneMatrix[bone.GetNo()];
		Matrix localMatrix = m_boneMatrix[bone.GetNo()];
		//親の行列とローカル行列を乗算して、ワールド行列を計算する。
		mBoneInRootSpace = localMatrix * parentMatrix;

		//子供のワールド行列も計算する。
		for (auto& childBone : bone.GetChildren()) {
			CalcBoneMatrixInRootBoneSpace(*childBone, mBoneInRootSpace);
		}
	}
	namespace
	{
		// ボーンのローカル行列を、平行移動・拡大・回転に分けたもの。
		// Animation::UpdateGlobalPose のクリップ同士の補間と同じ分け方。
		struct BonePose {
			Vector3 pos;
			Vector3 scale;
			Quaternion rot;
		};

		BonePose DecomposeBoneMatrix(const Matrix& src)
		{
			BonePose pose;
			pose.pos = *(const Vector3*)src.m[3];
			Matrix m = src;
			m.m[3][0] = m.m[3][1] = m.m[3][2] = 0.0f;
			pose.scale.x = (*(Vector3*)m.m[0]).Length();
			pose.scale.y = (*(Vector3*)m.m[1]).Length();
			pose.scale.z = (*(Vector3*)m.m[2]).Length();
			for (int c = 0; c < 3; c++) {
				m.m[0][c] /= pose.scale.x;
				m.m[1][c] /= pose.scale.y;
				m.m[2][c] /= pose.scale.z;
			}
			pose.rot.SetRotation(m);
			return pose;
		}

		// エルミート曲線で、p1（t = 0）から p2（t = 1）までの値を求める。v1・v2 はそれぞれの点での傾き。
		float Hermite(float p1, float p2, float v1, float v2, float t)
		{
			const float t2 = t * t;
			const float t3 = t2 * t;
			return (2.0f * t3 - 3.0f * t2 + 1.0f) * p1 + (t3 - 2.0f * t2 + t) * v1
				+ (-2.0f * t3 + 3.0f * t2) * p2 + (t3 - t2) * v2;
		}

		// p1 と p2 の間を、前後の p0・p3 も通るなめらかな曲線（Catmull-Rom）で補間する。
		// span01・span12・span23 は、それぞれのキーフレームの間の時間。間隔が違っても傾きが合うように使う。
		float CatmullRom(float p0, float p1, float p2, float p3, float span01, float span12, float span23, float t)
		{
			const float v1 = (p2 - p0) * span12 / (span01 + span12);
			const float v2 = (p3 - p1) * span12 / (span12 + span23);
			return Hermite(p1, p2, v1, v2, t);
		}

		// 4つのキーフレームの行列から、k1 と k2 の間（t = 0〜1）の行列を作る。
		// 直線で補間すると、キーフレームごとに動きの向きと速さが急に変わり、高い fps ではカクカクして見える。
		// 曲線にすると、キーフレームのところでも動きがなめらかにつながる。
		Matrix InterpolateBoneMatrixCurve(
			const Matrix& k0, const Matrix& k1, const Matrix& k2, const Matrix& k3,
			float span01, float span12, float span23, float t)
		{
			const BonePose p0 = DecomposeBoneMatrix(k0);
			const BonePose p1 = DecomposeBoneMatrix(k1);
			const BonePose p2 = DecomposeBoneMatrix(k2);
			const BonePose p3 = DecomposeBoneMatrix(k3);

			auto curve3 = [&](const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) {
				return Vector3(
					CatmullRom(a.x, b.x, c.x, d.x, span01, span12, span23, t),
					CatmullRom(a.y, b.y, c.y, d.y, span01, span12, span23, t),
					CatmullRom(a.z, b.z, c.z, d.z, span01, span12, span23, t));
			};
			const Vector3 pos = curve3(p0.pos, p1.pos, p2.pos, p3.pos);
			const Vector3 scale = curve3(p0.scale, p1.scale, p2.scale, p3.scale);

			// クォータニオンは q と -q が同じ回転なので、隣どうしの向きをそろえてから曲線にする。
			Quaternion q0 = p0.rot, q1 = p1.rot, q2 = p2.rot, q3 = p3.rot;
			if (Dot(q0, q1) < 0.0f) { q0.Scale(-1.0f); }
			if (Dot(q2, q1) < 0.0f) { q2.Scale(-1.0f); }
			if (Dot(q3, q2) < 0.0f) { q3.Scale(-1.0f); }
			Quaternion rot;
			rot.x = CatmullRom(q0.x, q1.x, q2.x, q3.x, span01, span12, span23, t);
			rot.y = CatmullRom(q0.y, q1.y, q2.y, q3.y, span01, span12, span23, t);
			rot.z = CatmullRom(q0.z, q1.z, q2.z, q3.z, span01, span12, span23, t);
			rot.w = CatmullRom(q0.w, q1.w, q2.w, q3.w, span01, span12, span23, t);
			rot.Normalize();

			Matrix scaleMatrix, rotMatrix, transMatrix;
			scaleMatrix.MakeScaling(scale);
			rotMatrix.MakeRotationFromQuaternion(rot);
			transMatrix.MakeTranslation(pos);
			return scaleMatrix * rotMatrix * transMatrix;
		}

		// 2つの行列がほぼ同じか（ループのクリップで、最後のキーフレームが最初と同じポーズかを見る）。
		bool IsSameBoneMatrix(const Matrix& a, const Matrix& b)
		{
			for (int r = 0; r < 4; r++) {
				for (int c = 0; c < 4; c++) {
					if (fabsf(a.m[r][c] - b.m[r][c]) > 1.0e-3f) {
						return false;
					}
				}
			}
			return true;
		}
	}

	void AnimationPlayController::SamplingBoneMatrixFromAnimationClip()
	{
		const auto& keyFramePtrListArray = m_animationClip->GetKeyFramePtrListArray();
		for (const auto& keyFrameList : keyFramePtrListArray) {
			if (keyFrameList.size() == 0) {
				continue;
			}
			//現在再生中のキーフレーム（今の時間より後で一番近いもの）を取ってくる。
			KeyFrame* keyframe = keyFrameList.at(m_currentKeyFrameNo);
			// 1つ前のキーフレームとの間を、今の時間で補間する。
			// キーフレームの間隔（30fps など）より細かく描画しても、ポーズがカクカク切り替わらないように。
			// さらに前後のキーフレームも使って曲線にし、キーフレームのところで動きが折れないようにする。
			if (m_currentKeyFrameNo > 0) {
				const int num = (int)keyFrameList.size();
				const int i1 = m_currentKeyFrameNo - 1;
				const int i2 = m_currentKeyFrameNo;
				KeyFrame* prev = keyFrameList.at(i1);
				const float span = keyframe->time - prev->time;
				if (span > FLT_EPSILON) {
					const float t = max(0.0f, min(1.0f, (m_time - prev->time) / span));
					// ループのクリップで、最後のキーフレームが最初と同じポーズなら、両端の前後は反対側から取る。
					// そうでなければ、両端の前後は端のキーフレームそのものにする。
					const bool isWrap = m_animationClip->IsLoop() && num >= 3
						&& IsSameBoneMatrix(keyFrameList.front()->transform, keyFrameList.back()->transform);
					int i0 = i1 - 1;
					int i3 = i2 + 1;
					float span01 = span;
					float span23 = span;
					if (i0 >= 0) {
						span01 = prev->time - keyFrameList.at(i0)->time;
					}
					else if (isWrap) {
						i0 = num - 2;
						span01 = keyFrameList.at(num - 1)->time - keyFrameList.at(num - 2)->time;
					}
					else {
						i0 = i1;
					}
					if (i3 < num) {
						span23 = keyFrameList.at(i3)->time - keyframe->time;
					}
					else if (isWrap) {
						i3 = 1;
						span23 = keyFrameList.at(1)->time - keyFrameList.at(0)->time;
					}
					else {
						i3 = i2;
					}
					m_boneMatrix[keyframe->boneIndex] = InterpolateBoneMatrixCurve(
						keyFrameList.at(i0)->transform, prev->transform, keyframe->transform, keyFrameList.at(i3)->transform,
						max(span01, FLT_EPSILON), span, max(span23, FLT_EPSILON), t);
					continue;
				}
			}
			m_boneMatrix[keyframe->boneIndex] = keyframe->transform;
		}
	}
	void AnimationPlayController::CalcBoneMatrixInRootBoneSpace()
	{
		int numBone = m_skeleton->GetNumBones();
		for (int boneNo = 0; boneNo < numBone; boneNo++) {
			//ルートの骨を検索する。
			auto bone = m_skeleton->GetBone(boneNo);
			if (bone->GetParentBoneNo() != -1) {
				continue;
			}
			CalcBoneMatrixInRootBoneSpace(*bone, g_matIdentity);
		}
	}
	void AnimationPlayController::SamplingDeltaValueFootstepBone()
	{
		if (m_currentKeyFrameNoLastFrame == m_currentKeyFrameNo) {
			//キーフレームが進んでいない。
			return;
		}
		if (m_footstepBoneNo == -1) {
			return;
		}
		int numBone = m_skeleton->GetNumBones();

		for (int boneNo = 0; boneNo < numBone; boneNo++) {
			auto bone = m_skeleton->GetBone(boneNo);
			if (m_footstepBoneNo == bone->GetNo()) {
				auto mat = m_boneMatrix[bone->GetNo()];
				Vector3 footstepBonePos;
				footstepBonePos.x = mat.m[3][0];
				footstepBonePos.y = mat.m[3][1];
				footstepBonePos.z = mat.m[3][2];
				//このフレームでのfootstepの移動量を計算する。
				m_footstepDeltaValue = footstepBonePos - m_footstepPos;
				//このフレームでのfootstepの座標を更新する。
				m_footstepPos = footstepBonePos;
				break;
			}
		}
	}
	void AnimationPlayController::SubtractFootstepbonePosFromAllBone()
	{
		if (m_footstepBoneNo == -1) {
			return;
		}
		int numBone = m_skeleton->GetNumBones();

		for (int boneNo = 0; boneNo < numBone; boneNo++) {
			auto bone = m_skeleton->GetBone(boneNo);
			m_boneMatrix[bone->GetNo()].m[3][0] -= m_footstepPos.x;
			m_boneMatrix[bone->GetNo()].m[3][1] -= m_footstepPos.y;
			m_boneMatrix[bone->GetNo()].m[3][2] -= m_footstepPos.z;
		}
	}
	void AnimationPlayController::ProgressKeyframeNo(float deltaTime)
	{
		//1フレーム前のキーフレーム番号を記録しておく。
		m_currentKeyFrameNoLastFrame = m_currentKeyFrameNo;

		const auto& topBoneKeyFrameList = m_animationClip->GetTopBoneKeyFrameList();

		//補完時間も進めていく。
		m_interpolateTime = min(1.0f, m_interpolateTime + deltaTime);
		while (true) {
			if (m_currentKeyFrameNo >= (int)topBoneKeyFrameList.size()) {
				//終端まで行った。
				if (m_animationClip->IsLoop()) {
					//ループ。最後のキーフレームを越えた分は、次の周回へ持ち越す（補間するので、捨てるとつなぎ目でつまずく）。
					const float length = topBoneKeyFrameList.back()->time;
					const float over = m_time - length;
					StartLoop();
					if (length > FLT_EPSILON && over > 0.0f && over < length) {
						m_time = over;
						continue;
					}
				}
				else {
					//ワンショット再生。
					m_currentKeyFrameNo--;
					m_isPlaying = false;	//再生終わり。
				}
				break;
			}
			if (topBoneKeyFrameList.at(m_currentKeyFrameNo)->time >= m_time) {
				//終わり。
				break;
			}
			//次へ。
			m_currentKeyFrameNo++;
		}
	}
	void AnimationPlayController::Update(
		float deltaTime,
		Animation* animation,
		bool isInvokeAnimationEvent)
	{
		if (m_animationClip == nullptr) {
			return;
		}

		m_time += deltaTime;

		if (isInvokeAnimationEvent) {
			//アニメーションイベントの発生。
			InvokeAnimationEvent(animation);
		}
		//キーフレーム番号を進める。
		ProgressKeyframeNo(deltaTime);

		//ボーン行列をアニメーションクリップからサンプリングしていく。
		SamplingBoneMatrixFromAnimationClip();

		//親の骨座標系になっているボーン行列をルートのボーンの空間に変換していく。
		CalcBoneMatrixInRootBoneSpace();

		//footstepボーンの移動量を取得する。
		SamplingDeltaValueFootstepBone();

		//footstepボーンの移動量を全体の骨から減算する。
		SubtractFootstepbonePosFromAllBone();

		//アニメーション再生した印をスケルトンにつける。
		m_skeleton->SetMarkPlayAnimation();
	}
}
