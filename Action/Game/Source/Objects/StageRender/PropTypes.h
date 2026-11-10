#pragma once

#include <string>
#include <vector>

// ステージの部品（岩・木・草など）を描くときに、部品どうしで受け渡すデータの形。
// どの部品（区画の分け方・フィルター・LOD 選び）も、この形だけを知っていればよい。

// 置いた部品1つ。動かないので、行列などは最初に計算しておく。
struct PropInstance
{
	Vector3 position;     // 置いた場所（足もと）
	float radius = 0.0f;  // 足もとを中心に、部品がまるごと入る球の半径（cm）
	Matrix world;         // ワールド行列（前もって計算しておく）
	float keepValue = 0.0f;  // 0〜1 の決まった乱数。間引くときに、毎フレーム同じ物を残すために使う
};

// 部品1種類の情報。
struct PropModelInfo
{
	std::string kind;           // tree / rock / detail / grass
	float drawDistance = 0.0f;  // この距離より遠い物は描かない（cm）
};

// 描く物を選ぶときに、どの部品にも渡す情報。
struct PropCullContext
{
	Vector3 cameraPosition;
	Vector4 frustumPlanes[6];   // 視錐台の6面。(a, b, c, d) で a*x + b*y + c*z + d >= 0 が内側
	float projectionScaleY = 1.0f;  // 1 / tan(縦の画角 / 2)。画面での大きさを求めるのに使う
	const PropModelInfo* model = nullptr;  // 今選んでいる部品の種類
};

// 描く候補の一覧。フィルターはここから要らない物を消していく。
using PropList = std::vector<const PropInstance*>;
