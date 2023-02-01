#include "framework.h"
#include "DX12Renderer.h"
#include "Helper.h"
#include "input.h"

#define PS_ENTRYPOINT	"ps"

// staticメンバ変数
ComPtr<ID3D12Device> DX12Renderer::m_Device = nullptr;
ComPtr<IDXGIFactory6> DX12Renderer::m_DXGIFactry = nullptr;
ComPtr<IDXGISwapChain4> DX12Renderer::m_SwapChain4 = nullptr;
ComPtr<ID3D12Resource> DX12Renderer::m_DepthBuffer = nullptr;
ComPtr<ID3D12DescriptorHeap> DX12Renderer::m_DSVDescHeap = nullptr;
ComPtr<ID3D12CommandAllocator> DX12Renderer::m_CmdAllocator = nullptr;
ComPtr<ID3D12GraphicsCommandList> DX12Renderer::m_GCmdList = nullptr;
ComPtr<ID3D12CommandQueue> DX12Renderer::m_CmdQueue = nullptr;
ComPtr<ID3D12DescriptorHeap> DX12Renderer::m_DescHeap = nullptr;
ComPtr<ID3D12RootSignature> DX12Renderer::m_RootSignature = nullptr;
ComPtr<ID3D12PipelineState> DX12Renderer::m_PipelineState = nullptr;
ComPtr<ID3D12Fence> DX12Renderer::m_Fence = nullptr;
ComPtr<ID3DBlob> DX12Renderer::m_VSBlob = nullptr;
ComPtr<ID3DBlob> DX12Renderer::m_PSBlob = nullptr;
ComPtr<ID3DBlob> DX12Renderer::m_ErrorBlob = nullptr;
ComPtr<ID3D12Resource> DX12Renderer::m_SceneConstBuffer = nullptr;
ComPtr<ID3D12DescriptorHeap> DX12Renderer::m_sceneDescHeap = nullptr;
DX12Renderer::SCENEMATRIX* DX12Renderer::m_MappedSceneMatrix = nullptr;
ComPtr<ID3D12Resource> DX12Renderer::m_LightCBuffer = nullptr;
ComPtr<ID3D12DescriptorHeap> DX12Renderer::m_LightDescHeap = nullptr;
XMFLOAT3 DX12Renderer::m_ParallelLightVec = {1, -1, 1};
DX12Renderer::LIGHT* DX12Renderer::m_mapLight = nullptr;
UINT64 DX12Renderer::m_FenceVal = 0;
D3D12_RESOURCE_BARRIER DX12Renderer::m_Barrier = {};
std::vector<ID3D12Resource*> DX12Renderer::m_BackBuffers;

// グローバル変数
CD3DX12_VIEWPORT g_Viewport = {};
CD3DX12_RECT g_Scissorect = {};

