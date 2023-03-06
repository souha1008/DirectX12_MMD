#pragma once
class DX12Renderer
{
public:

    // �\����
    struct LIGHT
    {
        BOOL    Enable;
        BOOL    Dummy[3];
        XMVECTOR    Direction;
        XMFLOAT4    Diffuse;
        XMFLOAT4    Ambient;

        XMFLOAT4X4    ViewMatrix;
        XMFLOAT4X4    ProjMatrix;
    };

    static void Init();     // ������
    static void Uninit();   // �I��
    static void Begin();    // �`��J�n����
    static void Draw3D();   // 3D��ԕ`��ׂ̈̃Z�b�e�B���O
    static void End();      // �R�}���h���X�g���`����s����

    static void EnableDebugLayer();     // �o�͂Ƀf�o�b�O����\��
    static ID3D12Device* GetDevice() { return m_Device.Get(); }
    static ID3D12GraphicsCommandList* GetGraphicsCommandList() { return m_GCmdList.Get(); }
    static ID3D12PipelineState* GetPipelineState() { return m_PipelineState.Get(); }

    static HRESULT CreateFactory();
    static HRESULT CreateDevice();
    static HRESULT CreateCommandQueue();
    static HRESULT CreateCommandList_Allocator();
    static HRESULT CreateSwapChain();
    static HRESULT CreateDepthBuffer();
    static HRESULT CreateDepthBufferView();
    static HRESULT CreateDescripterHeap();
    static HRESULT CreateRenderTargetView();
    static HRESULT CreateFence();
    static HRESULT CreateRootSignature();
    static HRESULT CreatePipelineState();

    static HRESULT CreateSceneConstBuffer();
    static void SetView(XMFLOAT4X4* view) 
    {
        m_MappedSceneMatrix->view = *view;
    }
    static void SetProj(XMFLOAT4X4* proj)
    {
        m_MappedSceneMatrix->proj = *proj;
    }

    static HRESULT CreateLightConstBuffer();
    static HRESULT SetLight(LIGHT light);

