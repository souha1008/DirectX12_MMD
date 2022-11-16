#include "BasicShaderHeader.hlsli"

OUTPUT BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
	OUTPUT output;
	output.svpos = mul(mul(viewproj, world), pos);
	normal.w = 0;
	output.normal = mul(world, normal);
	output.uv = uv;
	return output;
	//return float4(0, 0, 0, 1);
}