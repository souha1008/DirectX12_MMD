#pragma once
class DX12Renderer
{
public:

    // 構造体
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

    static void Init();     // 初期化
    static void Uninit();   // 終了
    static void Begin();    // 描画開始処理
    static void Draw3D();   // 3D空間描画の為のセッティング
    static void End();      // コマンドリスト等描画実行処理

    static void EnableDebugLayer();     // 出力にデバッグ情報を表示
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
        // バックバッファのインデックスを取得
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

private:
    typedef struct
    {
        XMFLOAT4X4 view;
        XMFLOAT4X4 proj;
        XMMATRIX shadow;
        XMFLOAT3 eye;   // 視線座標
    }SCENEMATRIX;

    static ComPtr<ID3D12Device> m_Device;
    static ComPtr<IDXGIFactory6> m_DXGIFactry;
    static ComPtr<IDXGISwapChain4> m_SwapChain4;
    static ComPtr<ID3D12Resource> m_DepthBuffer;
    static ComPtr<ID3D12DescriptorHeap> m_DSVDescHeap;
    static ComPtr<ID3D12CommandAllocator> m_CmdAllocator;
    static ComPtr<ID3D12GraphicsCommandList> m_GCmdList;
    static ComPtr<ID3D12CommandQueue> m_CmdQueue;
    static ComPtr<ID3D12DescriptorHeap> m_DescHeap;
    static ComPtr<ID3D12RootSignature> m_RootSignature;
    static ComPtr<ID3D12PipelineState> m_PipelineState;

    static ComPtr<ID3D12Resource> m_SceneConstBuffer;    // ビュー用定数バッファ
    static ComPtr<ID3D12DescriptorHeap> m_sceneDescHeap;  // ビュー用デスクリプタヒープ
    static SCENEMATRIX* m_MappedSceneMatrix;    // ビュー用

    static ComPtr<ID3D12Resource> m_LightCBuffer;
    static ComPtr<ID3D12DescriptorHeap> m_LightDescHeap;
    // 並行ライトの向き
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
    void PrePeraDraw();
    void PeraDraw1();
    void PostPeraDraw();
    void CreatePeraVertex();
    void CreatePeraPipeline();

    std::vector<float> GetGaussianWeights(size_t count, float s);
    void CreateBokehParamResource();


private:
    ComPtr<ID3D12Resource> m_PeraResource;
    ComPtr<ID3D12Resource> m_PeraResource2;
    ComPtr<ID3D12DescriptorHeap> m_PeraRTVDescHeap; // レンダーターゲット用
    ComPtr<ID3D12DescriptorHeap> m_PeraSRVDescHeap; // テクスチャ用
    ComPtr<ID3D12Resource> m_PeraVB;
    D3D12_VERTEX_BUFFER_VIEW m_PeraVBView;
    ComPtr<ID3D12RootSignature> m_PeraRootSignature;
    ComPtr<ID3D12PipelineState> m_PeraPipelineState;
    ComPtr<ID3D12PipelineState> m_PeraPipelineState2;
    
    ComPtr<ID3D12Resource> m_BokehParamResource;  // ガウス分布ウェイト値

};

