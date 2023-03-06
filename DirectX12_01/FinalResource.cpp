#include "framework.h"
#include "DX12Renderer.h"
#include "FinalResource.h"
#include "Helper.h"
#include "input.h"

void FinalResource::Init()
{
	CreateResouceAndView();
	CreateVertex();
	CreatePipeline();
}

void FinalResource::CreateResouceAndView()
{
    // 作成済みのヒープ情報を使って作る
    D3D12_DESCRIPTOR_HEAP_DESC hd = DX12Renderer::GetRTVDescHeap()->GetDesc();

    // 使っているバックバッファーの情報を利用する
    auto bbuff = DX12Renderer::GetBackBuffers();

    D3D12_RESOURCE_DESC rd = bbuff[0]->GetDesc();

	D3D12_HEAP_PROPERTIES hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// レンダリング時のクリア値と同じ値
	float clsClr[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clsClr);

	// リソース作成
	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_FinalResource.ReleaseAndGetAddressOf())
	);

	// RTV用ヒープを作る
	hd.NumDescriptors = 1;
	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&hd,
		IID_PPV_ARGS(m_FinalRTVDescHeap.ReleaseAndGetAddressOf())
	);
	D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};
	rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	auto handle = m_FinalRTVDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	// RTVを作る
	DX12Renderer::GetDevice()->CreateRenderTargetView(
		m_FinalResource.Get(),
		&rtvd,
		handle
	);

	// SRV用ヒープを作る
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&hd,
		IID_PPV_ARGS(m_FinalSRVDescHeap.ReleaseAndGetAddressOf())
	);
	handle = m_FinalSRVDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvd.Format = rtvd.Format;
	srvd.Texture2D.MipLevels = 1;
	srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	// SRVを作る
	DX12Renderer::GetDevice()->CreateShaderResourceView(
		m_FinalResource.Get(),
		&srvd,
		handle
	);
}

void FinalResource::CreateVertex()
{
	typedef struct
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	}PeraVertex;

	PeraVertex pv[4] = {
		{{-1, -1, 0.1f}, {0, 1}},	// 左下
		{{-1, 1, 0.1f}, {0, 0}},	// 左上
		{{1, -1, 0.1f}, {1, 1}},	// 右下
		{{1, 1, 0.1f}, {1, 0}}		// 右上
	};

	auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto rd = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pv));

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_FinalVB.ReleaseAndGetAddressOf())
	);
	m_FinalVBView.BufferLocation = m_FinalVB.Get()->GetGPUVirtualAddress();
	m_FinalVBView.SizeInBytes = sizeof(pv);
	m_FinalVBView.StrideInBytes = sizeof(PeraVertex);

	PeraVertex* mappedPera = nullptr;
	m_FinalVB.Get()->Map(0, nullptr, (void**)&mappedPera);
	std::copy(std::begin(pv), std::end(pv), mappedPera);
	m_FinalVB.Get()->Unmap(0, nullptr);
}

void FinalResource::CreatePipeline()
{
	HRESULT hr;

	D3D12_DESCRIPTOR_RANGE dr = {};
	dr.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	dr.BaseShaderRegister = 0;
	dr.NumDescriptors = 1;

	D3D12_ROOT_PARAMETER rp = {};
	rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp.DescriptorTable.pDescriptorRanges = &dr;
	rp.DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC ssd = CD3DX12_STATIC_SAMPLER_DESC(0);
	ssd.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	ssd.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ComPtr<ID3DBlob> rs;
	ComPtr<ID3DBlob> errBlob;

	hr = D3DCompileFromFile(
		L"PeraVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"vs", "vs_5_0", 0, 0,
		vs.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);

	hr = D3DCompileFromFile(
		L"PeraPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"ps", "ps_5_0", 0, 0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);

	D3D12_INPUT_ELEMENT_DESC layout[2] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.NumParameters = 1;
	rsd.pParameters = &rp;
	rsd.NumStaticSamplers = 1;
	rsd.pStaticSamplers = &ssd;
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	hr = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		rs.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	hr = DX12Renderer::GetDevice()->CreateRootSignature(
		0,
		rs.Get()->GetBufferPointer(),
		rs.Get()->GetBufferSize(),
		IID_PPV_ARGS(m_FinalRootSignature.ReleaseAndGetAddressOf())
	);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
	gpsd.InputLayout.NumElements = _countof(layout);
	gpsd.InputLayout.pInputElementDescs = layout;
	gpsd.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	gpsd.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	gpsd.DepthStencilState.DepthEnable = false;
	gpsd.DepthStencilState.StencilEnable = false;
	gpsd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsd.NumRenderTargets = 1;
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsd.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpsd.SampleDesc.Count = 1;
	gpsd.SampleDesc.Quality = 0;
	gpsd.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gpsd.pRootSignature = m_FinalRootSignature.Get();
	hr = DX12Renderer::GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(m_FinalPipelineState.ReleaseAndGetAddressOf()));
}

void FinalResource::PreFinalDraw()
{
	// リソース切り替え
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_FinalResource.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	DX12Renderer::GetGraphicsCommandList()->ResourceBarrier(
		1,
		&barrier
	);
	auto rtvHeapHandle = m_FinalRTVDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	auto dsvHeapHandle = DX12Renderer::GetDSVDescHeap()->GetCPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->OMSetRenderTargets(
		1,
		&rtvHeapHandle,
		false,
		&dsvHeapHandle
	);
	float clsClr[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	DX12Renderer::GetGraphicsCommandList()->ClearRenderTargetView(rtvHeapHandle, clsClr, 0, nullptr);
	
	
}

void FinalResource::PostFinalDraw()
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_FinalResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	DX12Renderer::GetGraphicsCommandList()->ResourceBarrier(
		1,
		&barrier
	);
}

void FinalResource::FinalDraw()
{
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootSignature(m_FinalRootSignature.Get());
	DX12Renderer::GetGraphicsCommandList()->SetPipelineState(m_FinalPipelineState.Get());
	DX12Renderer::GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	DX12Renderer::GetGraphicsCommandList()->IASetVertexBuffers(0, 1, &m_FinalVBView);

	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, m_FinalSRVDescHeap.GetAddressOf());
	auto srvhandle = m_FinalSRVDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(0, srvhandle);
	DX12Renderer::GetGraphicsCommandList()->DrawInstanced(4, 1, 0, 0);
}

