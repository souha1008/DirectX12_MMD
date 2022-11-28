Texture2D<float4> tex : register(t0);   // 0番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t1);   // 1番スロットに設定されたテクスチャ
Texture2D<float4> spa :register(t2);     //2番スロットに設定されたテクスチャ(加算)
Texture2D<float4> toon:register(t3);     //3番スロットに設定されたテクスチャ(トゥーン)
SamplerState smp : register(s0);
SamplerState smpToon : register(s1);    // 1番スロットに設定された(トゥーン)

cbuffer SceneView : register(b0)   // 定数バッファー
{
    matrix view;    // ビュー
    matrix proj;    // プロジェクション
    float3 eye;     // 視線
};

cbuffer Transform : register(b1)
{
    matrix world;   // ワールド変換行列
    matrix bones[256];  // ボーン行列
};

cbuffer Material : register(b2)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
};

struct OUTPUT
{
    float4 svpos : SV_POSITION; // システム用頂点座標
    float4 pos : POSITION;      // システム用頂点座標
    float4 normal : NORMAL0;    // 法線ベクトル
    float4 vnormal : NORMAL1;   // 法線ベクトル
    float2 uv : TEXCOORD;       // uv値
    float3 ray : VECTOR;        // 視線ベクトル
};