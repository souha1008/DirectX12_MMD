#include "framework.h"
#include "DX12Renderer.h"
#include "Polygon2D.h"
#include "3DObject.h"

using namespace DirectX;

// グローバル変数

// 頂点情報
POLYGON vertex[] =
{
	{{-1.0f,-1.0f, 0.0f}, {0.0f, 1.0f}},//左下
	{{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},//左上
	{{ 1.0f,-1.0f, 0.0f}, {1.0f, 1.0f}},//右下
	{{ 1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},//右上
};

// テクスチャ情報
//std::vector<TEXRGBA> g_Texture(256 * 256);

float g_angle = 0.0f;
float g_posX = 0.0f;

Object3D* g_Object;
MODEL_DX12 m_Model;

void Polygon2D::Init()
{

	g_Object = new Object3D();

	HRESULT hr = g_Object->CreateModel("Assets/Model/シンプルモデル/初音ミク.pmd"
		, "Assets/VMD/Unknown.vmd"
		, &m_Model);

	if (FAILED(hr))
	{
		return;
	}

	g_Object->PlayAnimation();
}

void Polygon2D::Uninit()
{
	g_Object->UnInit(&m_Model);
}

void Polygon2D::Update()
{
	//PolygonRotation();

	g_Object->MotionUpdate(&m_Model);

	//PolygonMove();
}

void Polygon2D::Draw()
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

	// 描画処理
	//DX12Renderer::GetGraphicsCommandList()->DrawIndexedInstanced(m_Model.sub.indecesNum, 1, 0, 0, 0);
	//DX12Renderer::GetGraphicsCommandList()->DrawInstanced(m_Model.sub.vertNum, 1, 0, 0);
}

HRESULT Polygon2D::CreateVertexBuffer()
{
	HRESULT hr;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex));		// d3d12x.h

	// 頂点バッファー生成
	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,	// UPLOADヒープ
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,		// サイズに応じて適切な設定をする
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_VertexBuffer)
	);

	// 頂点バッファーマップ
	POLYGON* vertMap = nullptr;
	m_VertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertex), std::end(vertex), vertMap);
	m_VertexBuffer->Unmap(0, nullptr);

	return hr;
}

HRESULT Polygon2D::SettingVertexBufferView()
{
	m_vbView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress(); // バッファの仮想アドレス
	m_vbView.SizeInBytes = sizeof(vertex);	// 全バイト数
	m_vbView.StrideInBytes = sizeof(vertex[0]);	// 1頂点あたりのバイト数

	if (m_vbView.SizeInBytes <= 0 || m_vbView.StrideInBytes <= 0)
	{
		return ERROR;
	}
	
	return S_OK;
}

HRESULT Polygon2D::CreateIndexBuffer(unsigned short* index)
{
	HRESULT hr;
	unsigned short Index[] = { 0,1,2, 2,1,3 };	// インデックス

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Index));		// d3d12x.h

	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_IndexBuffer)
	);
	// インデックスバッファーマップ
	unsigned short* indMap = nullptr;
	m_IndexBuffer->Map(0, nullptr, (void**)&indMap);
	std::copy(std::begin(Index), std::end(Index), indMap);
	m_IndexBuffer->Unmap(0, nullptr);

	return hr;
}

HRESULT Polygon2D::SettingIndexBufferView(unsigned short* index)
{
	unsigned short Index[] = { 0,1,2, 2,1,3 };	// インデックス
	m_ibView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_ibView.Format = DXGI_FORMAT_R16_UINT;
	m_ibView.SizeInBytes = sizeof(Index);
	return S_OK;
}

HRESULT Polygon2D::CreateSceneCBuffer()
{
	m_WorldMatrix = XMMatrixRotationY(XM_PIDIV4);

	// 視線
	XMFLOAT3 eye(0, 0, -5);
	// 注視点
	XMFLOAT3 target(0, 0, 0);
	// 上ベクトル
	XMFLOAT3 v_up(0, 1, 0);

	 m_viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&v_up));
	 m_projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2,//画角は90°
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//アス比
		1.0f,//近い方
		10.0f//遠い方
	);


	// スクリーン座標にする
	//matrix.r[0].m128_f32[0] = 2.0f / SCREEN_WIDTH;
	//matrix.r[1].m128_f32[1] = 2.0f / SCREEN_HEIGHT;
	//matrix.r[3].m128_f32[3] = -1.0f;
	//matrix.r[3].m128_f32[3] = 1.0f;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 0xff) & ~0xff);		// d3d12x.h


	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_ConstBuffer)
	);

	hr = m_ConstBuffer->Map(0, nullptr, (void**)&m_MapMatrix);
	*m_MapMatrix = m_WorldMatrix * m_viewMat * m_projMat;

	return hr;
}

