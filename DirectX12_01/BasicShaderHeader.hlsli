Texture2D<float4> tex : register(t0);   // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t1);   // 1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa :register(t2);     //2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(���Z)
Texture2D<float4> toon:register(t3);     //3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�g�D�[��)
SamplerState smp : register(s0);
SamplerState smpToon : register(s1);    // 1�ԃX���b�g�ɐݒ肳�ꂽ(�g�D�[��)

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

struct OUTPUT
{
    float4 svpos : SV_POSITION; // �V�X�e���p���_���W
    float4 pos : POSITION;      // �V�X�e���p���_���W
    float4 normal : NORMAL0;    // �@���x�N�g��
    float4 vnormal : NORMAL1;   // �@���x�N�g��
    float2 uv : TEXCOORD;       // uv�l
    float3 ray : VECTOR;        // �����x�N�g��
};