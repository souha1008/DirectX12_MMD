#include "framework.h"
#include "DX12Renderer.h"
#include "Helper.h"
#include "input.h"

#define PS_ENTRYPOINT	"ps"

// static�����o�ϐ�
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

// �O���[�o���ϐ�
CD3DX12_VIEWPORT g_Viewport = {};
CD3DX12_RECT g_Scissorect = {};

void DX12Renderer::Init()
{
#ifdef _DEBUG
	// �f�o�b�O���C���[�I��
	EnableDebugLayer();
#endif // _DEBUG

	// �t�@�N�g���[����
	HRESULT hr = CreateFactory();
	// �f�o�C�X����
	hr = CreateDevice();
	// �R�}���h�L���[����
	hr = CreateCommandQueue();
	// �R�}���h���X�g���A���P�[�^�[����
	hr = CreateCommandList_Allocator();
	// �X���b�v�`�F�[������
	hr = CreateSwapChain();
	// �[�x�o�b�t�@�[����
	hr = CreateDepthBuffer();
	// �f�v�X�X�e���V���r���[����
	hr = CreateDepthBufferView();
	// �f�B�X�N���v�^�q�[�v����
	hr = CreateDescripterHeap();
	// �����_�[�^�[�Q�b�g�r���[����
	hr = CreateRenderTargetView();
	// �t�F���X����
	hr = CreateFence();
	// ���[�g�V�O�l�`������
	hr = CreateRootSignature();
	// �p�C�v���C���X�e�[�g�쐬
	hr = CreatePipelineState();

	// �r���[�v���W�F�N�V�����쐬
	hr = CreateSceneConstBuffer();

	// ���C�g�쐬
	hr = CreateLightConstBuffer();

	// �r���[�|�[�g
	g_Viewport = CD3DX12_VIEWPORT(m_BackBuffers[0]);	// �o�b�N�o�b�t�@�[���玩���v�Z

	// �؂蔲��
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
	// �o�b�N�o�b�t�@�̃C���f�b�N�X���擾
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

	// �����_�[�^�[�Q�b�g�w��
	D3D12_CPU_DESCRIPTOR_HANDLE rtvH = m_DescHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvH = m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart();
	m_GCmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

	// ��ʃN���A
	FLOAT clear_color[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	m_GCmdList->ClearRenderTargetView(rtvH, clear_color, 0, nullptr);
	m_GCmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);






}

void DX12Renderer::Draw3D()
{
	// ���[�g�V�O�l�`���Z�b�g
	m_GCmdList->SetGraphicsRootSignature(m_RootSignature.Get());

	// �p�C�v���C���Z�b�g
	m_GCmdList->SetPipelineState(m_PipelineState.Get());

	// �r���[
	ID3D12DescriptorHeap* scene_dh[] = { m_sceneDescHeap.Get() };
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, scene_dh);
	auto scene_handle = m_sceneDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(0, scene_handle);

	// ���C�g�̃f�X�N���v�^�[�Z�b�g
	ID3D12DescriptorHeap* light_dh[] = { m_LightDescHeap.Get() };
	m_GCmdList.Get()->SetDescriptorHeaps(1, light_dh);
	auto light_handle = m_LightDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	m_GCmdList.Get()->SetGraphicsRootDescriptorTable(3, light_handle);

	// �r���[�|�[�g�Z�b�g
	m_GCmdList->RSSetViewports(1, &g_Viewport);
	m_GCmdList->RSSetScissorRects(1, &g_Scissorect);
}

