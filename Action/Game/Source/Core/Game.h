#pragma once

class Player;
class Enemy;
class GameCamera;
class Stage;

namespace nsK2Engine {
	class SkyCube;
}

class Game : public IGameObject
{
public:
	Game();
	~Game();
	bool Start();
	void Update();
	void Render(RenderContext& rc);

private:
	Stage* m_stage = nullptr;
	Player* m_player = nullptr;
	Enemy* m_enemy = nullptr;
	GameCamera* m_gameCamera = nullptr;
	nsK2Engine::SkyCube* m_skyCube = nullptr;
};
