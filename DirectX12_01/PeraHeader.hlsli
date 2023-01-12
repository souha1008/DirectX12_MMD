Texture2D<float4> tex : register(t0); // �ʏ�e�N�X�`��
Texture2D<float4> distTex : register(t1);
SamplerState smp : register(s0);

cbuffer PostEffect : register(b0)
{
    float4 bkWeights[2];
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};