void DX12Renderer::Init()
{
#ifdef _DEBUG
	// デバッグレイヤーオン
	EnableDebugLayer();
#endif // _DEBUG

	// ファクトリー生成
	HRESULT hr = CreateFactory();
	// デバイス生成
	hr = CreateDevice();
	// コマンドキュー生成
	hr = CreateCommandQueue();
	// コマンドリスト＆アロケーター生成
	hr = CreateCommandList_Allocator();
	// スワップチェーン生成
	hr = CreateSwapChain();
	// 深度バッファー生成
	hr = CreateDepthBuffer();
	// デプスステンシルビュー生成
	hr = CreateDepthBufferView();
	// ディスクリプタヒープ生成
	hr = CreateDescripterHeap();
	// レンダーターゲットビュー生成
	hr = CreateRenderTargetView();
	// フェンス生成
	hr = CreateFence();
	// ルートシグネチャ生成
	hr = CreateRootSignature();
	// パイプラインステート作成
	hr = CreatePipelineState();

	// ビュープロジェクション作成
	hr = CreateSceneConstBuffer();

	// ライト作成
	hr = CreateLightConstBuffer();

	// ビューポート
	g_Viewport = CD3DX12_VIEWPORT(m_BackBuffers[0]);	// バックバッファーから自動計算

	// 切り抜き
	g_Scissorect = CD3DX12_RECT(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

}

void DX12Renderer::Uninit()
{
	m_SceneConstBuffer.ReleaseAndGetAddressOf();
	m_sceneDescHeap.ReleaseAndGetAddressOf();
	m_LightCBuffer.ReleaseAndGetAddressOf();
	m_LightDescHeap.ReleaseAndGetAddressOf();

	m_PipelineState.ReleaseAndGetAddressOf();
	m_RootSignature.ReleaseAndGetAddressOf();
	m_DescHeap.ReleaseAndGetAddressOf();
	m_SwapChain4.ReleaseAndGetAddressOf();
	m_CmdAllocator.ReleaseAndGetAddressOf();
	m_GCmdList.ReleaseAndGetAddressOf();
	m_CmdQueue.ReleaseAndGetAddressOf();
	m_Device.ReleaseAndGetAddressOf();
	m_DXGIFactry.ReleaseAndGetAddressOf();
}

void DX12Renderer::Begin()
{
	// バックバッファのインデックスを取得
	UINT bbIdx = m_SwapChain4->GetCurrentBackBufferIndex();

	//m_Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//m_Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//m_Barrier.Transition.pResource = m_BackBuffers[bbIdx];
	//m_Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//m_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//m_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	CD3DX12_RESOURCE_BARRIER tmp = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BackBuffers[bbIdx], 
		D3D12_RESOURCE_STATE_PRESENT, 
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_GCmdList->ResourceBarrier(1, &tmp);

	// レンダーターゲット指定
	D3D12_CPU_DESCRIPTOR_HANDLE rtvH = m_DescHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvH = m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart();
	m_GCmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

	// 画面クリア
	FLOAT clear_color[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	m_GCmdList->ClearRenderTargetView(rtvH, clear_color, 0, nullptr);
	m_GCmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);






}

void DX12Renderer::Draw3D()
{
	// ルートシグネチャセット
	m_GCmdList->SetGraphicsRootSignature(m_RootSignature.Get());

	// パイプラインセット
	m_GCmdList->SetPipelineState(m_PipelineState.Get());

	// ビュー
	ID3D12DescriptorHeap* scene_dh[] = { m_sceneDescHeap.Get() };
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, scene_dh);
	auto scene_handle = m_sceneDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(0, scene_handle);

	// ライトのデスクリプターセット
	ID3D12DescriptorHeap* light_dh[] = { m_LightDescHeap.Get() };
	m_GCmdList.Get()->SetDescriptorHeaps(1, light_dh);
	auto light_handle = m_LightDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	m_GCmdList.Get()->SetGraphicsRootDescriptorTable(3, light_handle);

	// ビューポートセット
	m_GCmdList->RSSetViewports(1, &g_Viewport);
	m_GCmdList->RSSetScissorRects(1, &g_Scissorect);
}

void DX12Renderer::End()
{
	
	// バックバッファのインデックスを取得
	UINT bbIdx = m_SwapChain4->GetCurrentBackBufferIndex();
	// 待機状態入れ替え
	//m_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//m_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	CD3DX12_RESOURCE_BARRIER tmp = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BackBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_GCmdList->ResourceBarrier(1, &tmp);

	// 命令クローズ
	m_GCmdList->Close();

	// コマンドリストの実行
	ID3D12CommandList* cmdlist[] = { m_GCmdList.Get() };
	m_CmdQueue->ExecuteCommandLists(1, cmdlist);

	// 待ち
	m_CmdQueue->Signal(m_Fence.Get(), ++m_FenceVal);
	if (m_Fence->GetCompletedValue() != m_FenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		m_Fence->SetEventOnCompletion(m_FenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	m_CmdAllocator->Reset();
	m_GCmdList->Reset(m_CmdAllocator.Get(), m_PipelineState.Get());

	// フリップ
	m_SwapChain4->Present(1, 0);

}

void DX12Renderer::EnableDebugLayer()
{
	ID3D12Debug1* debugLayer = nullptr;

	auto g_Result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer();
	//debugLayer->SetEnableGPUBasedValidation(true);
	debugLayer->Release();
}

HRESULT DX12Renderer::CreateFactory()
{
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(m_DXGIFactry.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT DX12Renderer::CreateDevice()
{
	HRESULT hr = S_OK;

	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// 特定の名前を持つアダプターオブジェクトが入る
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; m_DXGIFactry->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	// Descriptionメンバー変数から、アダプターの名前を見つける
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);

		std::wstring strDesc = adesc.Description;

		/// 探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	D3D_FEATURE_LEVEL FeatureLevel;
	for (auto l : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf())) == S_OK)
		{
			hr = S_OK;
			FeatureLevel = l;
			break;
		}
	}

	return hr;
}