void DX12Renderer::End()
{
	
	// �o�b�N�o�b�t�@�̃C���f�b�N�X���擾
	UINT bbIdx = m_SwapChain4->GetCurrentBackBufferIndex();
	// �ҋ@��ԓ���ւ�
	//m_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//m_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	CD3DX12_RESOURCE_BARRIER tmp = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BackBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_GCmdList->ResourceBarrier(1, &tmp);

	// ���߃N���[�Y
	m_GCmdList->Close();

	// �R�}���h���X�g�̎��s
	ID3D12CommandList* cmdlist[] = { m_GCmdList.Get() };
	m_CmdQueue->ExecuteCommandLists(1, cmdlist);

	// �҂�
	m_CmdQueue->Signal(m_Fence.Get(), ++m_FenceVal);
	if (m_Fence->GetCompletedValue() != m_FenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		m_Fence->SetEventOnCompletion(m_FenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	m_CmdAllocator->Reset();
	m_GCmdList->Reset(m_CmdAllocator.Get(), m_PipelineState.Get());

	// �t���b�v
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

	// ����̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; m_DXGIFactry->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	// Description�����o�[�ϐ�����A�A�_�v�^�[�̖��O��������
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);

		std::wstring strDesc = adesc.Description;

		/// �T�������A�_�v�^�[�̖��O���m�F
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

	///  COMMAND_QUEUE�̐ݒ�
	D3D12_COMMAND_QUEUE_DESC qDesc = {};
	qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	/// �^�C���A�E�g����
	qDesc.NodeMask = 0;	/// �A�_�v�^�[��������g��Ȃ��Ƃ��͂O
	qDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	/// �v���C�I���e�B�͓��Ɏw��Ȃ�
	qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	/// �R�}���h���X�g�ƍ��킹��
	hr = m_Device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(m_CmdQueue.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT DX12Renderer::CreateCommandList_Allocator()
{
	HRESULT hr;

	/// �R�}���h�A���P�[�^����
	hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_CmdAllocator.ReleaseAndGetAddressOf()));

	/// �R�}���h���X�g����
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
	swapDesc.Scaling = DXGI_SCALING_STRETCH;		/// �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	/// �t���b�v��͑��₩�ɔj��
	swapDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;	/// ���Ɏw��Ȃ�
	swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	/// �E�B���h�E�@�t���X�N���[���ؑ։\

	/// SwapChain����
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
	// �[�x�o�b�t�@�[�̍쐬

	// �[�x�o�b�t�@�[�p���\�[�X�ݒ�
	//D3D12_RESOURCE_DESC rd = {};
	//rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2�����̃e�N�X�`���f�[�^
	//rd.Width = SCREEN_WIDTH;
	//rd.Height = SCREEN_HEIGHT;
	//rd.DepthOrArraySize = 1;	//	�e�N�X�`���z��ł�3D�e�N�X�`���ł��Ȃ�
	//rd.Format = DXGI_FORMAT_D32_FLOAT;	// �[�x�l�������ݗp�t�H�[�}�b�g
	//rd.SampleDesc.Count = 1;	// �T���v����1�s�N�Z��������1��
	//rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// �f�v�X�X�e���V���Ƃ��Ďg�p

	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D(						// d3dx12
		DXGI_FORMAT_D32_FLOAT,		// �[�x�l�������ݗp�t�H�[�}�b�g
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		1,							//	�e�N�X�`���z��ł�3D�e�N�X�`���ł��Ȃ�
		1,							// �~�b�v�}�b�v��1
		1,							// �T���v����1�s�N�Z��������1��
		0,							// �N�I���e�B��0
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	// �[�x�l�p�q�[�v�v���p�e�B
	//D3D12_HEAP_PROPERTIES depth_hp = {};
	//depth_hp.Type = D3D12_HEAP_TYPE_DEFAULT;	// DEFAULT�Ȃ̂ł��Ƃ�UNKNOWN�ł悢
	//depth_hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//depth_hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	
	CD3DX12_HEAP_PROPERTIES depth_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);	// d3dx12

	// ���̃N���A�o�����[���d�v�ȈӖ�������
	//D3D12_CLEAR_VALUE depth_cv = {};
	//depth_cv.DepthStencil.Depth = 1.0f;
	//depth_cv.Format = DXGI_FORMAT_D32_FLOAT;	// 32�r�b�gFLOAT�l�Ƃ��ăN���A

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
	// �[�x�̂��߂̃f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};

	dhd.NumDescriptors = 1;	// �[�x�o�b�t�@�[�͂P��
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// �f�v�X�X�e���V�����r���[�Ƃ��Ďg��
	
	HRESULT hr = m_Device->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(m_DSVDescHeap.ReleaseAndGetAddressOf()));

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = {};
	dsvd.Format = DXGI_FORMAT_D32_FLOAT;	//�[�x�l��32�r�b�gfloat�g�p
	dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2D�e�N�X�`��
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

	/// �f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC hd = {};

	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;  /// �����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	hd.NodeMask = 0;
	hd.NumDescriptors = 2; /// ���\�̂Q��
	hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	/// �f�B�X�N���v�^�q�[�v����
	hr = m_Device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(m_DescHeap.ReleaseAndGetAddressOf()));
	return hr;
}

HRESULT DX12Renderer::CreateRenderTargetView()
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC scd = {};

	/// �f�B�X�N���v�^�ƃX���b�v�`�F�[����R�Â�
	hr = m_SwapChain4->GetDesc(&scd);
	static std::vector<ID3D12Resource*> backBuffers(scd.BufferCount);

	// sRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};

	rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE deschandle = m_DescHeap->GetCPUDescriptorHandleForHeapStart();
	for (size_t index = 0; index < scd.BufferCount; ++index)
	{
		hr = m_SwapChain4->GetBuffer(static_cast<UINT>(index), IID_PPV_ARGS(&backBuffers[index])); /// BackBuffers�̒��ɃX���b�v�`�F�[����̃o�b�N�o�b�t�@�[���擾
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

	// �f�B�X�N���v�^�����W�e�[�u���쐬
	// �萔(b0)�i�r���[�v���W�F�N�V�����p�j
	CD3DX12_DESCRIPTOR_RANGE dr[5] = {};
	dr[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);	// RangeType, NumDescriptor, BaseShaderRegister
	
	// �萔(b1)�i���[���h�E�{�[���p�j
	dr[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	// �}�e���A��(b2)
	dr[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	// �e�N�X�`���p�i�}�e���A���ƃy�A�j
	dr[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	// ���C�g�p
	dr[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3);

	// ���[�g�p�����[�^�쐬
	D3D12_ROOT_PARAMETER rootparam[4] = {};
	//rootparam[0].InitAsDescriptorTable(1, &dr[0]);	// �p�����[�^�[�^�C�v�̓f�X�N���v�^�[�e�[�u���ŁA�����W����1�A�����W�̃A�h���X
	
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// �S�V�F�[�_�[���猩����
	rootparam[0].DescriptorTable.pDescriptorRanges = &dr[0];	// �f�B�X�N���v�^�����W�̃A�h���X
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;	// �f�B�X�N���v�^�����W��

	// ���[���h�E�{�[��
	//rootparam[1].InitAsDescriptorTable(1, &dr[1]);
	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// ���_�V�F�[�_�[���猩����
	rootparam[1].DescriptorTable.pDescriptorRanges = &dr[1];	// �f�B�X�N���v�^�����W�̃A�h���X
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1;	// �f�B�X�N���v�^�����W��
	
	// �}�e���A��
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

	sd[0].Init(0);	// �V�F�[�_�[���W�X�^�[��0��

	sd[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	// �V�F�[�_�[���W�X�^�[��1�ԁAUV�̓N�����v
	
	// ���[�g�V�O�l�`���[�쐬
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters = rootparam;	// ���[�g�p�����[�^�̐擪�A�h���X
	rsd.NumParameters = 4;			// ���[�g�p�����[�^��
	rsd.pStaticSamplers = sd;		// �T���v���[�X�e�[�g�̐擪�A�h���X
	rsd.NumStaticSamplers = 2;		// �T���v���[�X�e�[�g��

	ID3DBlob* RootSigBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &RootSigBlob, &m_ErrorBlob);
	hr = m_Device->CreateRootSignature(0, RootSigBlob->GetBufferPointer(), RootSigBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.ReleaseAndGetAddressOf()));
	RootSigBlob->Release();
	return hr;
}

HRESULT DX12Renderer::CreatePipelineState()
{
	HRESULT hr;

	// ���_�V�F�[�_�[�ǂݍ���
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
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;
			errstr.resize(m_ErrorBlob->GetBufferSize());
			std::copy_n((char*)m_ErrorBlob->GetBufferPointer(), m_ErrorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//�s�V�������ȁc
	}

	// �s�N�Z���V�F�[�_�[�ǂݍ���
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
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;
			errstr.resize(m_ErrorBlob->GetBufferSize());
			std::copy_n((char*)m_ErrorBlob->GetBufferPointer(), m_ErrorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//�s�V�������ȁc
	}

	// ���_���C�A�E�g�ݒ�
	D3D12_INPUT_ELEMENT_DESC InputLayout[] =
	{
		// ���W���
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		// �@��
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// uv���W
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// �{�[���i���o�[
		{"BONE_NO", 0, DXGI_FORMAT_R16G16_UINT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// �E�F�C�g
		{"WEIGHT", 0, DXGI_FORMAT_R8_UINT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		// �E�F�C�g
		//{"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT,
		//0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// �O���t�B�b�N�p�C�v���C���X�e�[�g�쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
	gpsd.pRootSignature = m_RootSignature.Get();
	gpsd.VS = CD3DX12_SHADER_BYTECODE(m_VSBlob.Get());
	gpsd.PS = CD3DX12_SHADER_BYTECODE(m_PSBlob.Get());

	gpsd.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;	// ���g��0xffffff


	D3D12_RENDER_TARGET_BLEND_DESC rtbd = {};

	rtbd.BlendEnable = false;	// ���u�����f�B���O�͎g�p���Ȃ�
	rtbd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	rtbd.LogicOpEnable = false;		// �_�����Z�͎g�p���Ȃ�

	// �u�����h�X�e�[�g�ȗ�
	gpsd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// ���X�^���C�U�[�X�e�[�g�ȗ�
	gpsd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�
	
	
	// �[�x�o�b�t�@�[�ȗ�
	gpsd.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpsd.InputLayout.pInputElementDescs = InputLayout;	//���C�A�E�g�擪�A�h���X
	gpsd.InputLayout.NumElements = _countof(InputLayout);	//���C�A�E�g�̔z��v�f��

	gpsd.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	gpsd.NumRenderTargets = 1;//���͂P�̂�
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	gpsd.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	gpsd.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�

	hr = m_Device->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf()));

	return hr;
}

HRESULT DX12Renderer::CreateSceneConstBuffer()
{
	auto buffersize = sizeof(SCENEMATRIX);
	buffersize = (buffersize + 0xff) & ~0xff;
	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(buffersize);		// d3d12x.h

	// �萔�o�b�t�@����
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

	// GPU�Ƀ}�b�v
	hr = m_SceneConstBuffer.Get()->Map(0, nullptr, (void**)&m_MappedSceneMatrix);

	XMFLOAT3 eye(0, 10, -35); // ����
	XMFLOAT3 target(0, 10, 0); // �����_
	XMFLOAT3 v_up(0, 1, 0); // ��x�N�g��
	
	// ��������
	XMStoreFloat4x4(&m_MappedSceneMatrix->view, XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&v_up)));
	XMStoreFloat4x4(&m_MappedSceneMatrix->proj, XMMatrixPerspectiveFovLH(XM_PIDIV4,//��p��90��
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//�A�X��
		0.1f,//�߂���
		1000.0f//������
	));
	m_MappedSceneMatrix->eye = eye;
	XMFLOAT4 planeNormalVec(0, 1, 0, 0);
	m_MappedSceneMatrix->shadow = XMMatrixShadow(XMLoadFloat4(&planeNormalVec), -XMLoadFloat3(&m_ParallelLightVec));

	// �f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// �V�F�[�_�[���猩����
	dhd.NodeMask = 0;	// �}�X�N0
	dhd.NumDescriptors = 1;	// �r���[�͍��̂Ƃ���P����
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = m_Device.Get()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(m_sceneDescHeap.ReleaseAndGetAddressOf()));
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {};
	cbvd.BufferLocation = m_SceneConstBuffer.Get()->GetGPUVirtualAddress();
	cbvd.SizeInBytes = static_cast<UINT>(m_SceneConstBuffer.Get()->GetDesc().Width);

	// �o�b�t�@�[�r���[�쐬
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
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// �V�F�[�_�[���猩����
	dhd.NodeMask = 0;	// �}�X�N0
	dhd.NumDescriptors = 1;	// �r���[�͍��̂Ƃ���P����
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
	// �쐬�ς݂̃q�[�v�����g���č��
	D3D12_DESCRIPTOR_HEAP_DESC hd = DX12Renderer::GetRTVDescHeap()->GetDesc();

	// �g���Ă���o�b�N�o�b�t�@�[�̏��𗘗p����
	auto bbuff = DX12Renderer::GetBackBuffers();

	D3D12_RESOURCE_DESC rd = bbuff[0]->GetDesc();

	D3D12_HEAP_PROPERTIES hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// �����_�����O���̃N���A�l�Ɠ����l
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

	// RTV�p�q�[�v�����
	hd.NumDescriptors = 1;
	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&hd,
		IID_PPV_ARGS(m_PeraRTVDescHeap.ReleaseAndGetAddressOf())
	);
	D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};
	rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// RTV�����
	DX12Renderer::GetDevice()->CreateRenderTargetView(
		m_PeraResource.Get(),
		&rtvd,
		m_PeraRTVDescHeap.Get()->GetCPUDescriptorHandleForHeapStart()
	);


	// SRV�p�q�[�v�����
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

	// SRV�����
	DX12Renderer::GetDevice()->CreateShaderResourceView(
		m_PeraResource.Get(),
		&srvd,
		handle
	);

	// �ڂ��萔�o�b�t�@�[�r���[�ݒ�
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

	// 1�p�X��
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
		{{-1, -1, 0.1f}, {0, 1}},	// ����
		{{-1, 1, 0.1f}, {0, 0}},	// ����
		{{1, -1, 0.1f}, {1, 1}},	// �E��
		{{1, 1, 0.1f}, {1, 0}}		// �E��
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
	// �ʏ�`��p�C�v���C��
	hr = DX12Renderer::GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(m_PeraPipelineState.ReleaseAndGetAddressOf()));

	// �u���[�p�C�v���C��
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

	// �G���{�X�p�C�v���C��
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

	// �A�E�g���C���p�C�v���C��
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
	// �ʏ�
	if (Input::GetKeyTrigger('1'))
	{
		m_NowUsePipelineState = m_PeraPipelineState;
	}

	// �u���[
	if (Input::GetKeyTrigger('2'))
	{
		m_NowUsePipelineState = m_BlurPipeline;
	}

	// �G���{�X
	if (Input::GetKeyTrigger('3'))
	{
		m_NowUsePipelineState = m_EmbossPipeline;
	}

	// �A�E�g���C��
	if (Input::GetKeyTrigger('4'))
	{
		m_NowUsePipelineState = m_OutlinePipeline;
	}
}
