#include "framework.h"
#include "DX12Renderer.h"

// static�����o�ϐ�
ID3D12Device* DX12Renderer::m_Device = nullptr;
IDXGIFactory6* DX12Renderer::m_DXGIFactry = nullptr;
IDXGISwapChain4* DX12Renderer::m_SwapChain4 = nullptr;
ID3D12Resource* DX12Renderer::m_DepthBuffer = nullptr;
ID3D12DescriptorHeap* DX12Renderer::m_DSVDescHeap = nullptr;
ID3D12CommandAllocator* DX12Renderer::m_CmdAllocator = nullptr;
ID3D12GraphicsCommandList* DX12Renderer::m_GCmdList = nullptr;
ID3D12CommandQueue* DX12Renderer::m_CmdQueue = nullptr;
ID3D12DescriptorHeap* DX12Renderer::m_DescHeap = nullptr;
ID3D12RootSignature* DX12Renderer::m_RootSignature = nullptr;
ID3D12PipelineState* DX12Renderer::m_PipelineState = nullptr;
ID3D12Fence* DX12Renderer::m_Fence = nullptr;
ID3DBlob* DX12Renderer::m_VSBlob = nullptr;
ID3DBlob* DX12Renderer::m_PSBlob = nullptr;
ID3DBlob* DX12Renderer::m_ErrorBlob = nullptr;
UINT64 DX12Renderer::m_FenceVal = 0;
D3D12_RESOURCE_BARRIER DX12Renderer::m_Barrier = {};
std::vector<ID3D12Resource*> DX12Renderer::m_BackBuffers;

// �O���[�o���ϐ�
D3D12_VIEWPORT g_Viewport = {};
D3D12_RECT g_Scissorect = {};

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

	// �r���[�|�[�g
	g_Viewport.Width = SCREEN_WIDTH;		//�o�͐�̕�(�s�N�Z����)
	g_Viewport.Height = SCREEN_HEIGHT;		//�o�͐�̍���(�s�N�Z����)
	g_Viewport.TopLeftX = 0;				//�o�͐�̍�����WX
	g_Viewport.TopLeftY = 0;				//�o�͐�̍�����WY
	g_Viewport.MaxDepth = 1.0f;				//�[�x�ő�l
	g_Viewport.MinDepth = 0.0f;				//�[�x�ŏ��l

	// �؂蔲��
	g_Scissorect.top = 0;
	g_Scissorect.bottom = g_Scissorect.top + SCREEN_HEIGHT;
	g_Scissorect.left = 0;
	g_Scissorect.right = g_Scissorect.left + SCREEN_WIDTH;

}

void DX12Renderer::Uninit()
{
	m_PipelineState->Release();
	m_RootSignature->Release();
	m_BackBuffers.clear();
	m_DescHeap->Release();
	m_SwapChain4->Release();
	m_CmdAllocator->Release();
	m_GCmdList->Release();
	m_CmdQueue->Release();
	m_Device->Release();
	m_DXGIFactry->Release();
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
	FLOAT clear_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_GCmdList->ClearRenderTargetView(rtvH, clear_color, 0, nullptr);
	m_GCmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// �r���[�|�[�g�Z�b�g
	m_GCmdList->RSSetViewports(1, &g_Viewport);
	m_GCmdList->RSSetScissorRects(1, &g_Scissorect);

	// ���[�g�V�O�l�`���Z�b�g
	m_GCmdList->SetGraphicsRootSignature(m_RootSignature);

	// �p�C�v���C���Z�b�g
	m_GCmdList->SetPipelineState(m_PipelineState);

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
	ID3D12CommandList* cmdlist[] = { m_GCmdList };
	m_CmdQueue->ExecuteCommandLists(1, cmdlist);

	// �҂�
	m_CmdQueue->Signal(m_Fence, ++m_FenceVal);
	if (m_Fence->GetCompletedValue() != m_FenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		m_Fence->SetEventOnCompletion(m_FenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	m_CmdAllocator->Reset();
	m_GCmdList->Reset(m_CmdAllocator, m_PipelineState);

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
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactry));
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
		if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(&m_Device)) == S_OK)
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
	hr = m_Device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&m_CmdQueue));
	return hr;
}