    static void BeginDepthShadow()
    {
        // �o�b�N�o�b�t�@�̃C���f�b�N�X���擾
        UINT bbIdx = m_SwapChain4->GetCurrentBackBufferIndex();
        D3D12_CPU_DESCRIPTOR_HANDLE rtvH = m_DescHeap->GetCPUDescriptorHandleForHeapStart();
        rtvH.ptr += bbIdx * m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE dsvH = m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart();
        m_GCmdList.Get()->OMSetRenderTargets(0, NULL, false, &dsvH);
        m_GCmdList.Get()->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    static ID3D12DescriptorHeap* GetRTVDescHeap()
    {
        return m_DescHeap.Get();
    }

    static std::vector<ID3D12Resource*> GetBackBuffers()
    {
        return m_BackBuffers;
    }

    static ID3D12DescriptorHeap* GetDSVDescHeap()
    {
        return m_DSVDescHeap.Get();
    }

    static ID3D12DescriptorHeap* GetDepthSRVDescHeap()
    {
        return m_DepthSRVDescHeap.Get();
    }

    static ID3D12Resource* GetDepthResource()
    {
        return m_BackBufferDepthResource.Get();
    }

private:
    typedef struct
    {
        XMFLOAT4X4 view;
        XMFLOAT4X4 proj;
        XMMATRIX shadow;
        XMFLOAT3 eye;   // �������W
    }SCENEMATRIX;

    static ComPtr<ID3D12Device> m_Device;
    static ComPtr<IDXGIFactory6> m_DXGIFactry;
    static ComPtr<IDXGISwapChain4> m_SwapChain4;
    static ComPtr<ID3D12Resource> m_BackBufferDepthResource;        // �[�x�l�������݃��\�[�X�o�b�t�@�[
    static ComPtr<ID3D12DescriptorHeap> m_DSVDescHeap;  // �[�x�l
    static ComPtr<ID3D12DescriptorHeap> m_DepthSRVDescHeap;
    static ComPtr<ID3D12CommandAllocator> m_CmdAllocator;
    static ComPtr<ID3D12GraphicsCommandList> m_GCmdList;
    static ComPtr<ID3D12CommandQueue> m_CmdQueue;
    static ComPtr<ID3D12DescriptorHeap> m_DescHeap;
    static ComPtr<ID3D12RootSignature> m_RootSignature;
    static ComPtr<ID3D12PipelineState> m_PipelineState;

    static ComPtr<ID3D12Resource> m_SceneConstBuffer;    // �r���[�p�萔�o�b�t�@
    static ComPtr<ID3D12DescriptorHeap> m_sceneDescHeap;  // �r���[�p�f�X�N���v�^�q�[�v
    static SCENEMATRIX* m_MappedSceneMatrix;    // �r���[�p

    static ComPtr<ID3D12Resource> m_LightCBuffer;
    static ComPtr<ID3D12DescriptorHeap> m_LightDescHeap;
    // ���s���C�g�̌���
    static XMFLOAT3 m_ParallelLightVec;
    static LIGHT* m_mapLight;

    static std::vector<ID3D12Resource*> m_BackBuffers;

    static ComPtr<ID3DBlob> m_VSBlob;
    static ComPtr<ID3DBlob> m_PSBlob;
    static ComPtr<ID3DBlob> m_ErrorBlob;

    static ComPtr<ID3D12Fence> m_Fence;
    static UINT64 m_FenceVal;

    static D3D12_RESOURCE_BARRIER m_Barrier;
    

};

class PeraPolygon
{
public:
    void CreatePeraResorce();
    void PrePeraDraw1();
    void PeraDraw1();
    void PostPeraDraw1();
    void PrePeraDraw2();
    void PeraDraw2();
    void PostPeraDraw2();
    void CreatePeraVertex();
    void CreatePeraPipeline();

    std::vector<float> GetGaussianWeights(size_t count, float s);
    void CreateBokehParamResource();

    void Update();
    void ChangePipeline();  // Update���Ŏg��


private:
    ComPtr<ID3D12Resource> m_PeraResource;
    ComPtr<ID3D12Resource> m_PeraResource2;
    ComPtr<ID3D12DescriptorHeap> m_PeraRTVDescHeap; // �����_�[�^�[�Q�b�g�p
    ComPtr<ID3D12DescriptorHeap> m_PeraSRVDescHeap; // �e�N�X�`���p
    ComPtr<ID3D12Resource> m_PeraVB;
    D3D12_VERTEX_BUFFER_VIEW m_PeraVBView;
    ComPtr<ID3D12RootSignature> m_PeraRootSignature;

    // �ʏ�`��p�C�v���C��
    ComPtr<ID3D12PipelineState> m_NormalPipelineState;
    // �|�X�g�G�t�F�N�g���Ƃ̃p�C�v���C��
    ComPtr<ID3D12PipelineState> m_BlurPipeline;
    ComPtr<ID3D12PipelineState> m_EmbossPipeline;
    ComPtr<ID3D12PipelineState> m_OutlinePipeline;
    ComPtr<ID3D12PipelineState> m_GaussianHPipeline;
    ComPtr<ID3D12PipelineState> m_GaussianVPipeline;
    ComPtr<ID3D12PipelineState> m_MosaicPipeline;

    // ���ݎg�p���̃p�C�v���C��
    // ���̃p�C�v���C���͂�����G�t�F�N�g�̗ʂ����쐬����
    ComPtr<ID3D12PipelineState> m_NowUsePipelineState;
    ComPtr<ID3D12PipelineState> m_NowUsePipelineState2;
    
    ComPtr<ID3D12Resource> m_BokehParamResource;  // �K�E�X���z�E�F�C�g�l

};



