#pragma once

// EffectEngine::ResistEffect に登録するエフェクト番号。
// EffectEngine は番号をゲーム全体で共有し、登録済みの番号への再登録を黙って無視するため、
// 番号が重ならないようにここで一括管理する。
enum class EffectId : int
{
	Fire,
	IceBullet,  // 氷魔法の弾（飛翔中）
	IceHit,     // 氷魔法の着弾
};
