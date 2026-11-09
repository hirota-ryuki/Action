#pragma once

#include "Objects/StageRender/IPropFilter.h"

// 画面で小さくしか見えない物を消す（B4）。
// 画面での大きさ ≒ 半径 ÷ 距離 × (1 / tan(縦の画角 / 2)) を、画面の高さに対する割合として見る。
class ScreenSizeFilter : public IPropFilter
{
public:
	explicit ScreenSizeFilter(float minScreenRatio);
	void Apply(PropList& list, const PropCullContext& context) const override;

private:
	float m_minScreenRatio;  // これより小さく見える物は消す（画面の高さに対する割合）
};
