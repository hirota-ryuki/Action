#pragma once

// キャラクターの陣営。弾の当たり判定やAIの狙う相手の判断に使う。
enum class Faction
{
	Player,   // プレイヤー本人。
	Ally,     // プレイヤーの味方NPC。
	Neutral,  // 中立NPC。誰とも敵対しない（弾も当たらない）。
	Enemy,    // 敵NPC。
};

// a と b が敵対しているか。弾は敵対している相手にだけ当たり、AIは敵対している相手だけを狙う。
inline bool IsHostile(Faction a, Faction b)
{
	if (a == Faction::Neutral || b == Faction::Neutral) {
		return false;
	}
	const bool isAFriendly = a == Faction::Player || a == Faction::Ally;
	const bool isBFriendly = b == Faction::Player || b == Faction::Ally;
	return isAFriendly != isBFriendly;
}