HRESULT Polygon2D::SettingConstBufferView(D3D12_CPU_DESCRIPTOR_HANDLE* handle)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {};
	cbvd.BufferLocation = m_ConstBuffer->GetGPUVirtualAddress();
	cbvd.SizeInBytes = static_cast<UINT>(m_ConstBuffer->GetDesc().Width);

	DX12Renderer::GetDevice()->CreateConstantBufferView(&cbvd, *handle);

	return S_OK;
}

HRESULT Polygon2D::CreateBasicDescriptorHeap()
{
	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダーから見える
	dhd.NodeMask = 0;	// マスク0
	dhd.NumDescriptors = 2;	// ビューは今のところ１つだけ
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	HRESULT hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_basicDescHeap));
	return hr;
}

HRESULT Polygon2D::CreateTextureData()
{
	//for (TEXRGBA& rgba : g_Texture)
	//{
	//	rgba.R = rand() % 256;
	//	rgba.G = rand() % 256;
	//	rgba.B = rand() % 256;
	//	rgba.A = 255;
	//}

	
	ScratchImage ScratchImg = {};

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// WICテクスチャのロード
	hr = LoadFromWICFile(
		L"img/textest.png",
		WIC_FLAGS_NONE,
		&m_MetaData,
		ScratchImg
	);

	// 生データ抽出
	const Image* img = ScratchImg.GetImage(0, 0, 0);

	// WriteToSubresourceで転送するためのヒープ設定
	D3D12_HEAP_PROPERTIES heapprop = {};

	// 特殊な設定なのでDEFAULTでもUPLOADでもない
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;

	//ライトバック
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

	// 転送はL0、CPU側から直接行う
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	// 単一アダプターのため0
	heapprop.CreationNodeMask = 0;
	heapprop.VisibleNodeMask = 0;

	// リソース設定
	D3D12_RESOURCE_DESC rd = {};

	// RGBAフォーマット
	rd.Format = m_MetaData.format;

	rd.Width = m_MetaData.width;		// テクスチャの幅
	rd.Height = m_MetaData.height;	// テクスチャの高さ
	rd.DepthOrArraySize = m_MetaData.arraySize;	// 2Dで配列でもないので１
	rd.SampleDesc.Count = 1;	// 通常テクスチャなのでアンチエイリアシングしない
	rd.SampleDesc.Quality = 0;	// クオリティは最低
	rd.MipLevels = m_MetaData.mipLevels;	// 読み込んだメタデータのミップレベル参照
	rd.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(m_MetaData.dimension);	// 2Dテクスチャ用
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// レイアウトは決定しない
	rd.Flags = D3D12_RESOURCE_FLAG_NONE;	// フラグなし

	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&m_TextureBuffer)
	);
	

	hr = m_TextureBuffer->WriteToSubresource(
		0,
		nullptr,
		img->pixels,	// 元データアドレス
		img->rowPitch,	// 1ラインサイズ
		img->slicePitch
	);

	return hr;
}

HRESULT Polygon2D::CreateShaderResourceView()
{
	// シェーダーリソースビューを作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = m_MetaData.format;	// RGBA(0~1に正規化)
	srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = 1;
	
	DX12Renderer::GetDevice()->CreateShaderResourceView(
		m_TextureBuffer,
		&srvd,
		m_basicDescHeap->GetCPUDescriptorHandleForHeapStart()
	);


	return S_OK;
}

void Polygon2D::PolygonRotation()
{
	g_angle += 0.01f;
	//XMStoreFloat4x4(&m_Model.mappedMatrices[0], XMMatrixRotationY(g_angle));
	//m_Model.MapMatrix->viewproj = m_Model.viewMat * m_Model.projMat;
	m_Model.mappedMatrices[0] = XMMatrixRotationY(g_angle);
}

void Polygon2D::PolygonMove()
{
	g_posX += 0.01f;
	//m_Model.MapMatrix->world = XMMatrixTranslation(g_posX, 0.0f, 0.0f);
}
