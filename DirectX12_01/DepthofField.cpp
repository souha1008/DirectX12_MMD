#include "framework.h"
#include "DX12Renderer.h"
#include "DepthofField.h"
#include "Helper.h"

#define POS 0.5f

void DepthColor::CreateResource()
{
	// リソース作り
	// リソースディスク
	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D(						// d3dx12
		DXGI_FORMAT_R32_TYPELESS,		// 深度値書き込み用フォーマット
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		1,							//	テクスチャ配列でも3Dテクスチャでもない
		0,							// ミップマップは1
		1,							// サンプルは1ピクセル当たり1つ
		0,							// クオリティは0
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	CD3DX12_HEAP_PROPERTIES depth_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, 0, 0);

	CD3DX12_CLEAR_VALUE depth_cv = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);		// d3dx12

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&depth_hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&depth_cv,
		IID_PPV_ARGS(&m_DepthResource)
	);

	// ビュー作り
	// 深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};

	dhd.NumDescriptors = 1;	// 深度バッファーは１つ
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// デプスステンシャルビューとして使う
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dhd.NodeMask = 0;
	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(m_DSVHeap.ReleaseAndGetAddressOf()));

	// 深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = {};
	dsvd.Format = DXGI_FORMAT_D32_FLOAT;	//深度値に32ビットfloat使用
	dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2Dテクスチャ
	dsvd.Flags = D3D12_DSV_FLAG_NONE;
	dsvd.Texture2D.MipSlice = 0;

	DX12Renderer::GetDevice()->CreateDepthStencilView(
		m_DepthResource.Get(),
		&dsvd,
		m_DSVHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// 深度バッファー見る用SRVヒープ作成
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&dhd,
		IID_PPV_ARGS(m_SRVHeap.ReleaseAndGetAddressOf())
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvd.Format = DXGI_FORMAT_R32_FLOAT;
	srvd.Texture2D.MipLevels = 1;
	srvd.Texture2D.MostDetailedMip = 0;
	srvd.Texture2D.PlaneSlice = 0;
	srvd.Texture2D.ResourceMinLODClamp = 0.0f;
	srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto handle = m_SRVHeap.Get()->GetCPUDescriptorHandleForHeapStart();

	// SRVを作る
	DX12Renderer::GetDevice()->CreateShaderResourceView(
		m_DepthResource.Get(),
		&srvd,
		handle
	);

	
}

void DepthColor::CreateVertex()
{
	typedef struct
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	}PeraVertex;

	PeraVertex pv[4] = {
	{{-POS, -POS, 0.1f}, {0, 1}},	// 左下
	{{-POS, POS, 0.1f}, {0, 0}},	// 左上
	{{POS, -POS, 0.1f}, {1, 1}},	// 右下
	{{POS, POS, 0.1f}, {1, 0}}		// 右上
	};

	auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto rd = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pv));

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_DepthVB.ReleaseAndGetAddressOf())
	);

	m_DepthVBView.BufferLocation = m_DepthVB.Get()->GetGPUVirtualAddress();
	m_DepthVBView.SizeInBytes = sizeof(pv);
	m_DepthVBView.StrideInBytes = sizeof(PeraVertex);

	PeraVertex* mapped = nullptr;
	m_DepthVB.Get()->Map(0, nullptr, (void**)&mapped);
	std::copy(std::begin(pv), std::end(pv), mapped);
	m_DepthVB.Get()->Unmap(0, nullptr);

}

void DepthColor::CreateDepthColorPipeline()
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
		IID_PPV_ARGS(m_DepthRS.ReleaseAndGetAddressOf())
	);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
	gpsd.InputLayout.NumElements = _countof(layout);
	gpsd.InputLayout.pInputElementDescs = layout;
	gpsd.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	gpsd.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	
	// デプスステンシルステートの設定
	gpsd.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	gpsd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsd.NumRenderTargets = 1;
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpsd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsd.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpsd.SampleDesc.Count = 1;
	gpsd.SampleDesc.Quality = 0;
	gpsd.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gpsd.pRootSignature = m_DepthRS.Get();

	// 通常描画パイプライン
	hr = DX12Renderer::GetDevice()->CreateGraphicsPipelineState(&gpsd,
		IID_PPV_ARGS(m_Pipeline.ReleaseAndGetAddressOf()));

}

void DepthColor::PreDepthDraw()
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_DepthResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
	DX12Renderer::GetGraphicsCommandList()->ResourceBarrier(1, &barrier);
	auto dsvHeapHandle = m_DSVHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->ClearDepthStencilView(dsvHeapHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	DX12Renderer::GetGraphicsCommandList()->OMSetRenderTargets(
		0,
		nullptr,
		false,
		&dsvHeapHandle
	);

}

void DepthColor::DepthDraw()
{
	auto heap = m_SRVHeap.Get();
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, &heap);
	auto handle = heap->GetGPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(0, handle);

	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootSignature(m_DepthRS.Get());
	DX12Renderer::GetGraphicsCommandList()->SetPipelineState(m_Pipeline.Get());
	DX12Renderer::GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	DX12Renderer::GetGraphicsCommandList()->IASetVertexBuffers(0, 1, &m_DepthVBView);

	DX12Renderer::GetGraphicsCommandList()->DrawInstanced(4, 1, 0, 0);

}

void DepthColor::PostDepthDraw()
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_DepthResource.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);

	DX12Renderer::GetGraphicsCommandList()->ResourceBarrier(1, &barrier);
}
