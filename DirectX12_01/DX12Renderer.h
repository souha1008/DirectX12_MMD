#pragma once
class DX12Renderer
{
public:
    static void Init();     // ������
    static void Uninit();   // �I��
    static void Begin();    // �`��J�n����
    static void End();      // �R�}���h���X�g���`����s����

    static void EnableDebugLayer();     // �o�͂Ƀf�o�b�O����\��
    static ComPtr<ID3D12Device> GetDevice() { return m_Device.Get(); }
    static ComPtr<ID3D12GraphicsCommandList> GetGraphicsCommandList() { return m_GCmdList.Get(); }
    static ComPtr<ID3D12PipelineState> GetPipelineState() { return m_PipelineState.Get(); }

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

    static std::vector<ID3D12Resource*> m_BackBuffers;

    static ComPtr<ID3DBlob> m_VSBlob;
    static ComPtr<ID3DBlob> m_PSBlob;
    static ComPtr<ID3DBlob> m_ErrorBlob;

    static ComPtr<ID3D12Fence> m_Fence;
    static UINT64 m_FenceVal;

    static D3D12_RESOURCE_BARRIER m_Barrier;

};

