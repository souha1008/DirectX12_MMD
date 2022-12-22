#pragma once

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

#pragma pack(push, 1) // ここから1バイトパッキングとなり、アライメントは発生しない
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
#pragma pack(pop)  // パッキング指定を解除

#pragma pack(push, 1)
typedef struct
{
    char boneName[20];          // ボーン名
    unsigned short parentNo;    // 親ボーン番号
    unsigned short nextNo;      // 先端のボーン番号
    unsigned char type;         // ボーン種別
    unsigned short  ikBoneNo;   // IKボーン番号
    XMFLOAT3 pos;               // ボーンの基準点座標
}PMDBone;
#pragma pack(pop)

typedef struct
{
    uint16_t boneIdx;   // IK対象のボーンを示す
    uint16_t targetIdx; // ターゲットに近づけるためのボーンインデックス
    uint16_t iterations; // 試行回数
    float limit;        // 1回あたりの回転制限
    std::vector<uint16_t> nodeIdx;  // 間のノード番号
}PMDIK;

typedef struct
{
    char boneName[15];      // ボーン名
    unsigned int frameNo;   // フレーム番号
    XMFLOAT3 location;      // 位置
    XMFLOAT4 quaternion;    // クォータニオン（回転）
    unsigned char bezier[64]; // ベジェ保管パラメータ
}VMDMotion;

// シェーダーに転送するデータ
typedef struct
{
    XMFLOAT3 diffuse;   // ディフューズ色
    float alpha;        // ディフューズ
    XMFLOAT3 specular;  // スペキュラ色
    float specularity;   // スペキュラの強さ
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
}TRANSFORM;

struct BoneNode
{
    uint32_t boneIdx = 0;    // ボーンインデックス
    uint32_t boneType;
    uint32_t ikParentBone;
    XMFLOAT3 startPos = {0, 0, 0};  // ボーン基準点（回転の中心）
    XMFLOAT3 endPos = { 0, 0, 0 };    // ボーン先端店（実際のスキニングには利用しない）
    std::vector<BoneNode*>  children;   // 子ノード
};

struct KeyFrame
{
    unsigned int frameNo;   // アニメーション開始からのフレーム数
    XMVECTOR quaternion;    // クォータニオン
    XMFLOAT3 offset;
    XMFLOAT2 p1, p2;

    KeyFrame(unsigned int fno, XMVECTOR& q, XMFLOAT3& ofst, const XMFLOAT2& ip1, const XMFLOAT2& ip2) 
        : frameNo(fno), quaternion(q), offset(ofst), p1(ip1), p2(ip2)
    {}
};

typedef struct
{
    ComPtr<ID3D12Resource> VertexBuffer;
    ComPtr<ID3D12Resource> IndexBuffer;
    ComPtr<ID3D12Resource> TransfromConstBuffer;
    ComPtr<ID3D12Resource> TextureBuffer;
    ComPtr<ID3D12Resource> MaterialBuffer;

    std::vector<ComPtr<ID3D12Resource>> TextureResource;
    std::vector<ComPtr<ID3D12Resource>> sphResource;
    std::vector<ComPtr<ID3D12Resource>> spaResource;
    std::vector<ComPtr<ID3D12Resource>> toonResource;

    std::vector<XMMATRIX> BoneMatrix;

    D3D12_VERTEX_BUFFER_VIEW vbView;  // 頂点バッファービュー
    D3D12_INDEX_BUFFER_VIEW ibView;  // インデックスバッファービュー

    ComPtr<ID3D12DescriptorHeap> materialDescHeap;
    ComPtr<ID3D12DescriptorHeap> transformDescHeap;

    TRANSFORM TransformMatrix;

    TexMetadata MetaData;
    XMMATRIX* mappedMatrices;

    std::vector<MATERIAL> material;
    SUBSET sub;

    // モーションデータ
    std::unordered_map<std::string, std::vector<KeyFrame>> MotionData;

    std::vector<PMDIK> pmdIKData;


}MODEL_DX12;

class Object3D
{
public:
    // モデル生成（全部入り）
    HRESULT CreateModel(const char* Filename, const char* Motionname, MODEL_DX12* Model);

    // バーテックスバッファ生成
    HRESULT CreateVertexBuffer(MODEL_DX12* Model, std::vector<PMDVertex> vertices);
    HRESULT SettingVertexBufferView(MODEL_DX12* Model, std::vector<PMDVertex> vertices, size_t pmdVertex_size);

    // インデックスバッファ生成
    HRESULT CreateIndexBuffer(MODEL_DX12* Model, std::vector<unsigned short> index);
    HRESULT SettingIndexBufferView(MODEL_DX12* Model, std::vector<unsigned short> index);

    // オブジェクト用定数バッファ生成
    HRESULT CreateTransformCBuffer(MODEL_DX12* Model);

    // テクスチャデータ生成
    HRESULT CreateTextureData(MODEL_DX12* Model);
    // テクスチャデータロード
    ID3D12Resource* LoadTextureFromFile(MODEL_DX12* Model, std::string& texPath);

    // マテリアル生成
    HRESULT LoadMaterial(FILE* file, MODEL_DX12* Model, std::string ModelPath);
    HRESULT CreateMaterialBuffer(MODEL_DX12* Model);
    HRESULT CreateMaterialView(MODEL_DX12* Model);

    // ボーン生成
    HRESULT CreateBone(FILE* file, MODEL_DX12* Model);

    // VMDデータ読み込み
    HRESULT LoadVMDData(FILE* file, MODEL_DX12* Model);
    // IK読み込み
    HRESULT LoadIK(FILE* file, MODEL_DX12* Model);
    // nodeindexの数によって場合分け
    void IKSolve(MODEL_DX12* Model);
    // CCD-IKによりボーン方向を解決
    void SolveCCDIK(MODEL_DX12* Model, const PMDIK& ik);
    // 余弦定理IKによりボーン方向を解決
    void SolveCosineIK(MODEL_DX12* Model, const PMDIK& ik);
    // LookAt行列によりボーン方向を解決
    void SolveLookAtIK(MODEL_DX12* Model, const PMDIK& ik);


    // ボーンを子の末端まで伝える再帰関数
    void RecursiveMatrixMultiply(MODEL_DX12* Model, BoneNode* node, const XMMATRIX& mat);

    std::string GetTexturePathFromModelandTexPath(const std::string& modelPath, const char* texPath);
    std::wstring GetWideStringFromString(const std::string& str);
    std::string GetExtension(const std::string& path);
    std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');
    
    ID3D12Resource* CreateDefaultTexture(size_t width, size_t height);
    ID3D12Resource* CreateWhiteTexture();
    ID3D12Resource* CreateBlackTexture();
    ID3D12Resource* CreateGrayGradationTexture();

    void CreateLambdaTable();

    void PlayAnimation();
    void MotionUpdate(MODEL_DX12* Model);


    void UnInit(MODEL_DX12* Model);

private:
    float GetYFromXonBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);

    // ロード用ラムダ
    std::map<std::string, LoadLambda_t> m_loadLambdaTable;

    // リソース用
    std::map<std::string, ID3D12Resource*> m_resourceTable;

    // ボーン検索
    std::map<std::string, BoneNode> m_BoneNodeTable;

    // インデックスから検索しやすいように
    std::vector<std::string> m_BoneNameArray;
    
    //インデックスからノードを検索しやすいようにしておく
    std::vector<BoneNode*> _boneNodeAddressArray;

    // アニメーション開始時のミリ秒
    DWORD m_StartTime;
    unsigned int m_duration;

};



