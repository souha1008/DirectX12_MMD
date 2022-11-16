#pragma once
class DX12Renderer
{
public:
    static void Init();     // 初期化
    static void Uninit();   // 終了
    static void Begin();    // 描画開始処理
    static void End();      // コマンドリスト等描画実行処理

    static void EnableDebugLayer();     // 出力にデバッグ情報を表示
    static ID3D12Device* GetDevice() { return m_Device; }
    static ID3D12GraphicsCommandList* GetGraphicsCommandList() { return m_GCmdList; }
    static ID3D12PipelineState* GetPipelineState() { return m_PipelineState; }

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

private:
    static ID3D12Device* m_Device;
    static IDXGIFactory6* m_DXGIFactry;
    static IDXGISwapChain4* m_SwapChain4;
    static ID3D12Resource* m_DepthBuffer;
    static ID3D12DescriptorHeap* m_DSVDescHeap;
    static ID3D12CommandAllocator* m_CmdAllocator;
    static ID3D12GraphicsCommandList* m_GCmdList;
    static ID3D12CommandQueue* m_CmdQueue;
    static ID3D12DescriptorHeap* m_DescHeap;
    static ID3D12RootSignature* m_RootSignature;
    static ID3D12PipelineState* m_PipelineState;
    static std::vector<ID3D12Resource*> m_BackBuffers;

    static ID3DBlob* m_VSBlob;
    static ID3DBlob* m_PSBlob;
    static ID3DBlob* m_ErrorBlob;

    static ID3D12Fence* m_Fence;
    static UINT64 m_FenceVal;

    static D3D12_RESOURCE_BARRIER m_Barrier;

};