HRESULT DX12Renderer::CreateCommandList_Allocator()
{
	HRESULT hr;

	/// �R�}���h�A���P�[�^����
	hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CmdAllocator));

	/// �R�}���h���X�g����
	hr = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CmdAllocator, nullptr, IID_PPV_ARGS(&m_GCmdList));
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
		m_CmdQueue,
		GetWindow(),
		&swapDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&m_SwapChain4
	);
	return hr;
}

HRESULT DX12Renderer::CreateDepthBuffer()
{
	// �[�x�o�b�t�@�[�̍쐬

	// �[�x�o�b�t�@�[�p���\�[�X�ݒ�
	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2�����̃e�N�X�`���f�[�^
	rd.Width = SCREEN_WIDTH;
	rd.Height = SCREEN_HEIGHT;
	rd.DepthOrArraySize = 1;	//	�e�N�X�`���z��ł�3D�e�N�X�`���ł��Ȃ�
	rd.Format = DXGI_FORMAT_D32_FLOAT;	// �[�x�l�������ݗp�t�H�[�}�b�g
	rd.SampleDesc.Count = 1;	// �T���v����1�s�N�Z��������1��
	rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// �f�v�X�X�e���V���Ƃ��Ďg�p

	// �[�x�l�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depth_hp = {};
	depth_hp.Type = D3D12_HEAP_TYPE_DEFAULT;	// DEFAULT�Ȃ̂ł��Ƃ�UNKNOWN�ł悢
	depth_hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depth_hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// ���̃N���A�o�����[���d�v�ȈӖ�������
	D3D12_CLEAR_VALUE depth_cv = {};
	depth_cv.DepthStencil.Depth = 1.0f;
	depth_cv.Format = DXGI_FORMAT_D32_FLOAT;	// 32�r�b�gFLOAT�l�Ƃ��ăN���A

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
	
	HRESULT hr = m_Device->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_DSVDescHeap));

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = {};
	dsvd.Format = DXGI_FORMAT_D32_FLOAT;	//�[�x�l��32�r�b�gfloat�g�p
	dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2D�e�N�X�`��
	dsvd.Flags = D3D12_DSV_FLAG_NONE;

	m_Device->CreateDepthStencilView(
		m_DepthBuffer,
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
	hr = m_Device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&m_DescHeap));
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

	rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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
	HRESULT hr = m_Device->CreateFence(m_FenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
	return hr;
}

