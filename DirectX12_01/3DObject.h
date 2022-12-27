#pragma once

// ���[�h�p�����_��
using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

class Object3D
{
public:

    // ���f�������i�S������j
    HRESULT CreateModel(const char* Filename, const char* Motionname);

    // PMD�w�b�_�[�ǂݍ���
    HRESULT LoadPMDHeader(FILE* file);

    // �o�[�e�b�N�X�o�b�t�@����
    HRESULT CreateVertexBuffer(FILE* file);

    // �C���f�b�N�X�o�b�t�@����
    HRESULT CreateIndexBuffer(FILE* file);

    // �I�u�W�F�N�g�p�萔�o�b�t�@����
    HRESULT CreateTransformCBuffer();

    // �e�N�X�`���f�[�^���[�h
    ID3D12Resource* LoadTextureFromFile(std::string& texPath);

    // �}�e���A������
    HRESULT LoadMaterial(FILE* file, std::string ModelPath);
    HRESULT CreateMaterialBuffer();
    HRESULT CreateMaterialView();

    // �{�[������
    HRESULT LoadPMDBone(FILE* file);

    // VMD�f�[�^�ǂݍ���
    HRESULT LoadVMDData(FILE* file);
    // IK�ǂݍ���
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
        uint16_t boneIdx;   // IK�Ώۂ̃{�[��������
        uint16_t targetIdx; // �^�[�Q�b�g�ɋ߂Â��邽�߂̃{�[���C���f�b�N�X
        uint16_t iterations; // ���s��
        float limit;        // 1�񂠂���̉�]����
        std::vector<uint16_t> nodeIdx;  // �Ԃ̃m�[�h�ԍ�
    }PMDIK;

    // IK�I�m�t�f�[�^
    typedef struct
    {
        uint32_t frameNo;
        std::unordered_map<std::string, bool> ikEnableTable;
    }VMDIKEnable;

    // �V�F�[�_�[�ɓ]������f�[�^
    typedef struct
    {
        XMFLOAT3 diffuse;   // �f�B�t���[�Y�F
        float alpha;        // �f�B�t���[�Y
        XMFLOAT3 specular;  // �X�y�L�����F
        float specularity;   // �X�y�L�����̋���
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
    }TRANSFORM;

    struct BoneNode
    {
        uint32_t boneIdx = 0;    // �{�[���C���f�b�N�X
        uint32_t boneType;
        uint32_t ikParentBone;
        XMFLOAT3 startPos = { 0, 0, 0 };  // �{�[����_�i��]�̒��S�j
        XMFLOAT3 endPos = { 0, 0, 0 };    // �{�[����[�X�i���ۂ̃X�L�j���O�ɂ͗��p���Ȃ��j
        std::vector<BoneNode*>  children;   // �q�m�[�h
    };

    struct KeyFrame
    {
        unsigned int frameNo;   // �A�j���[�V�����J�n����̃t���[����
        XMVECTOR quaternion;    // �N�H�[�^�j�I��
        XMFLOAT3 offset;
        XMFLOAT2 p1, p2;

        KeyFrame(unsigned int fno, XMVECTOR& q, XMFLOAT3& ofst, const XMFLOAT2& ip1, const XMFLOAT2& ip2)
            : frameNo(fno), quaternion(q), offset(ofst), p1(ip1), p2(ip2)
        {}
    };

    ComPtr<ID3D12Resource> VertexBuffer;    // ���_�o�b�t�@�[
    D3D12_VERTEX_BUFFER_VIEW vbView;  // ���_�o�b�t�@�[�r���[
    ComPtr<ID3D12Resource> IndexBuffer; // �C���f�b�N�X�o�b�t�@�[
    D3D12_INDEX_BUFFER_VIEW ibView;  // �C���f�b�N�X�o�b�t�@�[�r���[

    // ���f���p�萔�o�b�t�@
    ComPtr<ID3D12Resource> TransfromConstBuffer;
    ComPtr<ID3D12DescriptorHeap> transformDescHeap;
    XMMATRIX* mappedMatrices;   // ���f���p�萔�o�b�t�@���}�b�v

    // �e�N�X�`���p�o�b�t�@
    ComPtr<ID3D12Resource> TextureBuffer;

    // �}�e���A�����i�[
    std::vector<MATERIAL> material;
    std::vector<ComPtr<ID3D12Resource>> TextureResource;
    std::vector<ComPtr<ID3D12Resource>> sphResource;
    std::vector<ComPtr<ID3D12Resource>> spaResource;
    std::vector<ComPtr<ID3D12Resource>> toonResource;
    SUBSET sub;

    // �}�e���A���p�萔�o�b�t�@
    ComPtr<ID3D12Resource> MaterialConstBuffer;
    ComPtr<ID3D12DescriptorHeap> materialDescHeap;

    // �{�[���}�g���N�X
    std::vector<XMMATRIX> BoneMatrix;

    // ���[�V�����f�[�^
    std::unordered_map<std::string, std::vector<KeyFrame>> MotionData;

    // IK�f�[�^�i�[
    std::vector<PMDIK> pmdIKData;

    // ���[�h�p�����_
    std::map<std::string, LoadLambda_t> m_loadLambdaTable;

    // ���\�[�X�p
    std::map<std::string, ID3D12Resource*> m_resourceTable;

    // �{�[������
    std::map<std::string, BoneNode> m_BoneNodeTable;

    // �C���f�b�N�X���猟�����₷���悤��
    std::vector<std::string> m_BoneNameArray;
    
    //�C���f�b�N�X����m�[�h���������₷���悤�ɂ��Ă���
    std::vector<BoneNode*> m_boneNodeAddressArray;

    std::vector<uint32_t> m_kneeIdxes;

    std::vector<VMDIKEnable> m_IKEnableData;

    // �A�j���[�V�����J�n���̃~���b
    DWORD m_StartTime;
    unsigned int m_duration;

    // nodeindex�̐��ɂ���ďꍇ����
    void IKSolve(int frameNo);
    // CCD-IK�ɂ��{�[������������
    void SolveCCDIK(const PMDIK& ik);
    // �]���藝IK�ɂ��{�[������������
    void SolveCosineIK(const PMDIK& ik);
    // LookAt�s��ɂ��{�[������������
    void SolveLookAtIK(const PMDIK& ik);


    // �{�[�����q�̖��[�܂œ`����ċA�֐�
    void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat, bool flag = false);

    float GetYFromXonBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);
};



