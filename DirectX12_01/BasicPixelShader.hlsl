#include "BasicShaderHeader.hlsli"

float4 BasicPS(OUTPUT input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));
	float brightness = dot(-light, input.normal);
	float2 normalUV = (input.normal.xy + float2(1, -1)) * float2(0.5f, -0.5f);
	return float4(brightness, brightness, brightness, 1) 
		* diffuse 
		* tex.Sample(smp, input.uv) 
		* sph.Sample(smp, normalUV) // èÊéZ
		+ spa.Sample(smp, normalUV);// â¡éZ
}