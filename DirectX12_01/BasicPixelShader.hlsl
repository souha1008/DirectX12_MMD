#include "BasicShaderHeader.hlsli"

float4 BasicPS(OUTPUT input) : SV_TARGET
{
	// 光の向かうベクトル
	float3 light = normalize(float3(1, -1, 1));
	// ライトのカラー
	float3 lightColor = float3(1, 1, 1);

	// ディフューズ計算
	float diffuseB = dot(-light, input.normal);

	// 光の反射ベクトル
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	// スフィアマップ用uv
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	// テクスチャカラー
	float4 Texcolor = tex.Sample(smp, input.uv);

	return max(diffuseB	// 輝度
		* diffuse	// ディフューズ色
		* Texcolor	// テクスチャカラー
		* sph.Sample(smp, sphereMapUV)	// スフィアマップ（乗算）
		+ saturate(spa.Sample(smp, sphereMapUV) * Texcolor	// スフィアマップ(加算)
		+ float4(specularB * specular.rgb, 1))	// スペキュラ
		, float4(Texcolor * ambient, 1));		// アンビエント
}