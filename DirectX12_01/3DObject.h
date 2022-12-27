#pragma once

// ロード用ラムダ式
using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

class Object3D
{
public:

    // モデル生成（全部入り）
    HRESULT CreateModel(const char* Filename, const char* Motionname);

    // PMDヘッダー読み込み
    HRESULT LoadPMDHeader(FILE* file);

    // バーテックスバッファ生成
    HRESULT CreateVertexBuffer(FILE* file);

    // インデックスバッファ生成
    HRESULT CreateIndexBuffer(FILE* file);

    // オブジェクト用定数バッファ生成
    HRESULT CreateTransformCBuffer();

    // テクスチャデータロード
    ID3D12Resource* LoadTextureFromFile(std::string& texPath);

    // マテリアル生成
    HRESULT LoadMaterial(FILE* file, std::string ModelPath);
    HRESULT CreateMaterialBuffer();
    HRESULT CreateMaterialView();

    // ボーン生成
    HRESULT LoadPMDBone(FILE* file);

    // VMDデータ読み込み
    HRESULT LoadVMDData(FILE* file);
    // IK読み込み
    HRESULT LoadIK(FILE* file);

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
    void MotionUpdate();

    void Draw();
    void UnInit();

private:
    typedef struct
    {
        unsigned int vertNum;
        unsigned int indecesNum;
        unsigned int materialNum;
        unsigned long long materialBufferSize;
    }SUBSET;

    typedef struct
    {
        uint16_t boneIdx;   // IK対象のボーンを示す
        uint16_t targetIdx; // ターゲットに近づけるためのボーンインデックス
        uint16_t iterations; // 試行回数
        float limit;        // 1回あたりの回転制限
        std::vector<uint16_t> nodeIdx;  // 間のノード番号
    }PMDIK;

    // IKオノフデータ
    typedef struct
    {
        uint32_t frameNo;
        std::unordered_map<std::string, bool> ikEnableTable;
    }VMDIKEnable;

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
        XMFLOAT3 startPos = { 0, 0, 0 };  // ボーン基準点（回転の中心）
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

    ComPtr<ID3D12Resource> VertexBuffer;    // 頂点バッファー
    D3D12_VERTEX_BUFFER_VIEW vbView;  // 頂点バッファービュー
    ComPtr<ID3D12Resource> IndexBuffer; // インデックスバッファー
    D3D12_INDEX_BUFFER_VIEW ibView;  // インデックスバッファービュー

    // モデル用定数バッファ
    ComPtr<ID3D12Resource> TransfromConstBuffer;
    ComPtr<ID3D12DescriptorHeap> transformDescHeap;
    XMMATRIX* mappedMatrices;   // モデル用定数バッファをマップ

    // テクスチャ用バッファ
    ComPtr<ID3D12Resource> TextureBuffer;

    // マテリアル情報格納
    std::vector<MATERIAL> material;
    std::vector<ComPtr<ID3D12Resource>> TextureResource;
    std::vector<ComPtr<ID3D12Resource>> sphResource;
    std::vector<ComPtr<ID3D12Resource>> spaResource;
    std::vector<ComPtr<ID3D12Resource>> toonResource;
    SUBSET sub;

    // マテリアル用定数バッファ
    ComPtr<ID3D12Resource> MaterialConstBuffer;
    ComPtr<ID3D12DescriptorHeap> materialDescHeap;

    // ボーンマトリクス
    std::vector<XMMATRIX> BoneMatrix;

    // モーションデータ
    std::unordered_map<std::string, std::vector<KeyFrame>> MotionData;

    // IKデータ格納
    std::vector<PMDIK> pmdIKData;

    // ロード用ラムダ
    std::map<std::string, LoadLambda_t> m_loadLambdaTable;

    // リソース用
    std::map<std::string, ID3D12Resource*> m_resourceTable;

    // ボーン検索
    std::map<std::string, BoneNode> m_BoneNodeTable;

    // インデックスから検索しやすいように
    std::vector<std::string> m_BoneNameArray;
    
    //インデックスからノードを検索しやすいようにしておく
    std::vector<BoneNode*> m_boneNodeAddressArray;

    std::vector<uint32_t> m_kneeIdxes;

    std::vector<VMDIKEnable> m_IKEnableData;

    // アニメーション開始時のミリ秒
    DWORD m_StartTime;
    unsigned int m_duration;

    // nodeindexの数によって場合分け
    void IKSolve(int frameNo);
    // CCD-IKによりボーン方向を解決
    void SolveCCDIK(const PMDIK& ik);
    // 余弦定理IKによりボーン方向を解決
    void SolveCosineIK(const PMDIK& ik);
    // LookAt行列によりボーン方向を解決
    void SolveLookAtIK(const PMDIK& ik);


    // ボーンを子の末端まで伝える再帰関数
    void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat, bool flag = false);

    float GetYFromXonBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);
};



