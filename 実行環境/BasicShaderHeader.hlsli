struct OUTPUT
{
    float4 svpos : SV_POSITION; // �V�X�e���p���_���W
    float4 pos : POSITION;      // �V�X�e���p���_���W
    float4 normal : NORMAL0;    // �@���x�N�g��
    float4 vnormal : NORMAL1;   // �@���x�N�g��
    float2 uv : TEXCOORD;       // uv�l
    float3 ray : VECTOR;        // �����x�N�g��
    uint instNo : SV_InstanceID;    // �C���X�^���X�ԍ�
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