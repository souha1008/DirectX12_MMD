#include "PeraHeader.hlsli"

float4 ps(Output input) : SV_TARGET
{
	float4 col = tex.Sample(smp, input.uv);
	float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
	return float4(Y, Y, Y, 1);

	//return float4(input.uv, 1, 1);
}

//float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));