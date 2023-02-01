#pragma once
class FinalResource
{
private:
    ComPtr<ID3D12Resource> m_FinalResource;
    ComPtr<ID3D12DescriptorHeap> m_FinalRTVDescHeap; // �����_�[�^�[�Q�b�g�p
    ComPtr<ID3D12DescriptorHeap> m_FinalSRVDescHeap; // �e�N�X�`���p
    ComPtr<ID3D12Resource> m_FinalVB;
    D3D12_VERTEX_BUFFER_VIEW m_FinalVBView;
    ComPtr<ID3D12RootSignature> m_FinalRootSignature;
    ComPtr<ID3D12PipelineState> m_FinalPipelineState;
    void CreateResouceAndView();
    void CreateVertex();
    void CreatePipeline();

public:
    void Init();
    void PreFinalDraw();
    void PostFinalDraw();
    void FinalDraw();

};