HRESULT DX12Renderer::CreateRootSignature()
{
	HRESULT hr;

	// �f�B�X�N���v�^�����W�e�[�u���쐬
	// �萔�p��ځi���W�ϊ��p�j
	D3D12_DESCRIPTOR_RANGE dr[3] = {};
	dr[0].NumDescriptors = 1;
	dr[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	dr[0].BaseShaderRegister = 0;
	dr[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �萔��ځi�}�e���A���p�j
	dr[1].NumDescriptors = 1;
	dr[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	dr[1].BaseShaderRegister = 1;
	dr[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �e�N�X�`����ځi�}�e���A���ƃy�A�j
	dr[2].NumDescriptors = 2;	// �e�N�X�`����sph
	dr[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	dr[2].BaseShaderRegister = 0;
	dr[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�쐬
	// �s�N�Z���V�F�[�_�[
	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// �s�N�Z���V�F�[�_�[���猩����
	rootparam[0].DescriptorTable.pDescriptorRanges = &dr[0];	// �f�B�X�N���v�^�����W�̃A�h���X
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;	// �f�B�X�N���v�^�����W��

	// ���_�V�F�[�_�[
	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// ���_�V�F�[�_�[���猩����
	rootparam[1].DescriptorTable.pDescriptorRanges = &dr[1];	// �f�B�X�N���v�^�����W�̃A�h���X
	rootparam[1].DescriptorTable.NumDescriptorRanges = 2;	// �f�B�X�N���v�^�����W��
	
	D3D12_STATIC_SAMPLER_DESC sd = {};

	sd.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	// �������̌J��Ԃ�
	sd.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	// �c�����̌J��Ԃ�
	sd.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	// ���s���̌J��Ԃ�
	sd.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	// �{�[�_�[�͍�
	sd.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	// ���`���
	sd.MaxLOD = D3D12_FLOAT32_MAX;	// �~�b�v�}�b�v�ő�l
	sd.MinLOD = 0.0f;	// �~�b�v�}�b�v�ŏ��l
	sd.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// �s�N�Z���V�F�[�_�[���猩����
	sd.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;	// ���T���v�����O���Ȃ�
	sd.ShaderRegister = 0;

	// ���[�g�V�O�l�`���[�쐬
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters = &rootparam[0];	// ���[�g�p�����[�^�̐擪�A�h���X
	rsd.NumParameters = 2;			// ���[�g�p�����[�^��
	rsd.pStaticSamplers = &sd;		// �T���v���[�X�e�[�g�̐擪�A�h���X
	rsd.NumStaticSamplers = 1;		// �T���v���[�X�e�[�g��

	ID3DBlob* RootSigBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1_0, &RootSigBlob, &m_ErrorBlob);
	hr = m_Device->CreateRootSignature(0, RootSigBlob->GetBufferPointer(), RootSigBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
	RootSigBlob->Release();
	return hr;
}

HRESULT DX12Renderer::CreatePipelineState()
{
	HRESULT hr;

	// ���_�V�F�[�_�[�ǂݍ���
	hr = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS","vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&m_VSBlob, &m_ErrorBlob);

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
		L"BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&m_PSBlob, &m_ErrorBlob
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
	gpsd.pRootSignature = nullptr;
	gpsd.VS.pShaderBytecode = m_VSBlob->GetBufferPointer();
	gpsd.VS.BytecodeLength = m_VSBlob->GetBufferSize();
	gpsd.PS.pShaderBytecode = m_PSBlob->GetBufferPointer();
	gpsd.PS.BytecodeLength = m_PSBlob->GetBufferSize();

	gpsd.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;	// ���g��0xffffff

	gpsd.BlendState.AlphaToCoverageEnable = false;
	gpsd.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC rtbd = {};
	// ���u�����f�B���O�͎g�p���Ȃ�
	rtbd.BlendEnable = false;
	rtbd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	// �_�����Z�͎g�p���Ȃ�
	rtbd.LogicOpEnable = false;

	gpsd.BlendState.RenderTarget[0] = rtbd;

	gpsd.RasterizerState.MultisampleEnable = false;//�܂��A���`�F���͎g��Ȃ�
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�
	gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//���g��h��Ԃ�
	gpsd.RasterizerState.DepthClipEnable = true;//�[�x�����̃N���b�s���O�͗L����

	//�c��
	gpsd.RasterizerState.FrontCounterClockwise = false;
	gpsd.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpsd.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpsd.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpsd.RasterizerState.AntialiasedLineEnable = false;
	gpsd.RasterizerState.ForcedSampleCount = 0;
	gpsd.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	
	// �[�x�o�b�t�@�[�̐ݒ�
	gpsd.DepthStencilState.DepthEnable = true;
	gpsd.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsd.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;	// �������ق��̗p
	gpsd.DepthStencilState.StencilEnable = false;
	gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//���C�A�E�g�擪�A�h���X
	gpsd.InputLayout.pInputElementDescs = InputLayout;
	//���C�A�E�g�̔z��v�f��
	gpsd.InputLayout.NumElements = _countof(InputLayout);

	gpsd.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	gpsd.NumRenderTargets = 1;//���͂P�̂�
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//0�`1�ɐ��K�����ꂽRGBA

	gpsd.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	gpsd.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�
	gpsd.pRootSignature = m_RootSignature;

	hr = m_Device->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_PipelineState));

	return hr;
}