HRESULT DX12Renderer::CreateCommandQueue()
{
	HRESULT hr;

	///  COMMAND_QUEUEの設定
	D3D12_COMMAND_QUEUE_DESC qDesc = {};
	qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	/// タイムアウト無し
	qDesc.NodeMask = 0;	/// アダプターを一つしか使わないときは０
	qDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	/// プライオリティは特に指定なし
	qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	/// コマンドリストと合わせる
	hr = m_Device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(m_CmdQueue.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT DX12Renderer::CreateCommandList_Allocator()
{
	HRESULT hr;

	/// コマンドアロケータ生成
	hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_CmdAllocator.ReleaseAndGetAddressOf()));

	/// コマンドリスト生成
	hr = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CmdAllocator.Get(), nullptr, IID_PPV_ARGS(m_GCmdList.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT DX12Renderer::CreateSwapChain()
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC1 swapDesc = {};

	swapDesc.Width = SCREEN_WIDTH;
	swapDesc.Height = SCREEN_HEIGHT;
	swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.Stereo = false;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapDesc.BufferCount = 2;
	swapDesc.Scaling = DXGI_SCALING_STRETCH;		/// バックバッファーは伸び縮み可能
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	/// フリップ後は速やかに破棄
	swapDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;	/// 特に指定なし
	swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	/// ウィンドウ　フルスクリーン切替可能

	/// SwapChain生成
	hr = m_DXGIFactry->CreateSwapChainForHwnd(
		m_CmdQueue.Get(),
		GetWindow(),
		&swapDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)m_SwapChain4.ReleaseAndGetAddressOf()
	);
	return hr;
}

HRESULT DX12Renderer::CreateDepthBuffer()
{
	// 深度バッファーの作成

	// 深度バッファー用リソース設定
	//D3D12_RESOURCE_DESC rd = {};
	//rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2次元のテクスチャデータ
	//rd.Width = SCREEN_WIDTH;
	//rd.Height = SCREEN_HEIGHT;
	//rd.DepthOrArraySize = 1;	//	テクスチャ配列でも3Dテクスチャでもない
	//rd.Format = DXGI_FORMAT_D32_FLOAT;	// 深度値書き込み用フォーマット
	//rd.SampleDesc.Count = 1;	// サンプルは1ピクセル当たり1つ
	//rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// デプスステンシルとして使用

	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D(						// d3dx12
		DXGI_FORMAT_D32_FLOAT,		// 深度値書き込み用フォーマット
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		1,							//	テクスチャ配列でも3Dテクスチャでもない
		1,							// ミップマップは1
		1,							// サンプルは1ピクセル当たり1つ
		0,							// クオリティは0
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	// 深度値用ヒーププロパティ
	//D3D12_HEAP_PROPERTIES depth_hp = {};
	//depth_hp.Type = D3D12_HEAP_TYPE_DEFAULT;	// DEFAULTなのであとはUNKNOWNでよい
	//depth_hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//depth_hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	
	CD3DX12_HEAP_PROPERTIES depth_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);	// d3dx12

	// このクリアバリューが重要な意味を持つ
	//D3D12_CLEAR_VALUE depth_cv = {};
	//depth_cv.DepthStencil.Depth = 1.0f;
	//depth_cv.Format = DXGI_FORMAT_D32_FLOAT;	// 32ビットFLOAT値としてクリア

	CD3DX12_CLEAR_VALUE depth_cv = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);		// d3dx12

	HRESULT hr = m_Device->CreateCommittedResource(
		&depth_hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depth_cv,
		IID_PPV_ARGS(&m_DepthBuffer)
	);

	return hr;
}

