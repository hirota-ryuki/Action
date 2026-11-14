#pragma once

// EffectEngine::ResistEffect に登録するエフェクト番号。
// EffectEngine は番号をゲーム全体で共有し、登録済みの番号への再登録を黙って無視するため、
// 番号が重ならないようにここで一括管理する。
enum class EffectId : int
{
	Fire,
	IceFly,     // 氷魔法の飛んでいる氷
	IceHit,     // 氷魔法の着弾
	FireFly,    // 炎魔法の飛んでいる炎
	FireCharge, // 炎魔法の溜め中
	FireCharged,// 炎魔法が最大まで溜まった瞬間
	FireHit,    // 炎魔法の着弾
	FireGround, // 炎魔法の着弾後に燃え続ける床
};
