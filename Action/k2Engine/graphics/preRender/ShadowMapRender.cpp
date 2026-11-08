#include "k2EnginePreCompile.h"
#include "ShadowMapRender.h"

namespace nsK2Engine {
    namespace {
        // 指数シャドウマップの指数。DrawShadowMap.fx と Shadowing.h の INFINITY と同じ値にする。
        constexpr float SHADOW_MAP_EXPONENT = 40.0f;
    }

    void ShadowMapRender::Init(bool isSoftShadow)
    {
        // 影を落とす物が描かれなかった所は、いちばん奥（深度 1.0）の値 exp(40) にしておき、光が当たるようにする。
        // 1.0 でクリアすると exp(40 × 0) と同じになり、「ライトのすぐ前に遮る物がある」として全部影になる。
        // ソフトシャドウでは g に2乗を入れる（DrawShadowMap.fx と同じ）。exp(80) でも float に収まる。
        const float farValue = expf(SHADOW_MAP_EXPONENT);
        float clearColor[4] = { farValue, farValue * farValue, 0.0f, 1.0f };

        DXGI_FORMAT colorFormat;
        DXGI_FORMAT depthFormat;
        if (isSoftShadow) {
            colorFormat = g_softShadowMapFormat.colorBufferFormat;
            depthFormat = g_softShadowMapFormat.depthBufferFormat;
        }
        else {
            colorFormat = g_hardShadowMapFormat.colorBufferFormat;
            depthFormat = g_hardShadowMapFormat.depthBufferFormat;
        }
        //近景用のシャドウマップ
        m_shadowMaps[0].Create(
            2048,
            2048,
            1,
            1,
            colorFormat,
            depthFormat,
            clearColor
        );
        //中景用のシャドウマップ
        m_shadowMaps[1].Create(
            1024,
            1024,
            1,
            1,
            colorFormat,
            depthFormat,
            clearColor
        );
        //遠景用のシャドウマップ
        m_shadowMaps[2].Create(
            512,
            512,
            1,
            1,
            colorFormat,
            depthFormat,
            clearColor
        );

        if (isSoftShadow) {
            // ソフトシャドウを行う。
            m_blur[0].Init(&m_shadowMaps[0].GetRenderTargetTexture());
            m_blur[1].Init(&m_shadowMaps[1].GetRenderTargetTexture());
            m_blur[2].Init(&m_shadowMaps[2].GetRenderTargetTexture());
        }
        m_isSoftShadow = isSoftShadow;
    }

    void ShadowMapRender::Render(
        RenderContext& rc,
        int ligNo,
        Vector3& lightDirection,
        std::vector< IRenderer* >& renderObjects,
        const Vector3& sceneMaxPosition,
        const Vector3& sceneMinPosition
    )
    {
        if (lightDirection.LengthSq() < 0.001f) {
            return;
        }
        // ライトの最大の高さをレンダラーのAABBから計算する。
        m_cascadeShadowMapMatrix.CalcLightViewProjectionCropMatrix(
            lightDirection,
            m_cascadeAreaRateArray,
            sceneMaxPosition,
            sceneMinPosition
        );

        int shadowMapNo = 0;
        for (auto& shadowMap : m_shadowMaps) {
            rc.WaitUntilToPossibleSetRenderTarget(shadowMap);
            rc.SetRenderTargetAndViewport(shadowMap);
            rc.ClearRenderTargetView(shadowMap);

            for (auto& renderer : renderObjects) {
                renderer->OnRenderShadowMap(
                    rc,
                    ligNo,
                    shadowMapNo,
                    m_cascadeShadowMapMatrix.GetLightViewProjectionCropMatrix(shadowMapNo)
                );
            }

            //描画が終わったらクリア
            m_renderers.clear();

            // 書き込み完了待ち
            rc.WaitUntilFinishDrawingToRenderTarget(shadowMap);
            shadowMapNo++;
        }

        if (m_isSoftShadow) {
            // ブラーを実行する。
            for (auto& blur : m_blur) {
                blur.ExecuteOnGPU(rc, 1.0f);
            }
        }
    }
}