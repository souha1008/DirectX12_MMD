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
//    float version;          // 例：00 00 80 3F == 1.00
//    char model_name[20];    // モデル名
//    char comment[256];      // モデルコメント
//}PMDHeader;
//
//typedef struct
//{
//    XMFLOAT3 pos;
//    XMFLOAT3 normal;
//    XMFLOAT3 uv;
//    unsigned short boneNo[2];   // ボーン番号
//    unsigned char boneWeight;   // ボーン影響度
//    unsigned char edgeFlag;     // 輪郭線フラグ 
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

    D3D12_VERTEX_BUFFER_VIEW m_vbView;  // 頂点バッファービュー
    D3D12_INDEX_BUFFER_VIEW m_ibView;  // インデックスバッファービュー

    ID3D12DescriptorHeap* m_basicDescHeap = nullptr;

    TexMetadata m_MetaData;
    
    XMMATRIX m_WorldMatrix;
    XMMATRIX m_viewMat;
    XMMATRIX m_projMat;
    XMMATRIX* m_MapMatrix;

    

};

