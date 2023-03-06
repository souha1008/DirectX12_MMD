#pragma once
class DepthColor
{
public:
    void CreateResource();
    void CreateVertex();
    void CreateDepthColorPipeline();
    void PreDepthDraw();
    void DepthDraw();
    void PostDepthDraw();

    void Init()
    {
        CreateResource();
        CreateVertex();
        CreateDepthColorPipeline();
    }


private:
    ComPtr<ID3D12Resource> m_DepthResource;
    ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
    ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
    ComPtr<ID3D12Resource> m_DepthVB;
    D3D12_VERTEX_BUFFER_VIEW m_DepthVBView;
    ComPtr<ID3D12RootSignature> m_DepthRS;
    ComPtr<ID3D12PipelineState> m_Pipeline;
};