HRESULT DX12Renderer::CreateDepthBufferView()
{
	// 深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};

	dhd.NumDescriptors = 1;	// 深度バッファーは１つ
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// デプスステンシャルビューとして使う
	
	HRESULT hr = m_Device->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(m_DSVDescHeap.ReleaseAndGetAddressOf()));

	// 深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = {};
	dsvd.Format = DXGI_FORMAT_D32_FLOAT;	//深度値に32ビットfloat使用
	dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2Dテクスチャ
	dsvd.Flags = D3D12_DSV_FLAG_NONE;

	m_Device->CreateDepthStencilView(
		m_DepthBuffer.Get(),
		&dsvd,
		m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart()
	);

	return hr;
}

HRESULT DX12Renderer::CreateDescripterHeap()
{
	HRESULT hr;

	/// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC hd = {};

	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;  /// レンダーターゲットビューなのでRTV
	hd.NodeMask = 0;
	hd.NumDescriptors = 2; /// 裏表の２つ
	hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	/// ディスクリプタヒープ生成
	hr = m_Device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(m_DescHeap.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT DX12Renderer::CreateRenderTargetView()
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC scd = {};

	/// ディスクリプタとスワップチェーンを紐づけ
	hr = m_SwapChain4->GetDesc(&scd);
	static std::vector<ID3D12Resource*> backBuffers(scd.BufferCount);

	// sRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};

	rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE deschandle = m_DescHeap->GetCPUDescriptorHandleForHeapStart();
	for (size_t index = 0; index < scd.BufferCount; ++index)
	{
		hr = m_SwapChain4->GetBuffer(static_cast<UINT>(index), IID_PPV_ARGS(&backBuffers[index])); /// BackBuffersの中にスワップチェーン上のバックバッファーを取得
		m_Device->CreateRenderTargetView(backBuffers[index], &rtvd, deschandle);
		deschandle.ptr += m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	m_BackBuffers = backBuffers;
	return hr;
}

HRESULT DX12Renderer::CreateFence()
{
	HRESULT hr = m_Device->CreateFence(m_FenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT DX12Renderer::CreateRootSignature()
{
	HRESULT hr;

	// ディスクリプタレンジテーブル作成
	// 定数(b0)（ビュープロジェクション用）
	CD3DX12_DESCRIPTOR_RANGE dr[5] = {};
	dr[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);	// RangeType, NumDescriptor, BaseShaderRegister
	
	// 定数(b1)（ワールド・ボーン用）
	dr[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	// マテリアル(b2)
	dr[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	// テクスチャ用（マテリアルとペア）
	dr[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	// ライト用
	dr[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3);

	// ルートパラメータ作成
	D3D12_ROOT_PARAMETER rootparam[4] = {};
	//rootparam[0].InitAsDescriptorTable(1, &dr[0]);	// パラメータータイプはデスクリプターテーブルで、レンジ数は1、レンジのアドレス
	
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// 全シェーダーから見える
	rootparam[0].DescriptorTable.pDescriptorRanges = &dr[0];	// ディスクリプタレンジのアドレス
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;	// ディスクリプタレンジ数

	// ワールド・ボーン
	//rootparam[1].InitAsDescriptorTable(1, &dr[1]);
	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// 頂点シェーダーから見える
	rootparam[1].DescriptorTable.pDescriptorRanges = &dr[1];	// ディスクリプタレンジのアドレス
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1;	// ディスクリプタレンジ数
	
	// マテリアル
	//rootparam[2].InitAsDescriptorTable(2, &dr[2], D3D12_SHADER_VISIBILITY_PIXEL);
	rootparam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootparam[2].DescriptorTable.pDescriptorRanges = &dr[2];
	rootparam[2].DescriptorTable.NumDescriptorRanges = 2;

	rootparam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootparam[3].DescriptorTable.pDescriptorRanges = &dr[4];
	rootparam[3].DescriptorTable.NumDescriptorRanges = 1;

	CD3DX12_STATIC_SAMPLER_DESC sd[2] = {};

	sd[0].Init(0);	// シェーダーレジスターは0番

	sd[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	// シェーダーレジスターは1番、UVはクランプ
	
	// ルートシグネチャー作成
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters = rootparam;	// ルートパラメータの先頭アドレス
	rsd.NumParameters = 4;			// ルートパラメータ数
	rsd.pStaticSamplers = sd;		// サンプラーステートの先頭アドレス
	rsd.NumStaticSamplers = 2;		// サンプラーステート数

	ID3DBlob* RootSigBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &RootSigBlob, &m_ErrorBlob);
	hr = m_Device->CreateRootSignature(0, RootSigBlob->GetBufferPointer(), RootSigBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.ReleaseAndGetAddressOf()));
	RootSigBlob->Release();
	return hr;
}

HRESULT DX12Renderer::CreatePipelineState()
{
	HRESULT hr;

	// 頂点シェーダー読み込み
	hr = D3DCompileFromFile(
		L"LightingVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"LightingVS","vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		m_VSBlob.ReleaseAndGetAddressOf(), m_ErrorBlob.ReleaseAndGetAddressOf());

	if (FAILED(hr)) {
		if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(m_ErrorBlob->GetBufferSize());
			std::copy_n((char*)m_ErrorBlob->GetBufferPointer(), m_ErrorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//行儀悪いかな…
	}

	// ピクセルシェーダー読み込み
	hr = D3DCompileFromFile(
		L"LightingPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"LightingPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		m_PSBlob.ReleaseAndGetAddressOf(), m_ErrorBlob.ReleaseAndGetAddressOf()
	);

	if (FAILED(hr)) {
		if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(m_ErrorBlob->GetBufferSize());
			std::copy_n((char*)m_ErrorBlob->GetBufferPointer(), m_ErrorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//行儀悪いかな…
	}

	// 頂点レイアウト設定
	D3D12_INPUT_ELEMENT_DESC InputLayout[] =
	{
		// 座標情報
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		// 法線
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// uv座標
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// ボーンナンバー
		{"BONE_NO", 0, DXGI_FORMAT_R16G16_UINT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// ウェイト
		{"WEIGHT", 0, DXGI_FORMAT_R8_UINT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// ウェイト
		//{"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT,
		//0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// グラフィックパイプラインステート作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
	gpsd.pRootSignature = m_RootSignature.Get();
	gpsd.VS = CD3DX12_SHADER_BYTECODE(m_VSBlob.Get());
	gpsd.PS = CD3DX12_SHADER_BYTECODE(m_PSBlob.Get());

	gpsd.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;	// 中身は0xffffff


	D3D12_RENDER_TARGET_BLEND_DESC rtbd = {};

	rtbd.BlendEnable = false;	// αブレンディングは使用しない
	rtbd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	rtbd.LogicOpEnable = false;		// 論理演算は使用しない

	// ブレンドステート省略
	gpsd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// ラスタライザーステート省略
	gpsd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	
	
	// 深度バッファー省略
	gpsd.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpsd.InputLayout.pInputElementDescs = InputLayout;	//レイアウト先頭アドレス
	gpsd.InputLayout.NumElements = _countof(InputLayout);	//レイアウトの配列要素数

	gpsd.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpsd.NumRenderTargets = 1;//今は１つのみ
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	gpsd.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpsd.SampleDesc.Quality = 0;//クオリティは最低

	hr = m_Device->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf()));

	return hr;
}

HRESULT DX12Renderer::CreateSceneConstBuffer()
{
	auto buffersize = sizeof(SCENEMATRIX);
	buffersize = (buffersize + 0xff) & ~0xff;
	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(buffersize);		// d3d12x.h

	// 定数バッファ生成
	HRESULT hr = m_Device.Get()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_SceneConstBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		assert(SUCCEEDED(hr));
		return hr;
	}

	// GPUにマップ
	hr = m_SceneConstBuffer.Get()->Map(0, nullptr, (void**)&m_MappedSceneMatrix);

	XMFLOAT3 eye(0, 10, -35); // 視線
	XMFLOAT3 target(0, 10, 0); // 注視点
	XMFLOAT3 v_up(0, 1, 0); // 上ベクトル
	
	// 書き込み
	XMStoreFloat4x4(&m_MappedSceneMatrix->view, XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&v_up)));
	XMStoreFloat4x4(&m_MappedSceneMatrix->proj, XMMatrixPerspectiveFovLH(XM_PIDIV4,//画角は90°
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//アス比
		0.1f,//近い方
		1000.0f//遠い方
	));
	m_MappedSceneMatrix->eye = eye;
	XMFLOAT4 planeNormalVec(0, 1, 0, 0);
	m_MappedSceneMatrix->shadow = XMMatrixShadow(XMLoadFloat4(&planeNormalVec), -XMLoadFloat3(&m_ParallelLightVec));

	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダーから見える
	dhd.NodeMask = 0;	// マスク0
	dhd.NumDescriptors = 1;	// ビューは今のところ１つだけ
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = m_Device.Get()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(m_sceneDescHeap.ReleaseAndGetAddressOf()));
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {};
	cbvd.BufferLocation = m_SceneConstBuffer.Get()->GetGPUVirtualAddress();
	cbvd.SizeInBytes = static_cast<UINT>(m_SceneConstBuffer.Get()->GetDesc().Width);

	// バッファービュー作成
	m_Device.Get()->CreateConstantBufferView(&cbvd, m_sceneDescHeap.Get()->GetCPUDescriptorHandleForHeapStart());

	return hr;
}

HRESULT DX12Renderer::CreateLightConstBuffer()
{
	auto bufferSize = sizeof(LIGHT);
	bufferSize = (bufferSize + 0xff) & ~0xff;
	CD3DX12_HEAP_PROPERTIES hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	HRESULT hr = m_Device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_LightCBuffer.ReleaseAndGetAddressOf()));

	
	DX12Renderer::LIGHT light;
	light.Enable = true;
	light.Direction = XMVECTOR{ 1.0f, -1.0f, 1.0f, 0.0f };
	XMVector4Normalize(light.Direction);
	light.Ambient = XMFLOAT4{ 0.1f, 0.1f, 0.1f, 1.0f };
	light.Diffuse = XMFLOAT4{ 1.0f, -1.0f, 1.0f, 1.0f };

	hr = m_LightCBuffer.Get()->Map(0, nullptr, (void**)&m_mapLight);
	SetLight(light);

	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダーから見える
	dhd.NodeMask = 0;	// マスク0
	dhd.NumDescriptors = 1;	// ビューは今のところ１つだけ
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = m_Device.Get()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(m_LightDescHeap.ReleaseAndGetAddressOf()));

	auto heapHandle = m_LightDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {};
	cbvd.BufferLocation = m_LightCBuffer.Get()->GetGPUVirtualAddress();
	cbvd.SizeInBytes = static_cast<UINT>(m_LightCBuffer.Get()->GetDesc().Width);
	m_Device.Get()->CreateConstantBufferView(&cbvd, heapHandle);

	return hr;
}

HRESULT DX12Renderer::SetLight(LIGHT light)
{
	XMMatrixTranspose(XMLoadFloat4x4(&light.ProjMatrix));
	XMMatrixTranspose(XMLoadFloat4x4(&light.ViewMatrix));

	m_mapLight->Enable = light.Enable;
	m_mapLight->Direction = light.Direction;
	m_mapLight->Diffuse = light.Diffuse;
	m_mapLight->Ambient = light.Ambient;
	m_mapLight->ViewMatrix = light.ViewMatrix;
	m_mapLight->ProjMatrix = light.ProjMatrix;
	
	return S_OK;
}

void PeraPolygon::CreatePeraResorce()
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

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_PeraResource.ReleaseAndGetAddressOf())
	);

	// RTV用ヒープを作る
	hd.NumDescriptors = 1;
	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&hd,
		IID_PPV_ARGS(m_PeraRTVDescHeap.ReleaseAndGetAddressOf())
	);
	D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};
	rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// RTVを作る
	DX12Renderer::GetDevice()->CreateRenderTargetView(
		m_PeraResource.Get(),
		&rtvd,
		m_PeraRTVDescHeap.Get()->GetCPUDescriptorHandleForHeapStart()
	);


	// SRV用ヒープを作る
	hd.NumDescriptors = 2;
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&hd,
		IID_PPV_ARGS(m_PeraSRVDescHeap.ReleaseAndGetAddressOf())
	);

	auto handle = m_PeraSRVDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvd.Format = rtvd.Format;
	srvd.Texture2D.MipLevels = 1;
	srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// SRVを作る
	DX12Renderer::GetDevice()->CreateShaderResourceView(
		m_PeraResource.Get(),
		&srvd,
		handle
	);

	// ぼけ定数バッファービュー設定
	handle.ptr += DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {};
	cbvd.BufferLocation = m_BokehParamResource.Get()->GetGPUVirtualAddress();
	cbvd.SizeInBytes = m_BokehParamResource.Get()->GetDesc().Width;
	DX12Renderer::GetDevice()->CreateConstantBufferView(&cbvd, handle);
	

}

