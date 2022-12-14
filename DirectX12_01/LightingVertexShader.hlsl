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

OUTPUT LightingVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
	OUTPUT output;

	// 0.0~1.0�͈̔͂ɂ���
	float w = weight / 100.0f;

	// �s��̐��`���
	matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1.0f - w);

	// ���`��Ԃ����s�����Z
	pos = mul(bm, pos);

	pos = mul(world, pos);
	output.svpos = mul(mul(proj, view), pos);
	output.pos = mul(view, pos);

	normal.w = 0;	// ���s�ړ������𖳌��ɂ���
	output.normal = mul(world, normal);	// �@���ɂ����[���h�ϊ����s��
	output.vnormal = mul(view, output.normal);

	output.uv = uv;

	output.ray = normalize(output.pos.xyz - mul(view, eye));	// �����x�N�g��

	// ���邳�̌v�Z
	float light = -dot(Light.Direction.xyz, output.normal.xyz);
	light = saturate(light);

	return output;
	//return float4(0, 0, 0, 1);
}