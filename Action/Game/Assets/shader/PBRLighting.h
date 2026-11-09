///////////////////////////////////////
// PBRライト用のヘッダーファイル
// このヘッダーファイルは3dsMaxでも使用しています。
///////////////////////////////////////

#ifndef _PBRLIGHTING_H_
#define _PBRLIGHTING_H_

///////////////////////////////////////
// 定数
///////////////////////////////////////
#include "PBRLighting_const.h"

///////////////////////////////////////
// 構造体
///////////////////////////////////////
#include "PBRLighting_struct.h"

// これらは3dsMaxでは不要。
#ifndef _MAX_

///////////////////////////////////////
// 定数バッファ。
///////////////////////////////////////

// ライト用の定数バッファー
cbuffer cb_0 : register(b1)
{
    LightCB light;  // ライトデータ
};


#endif // #ifdef _MAX_

///////////////////////////////////////
// 関数
///////////////////////////////////////

// ベックマン分布を計算する
float Beckmann(float m, float t)
{
    float t2 = t * t;
    float t4 = t * t * t * t;
    float m2 = m * m;
    float D = 1.0f / (4.0f * m2 * t4);
    D *= exp((-1.0f / m2) * (1.0f-t2)/ t2);
    return D;
}

// フレネルを計算。Schlick近似を使用
float SpcFresnel(float f0, float u)
{
    // from Schlick
    return f0 + (1-f0) * pow(1-u, 5);
}

// 鏡面反射の調整値。
static const float SPECULAR_F0 = 0.04f;             // 正面から見たときの反射率。金属でない物（草・土・岩・肌・布）はおよそ4%。
static const float SPECULAR_MIN_ROUGHNESS = 0.3f;   // 粗さの下限。0に近いとハイライトが点になり、明るさが跳ね上がる。
static const float SPECULAR_MAX = 1.0f;             // 鏡面反射の上限（ライトの色に掛ける前の値）。ライトの色より明るく照り返さない。

/// <summary>
/// クックトランスモデルの鏡面反射を計算（GGX 分布と Smith の遮蔽）
/// </summary>
/// <remark>
/// 戻り値は N·L を掛けたあとの値。光が当たっていない面（N·L が 0 以下）では 0 を返す。
/// 視線が面すれすれ（N·V が 0 に近い）でも、Smith の遮蔽の分母に k が残るので割り算で無限には大きくならない。
/// それでも逆光の面すれすれでは大きくなるので、SPECULAR_MAX で止める。
/// </remark>
/// <param name="L">光源に向かうベクトル</param>
/// <param name="V">視点に向かうベクトル</param>
/// <param name="N">法線ベクトル</param>
/// <param name="smooth">滑らかさ</param>
float CookTorranceSpecular(float3 L, float3 V, float3 N, float smooth)
{
    float NdotL = dot(N, L);
    if (NdotL <= 0.0f) {
        return 0.0f;
    }
    float NdotV = max(dot(N, V), 0.001f);

    // ライトに向かうベクトルと視線に向かうベクトルのハーフベクトルを求める
    float3 H = normalize(L + V);
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    // 粗さ。見た目の粗さに合わせるため2乗したものを使う。
    float roughness = max(1.0f - smooth, SPECULAR_MIN_ROUGHNESS);
    float a = roughness * roughness;
    float a2 = a * a;

    // D項：GGX 分布
    float d = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    float D = a2 / (PI * d * d);

    // F項：Schlick 近似
    float F = SpcFresnel(SPECULAR_F0, VdotH);

    // G項を 4·N·L·N·V で割ったもの（Smith の遮蔽、Schlick-GGX 近似）
    float k = a * 0.5f;
    float visL = NdotL * (1.0f - k) + k;
    float visV = NdotV * (1.0f - k) + k;
    float vis = 0.25f / (visL * visV);

    // 逆光で面すれすれに見るときは、滑らかな物ほど数十倍まで明るくなるので、上限で止める。
    return min(D * F * vis * NdotL, SPECULAR_MAX);
}