void PeraPolygon::PrePeraDraw()
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_PeraResource.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	DX12Renderer::GetGraphicsCommandList()->ResourceBarrier(
		1,
		&barrier
	);

	// 1パス目
	auto rtvHeapHandle = m_PeraRTVDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	auto dsvHeapHandle = DX12Renderer::GetDSVDescHeap()->GetCPUDescriptorHandleForHeapStart();

	DX12Renderer::GetGraphicsCommandList()->OMSetRenderTargets(
		1,
		&rtvHeapHandle,
		false,
		&dsvHeapHandle
	);
	float clsClr[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	DX12Renderer::GetGraphicsCommandList()->ClearRenderTargetView(rtvHeapHandle, clsClr, 0, nullptr);
	DX12Renderer::GetGraphicsCommandList()->ClearDepthStencilView(dsvHeapHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void PeraPolygon::PeraDraw1()
{
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootSignature(m_PeraRootSignature.Get());
	DX12Renderer::GetGraphicsCommandList()->SetPipelineState(m_NowUsePipelineState.Get());
	DX12Renderer::GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	DX12Renderer::GetGraphicsCommandList()->IASetVertexBuffers(0, 1, &m_PeraVBView);

	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, m_PeraSRVDescHeap.GetAddressOf());
	auto srvhandle = m_PeraSRVDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(0, srvhandle);
	srvhandle.ptr += DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(1, srvhandle);
	
	DX12Renderer::GetGraphicsCommandList()->DrawInstanced(4, 1, 0, 0);
}

void PeraPolygon::PostPeraDraw()
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_PeraResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	DX12Renderer::GetGraphicsCommandList()->ResourceBarrier(
		1,
		&barrier
	);
}

