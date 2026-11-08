#include "k2EngineLowPreCompile.h"
#include "k2EngineLow.h"
#include "graphics/Texture.h"

namespace nsK2EngineLow {
	K2EngineLow* g_engine = nullptr;
	GameTime* g_gameTime = nullptr;

	K2EngineLow::~K2EngineLow()
	{
		// ImGui はグラフィックエンジンを使っているので、先に終了する。
		DebugUI::Finalize();
		// グローバルなアクセスポイントにnullptrを代入。
		g_graphicsEngine = nullptr;
		g_gameTime = nullptr;
		
		delete m_graphicsEngine;
		
		//ゲームオブジェクトマネージャーを削除。
		GameObjectManager::DeleteInstance();
		PhysicsWorld::DeleteInstance();
		EffectEngine::DeleteInstance();

		delete g_soundEngine;
	}
	void K2EngineLow::Init(
		HWND hwnd,
		UINT frameBufferWidth,
		UINT frameBufferHeight,
		const raytracing::InitData& raytracingInitData
	)
	{
		if (hwnd) {
			//グラフィックエンジンの初期化。
			m_graphicsEngine = new GraphicsEngine();
			m_graphicsEngine->Init(
				hwnd, 
				frameBufferWidth, 
				frameBufferHeight,
				raytracingInitData
			);
			// 開発用の画面（ImGui）を起動する。
			DebugUI::Init(hwnd);
		}
		g_gameTime = &m_gameTime;
		//ゲームパッドの初期化。
		for (int i = 0; i < GamePad::CONNECT_PAD_MAX; i++) {
			g_pad[i] = &m_pad[i];
		}

		GameObjectManager::CreateInstance();
		PhysicsWorld::CreateInstance();
		g_soundEngine = new SoundEngine();
		if (m_graphicsEngine) {
			//エフェクトエンジンの初期化。
			EffectEngine::CreateInstance();
		}
#ifdef K2_DEBUG
		if (m_graphicsEngine) {
			m_fpsFont = std::make_unique<Font>();
			m_fpsFontShadow = std::make_unique<Font>();
		}
#endif
		g_engine = this;
	}
	void K2EngineLow::BeginFrame()
	{
		m_fpsLimitter.BeginFrame();
		m_gameTime.BeginMeasurement();
		m_graphicsEngine->BeginRender();
		// ImGui の受け付けを始める。ここから EndFrame() までの間、ゲームの Update で ImGui の関数を呼べる。
		DebugUI::BeginFrame();
		EffectEngine::GetInstance()->BeginFrame();
		for (auto& pad : m_pad) {
			pad.BeginFrame();
		}

	}
	void K2EngineLow::EndFrame()
	{
		// FPS の表示は、ゲームの UI（Game/Source/UI/Hud/DebugHud）で出すので、ここでは描かない。
		// エンジンだけで動かすとき（ゲームの UI がないとき）は、下のコメントを外せば右上に出る。
//#ifdef K2_DEBUG
//		m_fpsFont->Begin(g_graphicsEngine->GetRenderContext());
//		float time = g_gameTime->GetFrameDeltaTime();
//		wchar_t text[256];
//		swprintf(text, L"FPS = %0.2f", 1.0f / time);
//		const float fpsScale = 0.5f;
//		const float fpsShadowOffset = 1.5f;
//		// 右端をそろえるため、文字の幅だけ左から描き始める。位置は文字の左上。
//		const float fpsWidth = m_fpsFont->MeasureString(text).x * fpsScale;
//		const Vector2 fpsPosition = { UI_SPACE_WIDTH * 0.49f - fpsWidth, UI_SPACE_HEIGHT * 0.49f };
//		m_fpsFontShadow->Draw(text, { fpsPosition.x + fpsShadowOffset, fpsPosition.y - fpsShadowOffset }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, fpsScale, { 0.0f, 1.0f });
//		m_fpsFont->Draw(text, fpsPosition, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, fpsScale, { 0.0f, 1.0f });
//		m_fpsFont->End(g_graphicsEngine->GetRenderContext());
//#endif
		// ImGui の画面を、ゲームの絵の上に描く。
		DebugUI::Render();
		m_graphicsEngine->EndRender();
#ifdef USE_FPS_LIMITTER
		m_fpsLimitter.Wait();
#endif
		m_gameTime.EndMeasurement();

	}

	void K2EngineLow::ExecuteUpdate()
	{
		for (auto& pad : m_pad) {
			pad.Update();
		}
		g_soundEngine->Update();
		GameObjectManager::GetInstance()->ExecuteUpdate();
		// エフェクトエンジンの更新。
		EffectEngine::GetInstance()->Update(g_gameTime->GetFrameDeltaTime());
	}
	/// <summary>
	/// 描画処理を実行。
	/// </summary>
	void K2EngineLow::ExecuteRender()
	{
		auto& renderContext = g_graphicsEngine->GetRenderContext();
		// ゲームオブジェクトマネージャーの描画処理を実行。
		GameObjectManager::GetInstance()->ExecuteRender(renderContext);
		
	}

	/// <summary>
	/// 当たり判定描画処理を実行。
	/// </summary>
	void K2EngineLow::DebubDrawWorld()
	{
		auto& renderContext = g_graphicsEngine->GetRenderContext();
		//当たり判定描画処理を実行。
		PhysicsWorld::GetInstance()->DebubDrawWorld(renderContext);
	}
}