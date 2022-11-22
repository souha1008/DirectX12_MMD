#pragma once

using namespace DirectX;
// ���[�h�p�����_��
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
    float version;          // ��F00 00 80 3F == 1.00
    char model_name[20];    // ���f����
    char comment[256];      // ���f���R�����g
}PMDHeader;

#pragma pack(push, 1)
typedef struct
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
    uint16_t boneNo[2];   // �{�[���ԍ�
    uint8_t boneWeight;   // �{�[���e���x
    uint8_t edgeFlag;     // �֊s���t���O 
    uint16_t dummy;
}PMDVertex;
#pragma pack(pop)

#pragma pack(1) // ��������1�o�C�g�p�b�L���O�ƂȂ�A�A���C�����g�͔������Ȃ�
typedef struct
{
    XMFLOAT3 diffuse;   // �f�B�t���[�Y�F
    float alpha;        // �f�B�t���[�Y
    float specularity;   // �X�y�L�����̋���
    XMFLOAT3 specular;  // �X�y�L�����F
    XMFLOAT3 ambient;   // �A���r�G���g�F
    unsigned char toonIdx;  // �g�D�[���ԍ�
    unsigned char edgeFlg;  // �}�e���A�����Ƃ̗֊s���t���O

    // 2�o�C�g�̃p�f�B���O������
    unsigned int indicesNum;
    char texFilePath[20];   // �e�N�X�`���t�@�C���p�X + ��
}PMDMaterial;   // �p�f�B���O���������Ȃ�����70�o�C�g
#pragma pack()  // �p�b�L���O�w�������

#pragma pack(1)
typedef struct
{
    char boneName[20];          // �{�[����
    unsigned short parentNo;    // �e�{�[���ԍ�
    unsigned short nextNo;      // ��[�̃{�[���ԍ�
    unsigned char type;         // �{�[�����
    unsigned short  ikBoneNo;   // IK�{�[���ԍ�
    XMFLOAT3 pos;               // �{�[���̊�_���W
}PMDBone;

// �V�F�[�_�[�ɓ]������f�[�^
typedef struct
{
    XMFLOAT3 diffuse;   // �f�B�t���[�Y�F
    float alpha;        // �f�B�t���[�Y
    float specularity;   // �X�y�L�����̋���
    XMFLOAT3 specular;  // �X�y�L�����F
    XMFLOAT3 ambient;   // �A���r�G���g�F

}MaterialForHlsl;

// ����ȊO�̃}�e���A���f�[�^
typedef struct
{
    std::string texPath;    // �e�N�X�`���p�X
    int toonIdx;            // �g�D�[���ԍ�
    bool edgeFlg;           // �}�e���A�����Ƃ̗֊s���t���O

}AdditionalMaterial;

// �S�̂��܂Ƃ߂�f�[�^
typedef struct
{
    unsigned int indicesNum;
    MaterialForHlsl material;
    AdditionalMaterial additional;
}MATERIAL;

typedef struct
{
    XMFLOAT4X4 world; // ���f���{�̂���]��������ړ��������肷��
    XMFLOAT4X4 view;
    XMFLOAT4X4 proj;
    XMFLOAT3 eye;   // �������W
}SCENEMATRIX;

struct BoneNode
{
    int boneIdx;    // �{�[���C���f�b�N�X
    XMFLOAT3 startPos;  // �{�[����_�i��]�̒��S�j
    XMFLOAT3 endPos;    // �{�[����[�X�i���ۂ̃X�L�j���O�ɂ͗��p���Ȃ��j
    std::vector<BoneNode*>  children;   // �q�m�[�h
};



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

    std::vector<XMMATRIX> BoneMatrix;

    D3D12_VERTEX_BUFFER_VIEW vbView;  // ���_�o�b�t�@�[�r���[
    D3D12_INDEX_BUFFER_VIEW ibView;  // �C���f�b�N�X�o�b�t�@�[�r���[

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
    // ���f�������i�S������j
    HRESULT CreateModel(const char* Filename, MODEL_DX12* Model);

    // �o�[�e�b�N�X�o�b�t�@����
    HRESULT CreateVertexBuffer(MODEL_DX12* Model, std::vector<PMDVertex> vertices);
    HRESULT SettingVertexBufferView(MODEL_DX12* Model, std::vector<PMDVertex> vertices, size_t pmdVertex_size);

    // �C���f�b�N�X�o�b�t�@����
    HRESULT CreateIndexBuffer(MODEL_DX12* Model, std::vector<unsigned short> index);
    HRESULT SettingIndexBufferView(MODEL_DX12* Model, std::vector<unsigned short> index);

    // �萔�o�b�t�@����
    HRESULT CreateConstBuffer(MODEL_DX12* Model);
    HRESULT SettingConstBufferView(D3D12_CPU_DESCRIPTOR_HANDLE* handle, MODEL_DX12* Model);

    // �f�B�X�N���v�^�q�[�v����
    HRESULT CreateBasicDescriptorHeap(MODEL_DX12* Model);

    // �e�N�X�`���f�[�^����
    HRESULT CreateTextureData(MODEL_DX12* Model);
    // �e�N�X�`���f�[�^���[�h
    ID3D12Resource* LoadTextureFromFile(MODEL_DX12* Model, std::string& texPath);

    // �V�F�[�_�[���\�[�X�r���[����
    HRESULT CreateShaderResourceView(MODEL_DX12* Model);

    // �}�e���A������
    HRESULT LoadMaterial(FILE* file, MODEL_DX12* Model, std::string ModelPath);
    HRESULT CreateMaterialBuffer(MODEL_DX12* Model);
    HRESULT CreateMaterialView(MODEL_DX12* Model);

    // �{�[������
    HRESULT CreateBone(FILE* file, MODEL_DX12* Model);

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
    // ���[�h�p�����_
    std::map<std::string, LoadLambda_t> m_loadLambdaTable;

    // ���\�[�X�p
    std::map<std::string, ID3D12Resource*> m_resourceTable;

    // �{�[������
    std::map<std::string, BoneNode> m_BoneNodeTable;

};

