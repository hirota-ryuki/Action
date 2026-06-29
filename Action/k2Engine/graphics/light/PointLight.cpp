#include "k2EnginePreCompile.h"
#include "graphics/light/PointLight.h"

namespace nsK2Engine {

	PointLight::~PointLight()
	{
		if (g_sceneLight && m_pointLight) {
			g_sceneLight->DeletePointLight(m_pointLight);
		}
	}
	/// <summary>
	/// 初期化。
	/// </summary>
	void PointLight::Init()
	{
		m_pointLight = g_sceneLight->NewPointLight();
	}

}