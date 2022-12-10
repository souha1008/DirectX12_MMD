#include "BasicShaderHeader.hlsli"

cbuffer SceneView : register(b0)   // �萔�o�b�t�@�[
{
	matrix view;    // �r���[
	matrix proj;    // �v���W�F�N�V����
	float3 eye;     // ����

};

cbuffer Transform : register(b1)
{
	matrix world;   // ���[���h�ϊ��s��
	matrix bones[256];  // �{�[���s��
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

	normal.w = 0;	// ���s�ړ������𖳌��ɂ���
	output.normal = mul(world, normal);	// �@���ɂ����[���h�ϊ����s��
	output.vnormal = mul(view, output.normal);

	output.uv = uv;

	output.ray = normalize(pos.xyz - mul(view, eye));	// �����x�N�g��
	return output;
	//return float4(0, 0, 0, 1);
}