Texture2D<float4> tex : register(t0); // �ʏ�e�N�X�`��

cbuffer DepthParam : register(b0)
{
    float4 dofParam;
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};