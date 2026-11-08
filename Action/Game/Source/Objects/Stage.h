#pragma once

#include <memory>
#include <vector>

#include "StageChunk.h"
#include "StageProps.h"

// ステージ（草原）。
// 配置 JSON（Assets/data/stage/field_props.json）を読み、地形の区画（StageChunk）と
// 岩・木・草などの部品（StageProps）を作る。JSON は tools/StageGenerator/generate_stage.py が書き出す。
class Stage : public IGameObject
{
public:
	Stage();
	~Stage();
	bool Start();
	void Update();
	void Render(RenderContext& rc);

private:
	std::vector<std::unique_ptr<StageChunk>> m_chunks;
	StageProps m_props;
};
