#include "PeraHeader.hlsli"

float4 ps(Output input) : SV_TARGET
{
	float4 col = tex.Sample(smp, input.uv);
	
	// ���̂܂�
	//return tex.Sample(smp, input.uv);

	// �O���[�X�P�[��
	//float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
	//return float4(Y, Y, Y, 1);

	// �F���]
	//return float4(float3(1, 1, 1) - col.rgb, col.a);

	// �F���𗎂Ƃ�
	//return float4(col.rgb - fmod(col.rgb, 0.25f), col.a);


	return col;

}

float4 BlurPS(Output input) : SV_TARGET
{
	// �ڂ���
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy));	// ����
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));			// ��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy));		// �E��

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));			// ��
	ret += tex.Sample(smp, input.uv);								// ����
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0));			// �E

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy));	// ����
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy));			// ��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy));		// �E��

	return ret / 9.0f;
}

float4 EmbossPS(Output input) : SV_TARGET
{
	// �G���{�X���H
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2;	// ����
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));				// ��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;		// �E��

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));			// ��
	ret += tex.Sample(smp, input.uv);								// ����
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;			// �E

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy))* 2;	// ����
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;			// ��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 0;		// �E��

	float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));

	return float4(Y, Y, Y, ret.a);
}

float4 OutlinePS(Output input) : SV_TARGET
{
	// �֊s�����o
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1;			// ��
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1;			// ��
	ret += tex.Sample(smp, input.uv) * 4;								// ����
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;			// �E
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;			// ��

	// �F���]
	float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
	// ����
	Y = pow(1.0f - Y, 10.0f);
	// �]�v�ȗ֊s��������
	Y = step(0.2, Y);

	return float4(Y, Y, Y, ret.a);
}

float4 GaussianBlur(Output input) : SV_TARGET
{
	// �ڂ���
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 2.0f / w;
	float dy = 2.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	//���̃s�N�Z���𒆐S�ɏc��5���ɂȂ�悤���Z����
	//�ŏ�i
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 2 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 1 / 256;
	//�ЂƂ�i
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 1 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 1 * dy)) * 4 / 256;
	//���S��
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 0 * dy)) * 36 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 0 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0 * dy)) * 6 / 256;
	//����i
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, -1 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, -1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -1 * dy)) * 4 / 256;
	//�ŉ��i
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, -2 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, -2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 1 / 256;

	return ret;
}

float4 VerticalBokehPS(Output input) : SV_TARGET
{
	float w, h, level;
	tex.GetDimensions(0, w, h, level);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	float4 col = tex.Sample(smp, input.uv);
	ret += bkWeights[0] * col;
	for (int i = 1; i < 8; ++i) {
		ret += bkWeights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, dy * i));
		ret += bkWeights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, -dy * i));	// float2�̒��g���t�ɂ���Əc�����ɂڂ�����
	}
	return float4(ret.rgb, col.a);
}

float4 HorizontalBokehPS(Output input) : SV_TARGET
{
	float w, h, level;
	tex.GetDimensions(0, w, h, level);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);



	float4 col = tex.Sample(smp, input.uv);
	ret += bkWeights[0] * col;
	for (int i = 1; i < 8; ++i) {
		ret += bkWeights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(i * dx, 0));
		ret += bkWeights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(-i * dx, 0));
	}
	return float4(ret.rgb,col.a);
}