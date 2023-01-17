struct OUTPUT
{
    float4 svpos : SV_POSITION; // システム用頂点座標
    float4 pos : POSITION;      // システム用頂点座標
    float4 normal : NORMAL0;    // 法線ベクトル
    float4 vnormal : NORMAL1;   // 法線ベクトル
    float2 uv : TEXCOORD;       // uv値
    float3 ray : VECTOR;        // 視線ベクトル
    uint instNo : SV_InstanceID;    // インスタンス番号
};

struct LIGHT
{
    bool Enable;
    bool3 Dummy;
    float4 Direction;
    float4 Diffuse;
    float4 Ambient;
};
cbuffer LightBuffer : register(b3)
{
    LIGHT Light;
}