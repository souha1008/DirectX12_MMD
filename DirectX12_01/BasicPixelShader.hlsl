#include "BasicShaderHeader.hlsli"

float4 BasicPS(OUTPUT input) : SV_TARGET
{
	// ���̌������x�N�g��
	float3 light = normalize(float3(1, -1, 1));
	// ���C�g�̃J���[
	float3 lightColor = float3(1, 1, 1);

	// �f�B�t���[�Y�v�Z
	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	// ���̔��˃x�N�g��
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a * 1000);

	// �X�t�B�A�}�b�v�puv
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (input.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);

	// �e�N�X�`���J���[
	float4 Texcolor = tex.Sample(smp, input.uv);

	return max(saturate(toonDif	// �P�x
		* diffuse	// �f�B�t���[�Y�F
		* Texcolor	// �e�N�X�`���J���[
		* sph.Sample(smp, sphereMapUV))	// �X�t�B�A�}�b�v�i��Z�j
		+ saturate(spa.Sample(smp, sphereMapUV) * Texcolor	// �X�t�B�A�}�b�v(���Z)
		+ float4(specularB * float3(1, 1, 1), 1))	// �X�y�L����
		, float4(Texcolor * ambient, 1));		// �A���r�G���g

	//return float4(diffuseB, diffuseB, diffuseB, 1)
	//	* diffuse
	//	* Texcolor
	//	* sph.Sample(smp, sphereMapUV)
	//	+ spa.Sample(smp, sphereMapUV)
	//	;
}