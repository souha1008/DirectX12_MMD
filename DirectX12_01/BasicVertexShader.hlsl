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

OUTPUT BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
	OUTPUT output;

	pos = mul(bones[boneno[0]], pos);

	pos = mul(world, pos);
	output.svpos = mul(mul(proj, view), pos);
	output.pos = mul(view, pos);

	normal.w = 0;	// 平行移動成分を無効にする
	output.normal = mul(world, normal);	// 法線にもワールド変換を行う
	output.vnormal = mul(view, output.normal);

	output.uv = uv;

	output.ray = normalize(pos.xyz - mul(view, eye));	// 視線ベクトル
	return output;
	//return float4(0, 0, 0, 1);
}