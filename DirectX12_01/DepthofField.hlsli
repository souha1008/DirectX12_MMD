Texture2D<float4> tex : register(t0); // 通常テクスチャ

cbuffer DepthParam : register(b0)
{
    float4 dofParam;
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};