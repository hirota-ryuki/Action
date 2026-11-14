#pragma once

// ゲームが始まってからの時間（秒）。魔法のクールダウンや溜めの長さを、時刻の差で測るのに使う。
// PC の時計ではなく、1フレームの経過時間を足していく（一時停止の間は Advance を呼ばなければ止まる）。
class GameClock
{
public:
	// Game が毎フレーム1回だけ呼ぶ。
	static void Advance(float deltaTime);
	static float Now();

private:
	static float s_now;
};
