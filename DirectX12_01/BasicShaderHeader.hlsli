Texture2D<float4> tex : register(t0);   // 0番スロットに設定されたテクスチャ
SamplerState smp : register(s0);

cbuffer cbuff0 : register(b0)   // 定数バッファー
{
    matrix world; // ワールド変換行列
    matrix viewproj;    // ビュープロジェクション行列
}

cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
}

struct OUTPUT
{
    float4 svpos : SV_POSITION; // システム用頂点座標
    float4 normal : NORMAL;  // 法線ベクトル
    float2 uv : TEXCOORD;   // uv値
};