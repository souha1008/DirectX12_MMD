#include "BasicShaderHeader.hlsli"

OUTPUT BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
	OUTPUT output;

	//world = world_1;
	//view = view_1;
	//proj = proj_1;
	//eye = eye_1;

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