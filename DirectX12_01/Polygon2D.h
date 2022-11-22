#pragma once

using namespace DirectX;

typedef struct 
{
    XMFLOAT3 pos;
    XMFLOAT2 uv;
}POLYGON;

//typedef struct
//{
//    unsigned char R, G, B, A;
//}TEXRGBA;

//typedef struct
//{
//    float version;          // ��F00 00 80 3F == 1.00
//    char model_name[20];    // ���f����
//    char comment[256];      // ���f���R�����g
//}PMDHeader;
//
//typedef struct
//{
//    XMFLOAT3 pos;
//    XMFLOAT3 normal;
//    XMFLOAT3 uv;
//    unsigned short boneNo[2];   // �{�[���ԍ�
//    unsigned char boneWeight;   // �{�[���e���x
//    unsigned char edgeFlag;     // �֊s���t���O 
//}PMDVertex;

class Polygon2D
{
public:
    void Init();
    void Uninit();
    void Update();
    void Draw();

    HRESULT CreateVertexBuffer();
    HRESULT SettingVertexBufferView();

    HRESULT CreateIndexBuffer(unsigned short* index);
    HRESULT SettingIndexBufferView(unsigned short* index);

    HRESULT CreateSceneCBuffer();
    HRESULT SettingConstBufferView(D3D12_CPU_DESCRIPTOR_HANDLE* handle);

    HRESULT CreateBasicDescriptorHeap();

    HRESULT CreateTextureData();

    HRESULT CreateShaderResourceView();


    void PolygonRotation();
    void PolygonMove();

private:
    ID3D12Resource* m_VertexBuffer = nullptr;
    ID3D12Resource* m_IndexBuffer = nullptr;
    ID3D12Resource* m_ConstBuffer = nullptr;
    ID3D12Resource* m_TextureBuffer = nullptr;

    D3D12_VERTEX_BUFFER_VIEW m_vbView;  // ���_�o�b�t�@�[�r���[
    D3D12_INDEX_BUFFER_VIEW m_ibView;  // �C���f�b�N�X�o�b�t�@�[�r���[

    ID3D12DescriptorHeap* m_basicDescHeap = nullptr;

    TexMetadata m_MetaData;
    
    XMMATRIX m_WorldMatrix;
    XMMATRIX m_viewMat;
    XMMATRIX m_projMat;
    XMMATRIX* m_MapMatrix;

    

};

