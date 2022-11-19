#pragma once

using namespace DirectX;
// ロード用ラムダ式
using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;


typedef struct
{
    unsigned int vertNum;
    unsigned int indecesNum;
    unsigned int materialNum;
    unsigned long long materialBufferSize;
}SUBSET;

typedef struct
{
    unsigned char R, G, B, A;
}TEXRGBA;

typedef struct
{
    float version;          // 例：00 00 80 3F == 1.00
    char model_name[20];    // モデル名
    char comment[256];      // モデルコメント
}PMDHeader;

#pragma pack(push, 1)
typedef struct
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
    uint16_t boneNo[2];   // ボーン番号
    uint8_t boneWeight;   // ボーン影響度
    uint8_t edgeFlag;     // 輪郭線フラグ 
    uint16_t dummy;
}PMDVertex;
#pragma pack(pop)

#pragma pack(1) // ここから1バイトパッキングとなり、アライメントは発生しない
typedef struct
{
    XMFLOAT3 diffuse;   // ディフューズ色
    float alpha;        // ディフューズ
    float specularity;   // スペキュラの強さ
    XMFLOAT3 specular;  // スペキュラ色
    XMFLOAT3 ambient;   // アンビエント色
    unsigned char toonIdx;  // トゥーン番号
    unsigned char edgeFlg;  // マテリアルごとの輪郭線フラグ

    // 2バイトのパディングがある
    unsigned int indicesNum;
    char texFilePath[20];   // テクスチャファイルパス + α
}PMDMaterial;   // パディングが発生しないため70バイト
#pragma pack()  // パッキング指定を解除

// シェーダーに転送するデータ
typedef struct
{
    XMFLOAT3 diffuse;   // ディフューズ色
    float alpha;        // ディフューズ
    float specularity;   // スペキュラの強さ
    XMFLOAT3 specular;  // スペキュラ色
    XMFLOAT3 ambient;   // アンビエント色

}MaterialForHlsl;

// それ以外のマテリアルデータ
typedef struct
{
    std::string texPath;    // テクスチャパス
    int toonIdx;            // トゥーン番号
    bool edgeFlg;           // マテリアルごとの輪郭線フラグ

}AdditionalMaterial;

// 全体をまとめるデータ
typedef struct
{
    unsigned int indicesNum;
    MaterialForHlsl material;
    AdditionalMaterial additional;
}MATERIAL;

typedef struct
{
    XMFLOAT4X4 world; // モデル本体を回転させたり移動させたりする
    XMFLOAT4X4 view;
    XMFLOAT4X4 proj;
    XMFLOAT3 eye;   // 視線座標
}SCENEMATRIX;

typedef struct
{
    ComPtr<ID3D12Resource> VertexBuffer;
    ComPtr<ID3D12Resource> IndexBuffer;
    ComPtr<ID3D12Resource> ConstBuffer;
    ComPtr<ID3D12Resource> TextureBuffer;
    ComPtr<ID3D12Resource> MaterialBuffer;

    std::vector<ComPtr<ID3D12Resource>> TextureResource;
    std::vector<ComPtr<ID3D12Resource>> sphResource;
    std::vector<ComPtr<ID3D12Resource>> spaResource;
    std::vector<ComPtr<ID3D12Resource>> toonResource;

    D3D12_VERTEX_BUFFER_VIEW vbView;  // 頂点バッファービュー
    D3D12_INDEX_BUFFER_VIEW ibView;  // インデックスバッファービュー

    ComPtr<ID3D12DescriptorHeap> materialDescHeap;
    ComPtr<ID3D12DescriptorHeap> basicDescHeap;

    TexMetadata MetaData;
    SCENEMATRIX* MapMatrix;

    std::vector<MATERIAL> material;
    SUBSET sub;

}MODEL_DX12;

class Object3D
{
public:
    HRESULT CreateModel(const char* Filename, MODEL_DX12* Model);

    HRESULT CreateVertexBuffer(MODEL_DX12* Model, std::vector<PMDVertex> vertices);
    HRESULT SettingVertexBufferView(MODEL_DX12* Model, std::vector<PMDVertex> vertices, size_t pmdVertex_size);

    HRESULT CreateIndexBuffer(MODEL_DX12* Model, std::vector<unsigned short> index);
    HRESULT SettingIndexBufferView(MODEL_DX12* Model, std::vector<unsigned short> index);

    HRESULT CreateConstBuffer(MODEL_DX12* Model);
    HRESULT SettingConstBufferView(D3D12_CPU_DESCRIPTOR_HANDLE* handle, MODEL_DX12* Model);

    HRESULT CreateBasicDescriptorHeap(MODEL_DX12* Model);

    HRESULT CreateTextureData(MODEL_DX12* Model);
    ID3D12Resource* LoadTextureFromFile(MODEL_DX12* Model, std::string& texPath);

    HRESULT CreateShaderResourceView(MODEL_DX12* Model);

    HRESULT LoadMaterial(FILE* file, MODEL_DX12* Model, std::string ModelPath);
    HRESULT CreateMaterialBuffer(MODEL_DX12* Model);
    HRESULT CreateMaterialView(MODEL_DX12* Model);

    std::string GetTexturePathFromModelandTexPath(const std::string& modelPath, const char* texPath);
    std::wstring GetWideStringFromString(const std::string& str);
    std::string GetExtension(const std::string& path);
    std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');
    
    ID3D12Resource* CreateWhiteTexture();
    ID3D12Resource* CreateBlackTexture();
    ID3D12Resource* CreateGrayGradationTexture();

    void CreateLambdaTable();


    void UnInit(MODEL_DX12* Model);

private:
    // ロード用ラムダ
    std::map<std::string, LoadLambda_t> m_loadLambdaTable;

    // リソース用
    std::map<std::string, ID3D12Resource*> m_resourceTable;

};