/// <summary>
/// フレネル反射を考慮した拡散反射を計算
/// </summary>
/// <remark>
/// この関数はフレネル反射を考慮した拡散反射率を計算します
/// フレネル反射は、光が物体の表面で反射する現象のとこで、鏡面反射の強さになります
/// 一方拡散反射は、光が物体の内部に入って、内部錯乱を起こして、拡散して反射してきた光のことです
/// つまりフレネル反射が弱いときには、拡散反射が大きくなり、フレネル反射が強いときは、拡散反射が小さくなります
///
/// </remark>
/// <param name="N">法線</param>
/// <param name="L">光源に向かうベクトル。光の方向と逆向きのベクトル。</param>
/// <param name="V">視線に向かうベクトル。</param>
/// <param name="roughness">粗さ。0～1の範囲。</param>
float CalcDiffuseFromFresnel(float3 N, float3 L, float3 V, float smooth)
{
    // step-1 ディズニーベースのフレネル反射による拡散反射を真面目に実装する。
    // 光源に向かうベクトルと視線に向かうベクトルのハーフベクトルを求める
    float3 H = normalize(L+V);
    
    //粗さは0.5で固定。
    float roughness = 1.0f - smooth;
    
    //これは
    float energyBias = lerp(0.0f, 0.5f, roughness);
    float energyFactor = lerp(1.0, 1.0/1.51, roughness);

    // 光源に向かうベクトルとハーフベクトルがどれだけ似ているかを内積で求める
    float dotLH = saturate(dot(L,H));
    // 光源に向かうベクトルとハーフベクトル、光が平行に入射したときの拡散反射量を求めている。
    float Fd90 = energyBias + 2.0 * dotLH * dotLH * roughness;
    
    // 法線と光源に向かうベクトルｗを利用して拡散反射率を求めています
    float dotNL = saturate(dot(N,L));
    float FL = (1 + (Fd90 - 1) * pow(1 - dotNL, 5));

    
    // 法線と視点に向かうベクトルを利用して拡散反射率を求めています
    float dotNV = saturate(dot(N,V));
    float FV =  (1 + (Fd90 - 1) * pow(1 - dotNV, 5));

    //法線と光源への方向に依存する拡散反射率と、法線と視点ベクトルに依存する拡散反射率を
    // 乗算して最終的な拡散反射率を求めている。PIで除算しているのは正規化を行うため
    return (FL*FV * energyFactor);
}



// 反射光を計算する。
float3 CalcLighting(
    float3 ligDir, 
    float3 ligColor, 
    float3 normal,
    float3 toEye, 
    float4 albedoColor,  
    float metaric, 
    float smooth, 
    float3 specColor
    )
{
    // 影が落ちていないのでライトの計算を行う。
    // ディズニーベースの拡散反射を実装する
    // フレネル反射を考慮した拡散反射を計算
    float diffuseFromFresnel = CalcDiffuseFromFresnel(
        normal, -ligDir, toEye, smooth);

    // 正規化Lambert拡散反射を求める
    float NdotL = saturate(dot(normal, -ligDir));
    float3 lambertDiffuse = ligColor * NdotL / PI;

    // 最終的な拡散反射光を計算する
    float3 diffuse = albedoColor * diffuseFromFresnel * lambertDiffuse;

    // クックトランスモデルを利用した鏡面反射率を計算する
    // クックトランスモデルの鏡面反射率を計算する
    float3 spec = CookTorranceSpecular(
        -ligDir, toEye, normal, smooth)
        * ligColor;

    // 金属度が高ければ、鏡面反射はスペキュラカラー、低ければ白
    // スペキュラカラーの強さを鏡面反射率として扱う

    spec *= lerp(float3(1.0f, 1.0f, 1.0f), specColor, metaric);

    // 滑らかさを使って、拡散反射光と鏡面反射光を合成する
    return max( float3( 0.0f, 0.0f, 0.0f ), diffuse * (1.0f - smooth) + spec * smooth );   
}

#endif // _PBRLIGHTING_H_