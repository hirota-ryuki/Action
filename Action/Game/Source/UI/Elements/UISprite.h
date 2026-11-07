#pragma once

// UI の部品で使う画像。
namespace UISprite
{
	// 4×4 の白い画像。SetMulColor で色を付けて、ゲージや仮のアイコンに使う。
	constexpr const char* kWhitePath = "Assets/sprite/white.dds";
	// 4×16 の、上が白く下へ少し暗くなる画像。ゲージの中身に使うと、上に光が当たったような立体感が出る。
	constexpr const char* kGaugePath = "Assets/sprite/gauge.dds";
}
