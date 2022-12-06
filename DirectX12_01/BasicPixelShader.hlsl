#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex : register(t0);   // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t1);   // 1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa :register(t2);     //2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(���Z)
Texture2D<float4> toon:register(t3);     //3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�g�D�[��)

SamplerState smp : register(s0);
SamplerState smpToon : register(s1);    // 1�ԃX���b�g�ɐݒ肳�ꂽ(�g�D�[��)

cbuffer Material : register(b2)
{
	float4 diffuse;
	float4 specular;
	float3 ambient;
};

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
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a + 800);

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
		+ float4(specularB * float3(1, 1, 1) , 1))	// �X�y�L����
		, float4(Texcolor * ambient, 1));		// �A���r�G���g

	//return float4(diffuseB, diffuseB, diffuseB, 1)
	//	* diffuse
	//	* Texcolor
	//	* sph.Sample(smp, sphereMapUV)
	//	+ spa.Sample(smp, sphereMapUV)
	//	;
}