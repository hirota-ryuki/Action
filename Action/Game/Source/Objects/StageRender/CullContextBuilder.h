#pragma once

#include "Objects/StageRender/PropTypes.h"

// 今のカメラから、描く物を選ぶのに使う情報（カメラの位置・視錐台の6面・画面での大きさの係数）を作る。
// 1フレームに1回だけ作り、部品の種類ごとに model を差し替えて使い回す。
class CullContextBuilder
{
public:
	static PropCullContext Build(Camera& camera);
};
