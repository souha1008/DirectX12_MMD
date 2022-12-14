#include "BasicShaderHeader.hlsli"

cbuffer SceneView : register(b0)   // 定数バッファー
{
	matrix view;    // ビュー
	matrix proj;    // プロジェクション
	float3 eye;     // 視線
};

cbuffer Transform : register(b1)
{
	matrix world;   // ワールド変換行列
	matrix bones[256];  // ボーン行列
};

cbuffer Material : register(b2)
{
	float4 diffuse;
	float4 specular;
	float3 ambient;
};

OUTPUT LightingVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
	OUTPUT output;

	// 0.0~1.0の範囲にする
	float w = weight / 100.0f;

	// 行列の線形補間
	matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1.0f - w);

	// 線形補間した行列を乗算
	pos = mul(bm, pos);

	pos = mul(world, pos);
	output.svpos = mul(mul(proj, view), pos);
	output.pos = mul(view, pos);

	normal.w = 0;	// 平行移動成分を無効にする
	output.normal = mul(world, normal);	// 法線にもワールド変換を行う
	output.vnormal = mul(view, output.normal);

	output.uv = uv;

	output.ray = normalize(output.pos.xyz - mul(view, eye));	// 視線ベクトル

	// 明るさの計算
	float light = -dot(Light.Direction.xyz, output.normal.xyz);
	light = saturate(light);

	return output;
	//return float4(0, 0, 0, 1);
}