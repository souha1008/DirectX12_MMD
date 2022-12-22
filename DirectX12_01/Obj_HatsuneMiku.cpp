#include "framework.h"
#include "DX12Renderer.h"
#include "Obj_HatsuneMiku.h"

void Obj_HatsuneMiku::Init()
{
    m_Object = new Object3D();

    HRESULT hr = m_Object->CreateModel("Assets/Model/シンプルモデル/初音ミク.pmd"
        , "Assets/VMD/motion.vmd"
        , &m_Model);

    if (FAILED(hr))
    {
        return;
    }

    m_Object->PlayAnimation();
}

void Obj_HatsuneMiku::Uninit()
{
    m_Object->UnInit(&m_Model);
}

void Obj_HatsuneMiku::Update()
{
    m_Object->MotionUpdate(&m_Model);
}

void Obj_HatsuneMiku::Draw()
{
	// プリミティブトポロジ設定
	DX12Renderer::GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点バッファービューセット
	DX12Renderer::GetGraphicsCommandList()->IASetVertexBuffers(0, 1, &m_Model.vbView);

	// インデックスバッファービューセット
	DX12Renderer::GetGraphicsCommandList()->IASetIndexBuffer(&m_Model.ibView);

	ID3D12DescriptorHeap* trans_dh[] = { m_Model.transformDescHeap.Get() };
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, trans_dh);
	auto transform_handle = m_Model.transformDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(1, transform_handle);

	// マテリアルのディスクリプタヒープセット
	ID3D12DescriptorHeap* mdh[] = { m_Model.materialDescHeap.Get() };
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, mdh);

	// マテリアル用ディスクリプタヒープの先頭アドレスを取得
	auto material_handle = m_Model.materialDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	// CBVとSRVとSRVとSRVとSRVで1マテリアルを描画するのでインクリメントサイズを５倍にする
	auto cbvsize = DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

	for (auto& m : m_Model.material)
	{
		DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(2, material_handle);
		DX12Renderer::GetGraphicsCommandList()->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

		// ヒープポインタとインデックスを次に進める
		material_handle.ptr += cbvsize;
		idxOffset += m.indicesNum;
	}
}
