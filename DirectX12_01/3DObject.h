#pragma once

using namespace DirectX;

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
    XMFLOAT4X4 viewproj;   // �r���[�ƃv���W�F�N�V�����̍����s��
}MATRIXDATA;

typedef struct
{
    ID3D12Resource* VertexBuffer;
    ID3D12Resource* IndexBuffer;
    ID3D12Resource* ConstBuffer;
    ID3D12Resource* TextureBuffer;
    ID3D12Resource* MaterialBuffer;

    std::vector<ID3D12Resource*> TextureResource;
    std::vector<ID3D12Resource*> sphResource;

    D3D12_VERTEX_BUFFER_VIEW vbView;  // ���_�o�b�t�@�[�r���[
    D3D12_INDEX_BUFFER_VIEW ibView;  // �C���f�b�N�X�o�b�t�@�[�r���[

    ID3D12DescriptorHeap* materialDescHeap;
    ID3D12DescriptorHeap* basicDescHeap;

    TexMetadata MetaData;
    MATRIXDATA* MapMatrix;

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
    ID3D12Resource* CreateWhiteTexture();
    std::string GetExtension(const std::string& path);
    std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');


    void UnInit(MODEL_DX12* Model);

};

