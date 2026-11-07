#pragma once

#include "Status.h"

// プレイヤーの、遊んでいる間に変わる値。
// 今は Status（HP）と同じ。スタミナなど Player だけの値は、使うときにここへ足す。
class PlayerStatus : public Status
{
};
