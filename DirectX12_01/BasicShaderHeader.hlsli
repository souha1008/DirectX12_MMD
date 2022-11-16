Texture2D<float4> tex : register(t0);   // 0番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t1);   // 1番スロットに設定されたテクスチャ
Texture2D<float4> spa:register(t2);//2番スロットに設定されたテクスチャ(加算)
SamplerState smp : register(s0);

cbuffer SceneBuffer : register(b0)   // 定数バッファー
{
    matrix world; // ワールド変換行列
    matrix view;    // ビュー
    matrix proj;    // プロジェクション
    float3 eye; // 視線
};

cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
};

struct OUTPUT
{
    float4 svpos : SV_POSITION; // システム用頂点座標
    float4 pos : POSITION;
    float4 normal : NORMAL0;  // 法線ベクトル
    float4 vnormal : NORMAL1;
    float2 uv : TEXCOORD;   // uv値
    float3 ray : VECTOR;
};