void PeraPolygon::CreatePeraVertex()
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
		IID_PPV_ARGS(m_PeraVB.ReleaseAndGetAddressOf())
	);
	m_PeraVBView.BufferLocation = m_PeraVB.Get()->GetGPUVirtualAddress();
	m_PeraVBView.SizeInBytes = sizeof(pv);
	m_PeraVBView.StrideInBytes = sizeof(PeraVertex);

	PeraVertex* mappedPera = nullptr;
	m_PeraVB.Get()->Map(0, nullptr, (void**)&mappedPera);
	std::copy(std::begin(pv), std::end(pv), mappedPera);
	m_PeraVB.Get()->Unmap(0, nullptr);

}

void PeraPolygon::CreatePeraPipeline()
{
	HRESULT hr;

	D3D12_DESCRIPTOR_RANGE dr[2] = {};
	dr[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	dr[0].BaseShaderRegister = 0;
	dr[0].NumDescriptors = 1;
	dr[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	dr[1].BaseShaderRegister = 0;
	dr[1].NumDescriptors = 1;

	D3D12_ROOT_PARAMETER rp[2] = {};
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[0].DescriptorTable.pDescriptorRanges = &dr[0];
	rp[0].DescriptorTable.NumDescriptorRanges = 1;
	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[1].DescriptorTable.pDescriptorRanges = &dr[1];
	rp[1].DescriptorTable.NumDescriptorRanges = 1;

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
		PS_ENTRYPOINT, "ps_5_0", 0, 0,
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
	rsd.NumParameters = 2;
	rsd.pParameters = rp;
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
		IID_PPV_ARGS(m_PeraRootSignature.ReleaseAndGetAddressOf())
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
	gpsd.pRootSignature = m_PeraRootSignature.Get();
	// 通常描画パイプライン
	hr = DX12Renderer::GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(m_PeraPipelineState.ReleaseAndGetAddressOf()));

	// ブラーパイプライン
	hr = D3DCompileFromFile(
		L"PeraPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BlurPS", "ps_5_0", 0, 0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	gpsd.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	hr = DX12Renderer::GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(m_BlurPipeline.ReleaseAndGetAddressOf()));

	// エンボスパイプライン
	hr = D3DCompileFromFile(
		L"PeraPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"EmbossPS", "ps_5_0", 0, 0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	gpsd.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	hr = DX12Renderer::GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(m_EmbossPipeline.ReleaseAndGetAddressOf()));

	// アウトラインパイプライン
	hr = D3DCompileFromFile(
		L"PeraPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"OutlinePS", "ps_5_0", 0, 0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	gpsd.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	hr = DX12Renderer::GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(m_OutlinePipeline.ReleaseAndGetAddressOf()));
	m_NowUsePipelineState = m_OutlinePipeline;
}

void PeraPolygon::CreatePostEffectPipeline()
{
	
}

std::vector<float> PeraPolygon::GetGaussianWeights(size_t count, float s)
{
	std::vector<float> weights(count);
	float x = 0.0f;
	float total = 0.0f;
	for (auto& wgt : weights)
	{
		wgt = expf(-(x * x) / (2 * s * s));
		total += wgt;
		x += 1.0f;
	}
	total = total * 2.0f - 1.0f;
	for (auto& wgt : weights)
	{
		wgt /= total;
	}

	return weights;
}

void PeraPolygon::CreateBokehParamResource()
{
	auto weights = GetGaussianWeights(8, 5.0f);
	auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto rd = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(weights[0]) * weights.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_BokehParamResource.ReleaseAndGetAddressOf())
	);

	float* mappedWeight = nullptr;
	hr = m_BokehParamResource.Get()->Map(0, nullptr, (void**)&mappedWeight);
	copy(weights.begin(), weights.end(), mappedWeight);
	m_BokehParamResource.Get()->Unmap(0, nullptr);

}

void PeraPolygon::Update()
{
	ChangePipeline();
}

void PeraPolygon::ChangePipeline()
{
	// 通常
	if (Input::GetKeyTrigger('1'))
	{
		m_NowUsePipelineState = m_PeraPipelineState;
	}

	// ブラー
	if (Input::GetKeyTrigger('2'))
	{
		m_NowUsePipelineState = m_BlurPipeline;
	}

	// エンボス
	if (Input::GetKeyTrigger('3'))
	{
		m_NowUsePipelineState = m_EmbossPipeline;
	}

	// アウトライン
	if (Input::GetKeyTrigger('4'))
	{
		m_NowUsePipelineState = m_OutlinePipeline;
	}
}
