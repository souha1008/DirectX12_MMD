#include "framework.h"
#include "DX12Renderer.h"
#include "3DObject.h"

using namespace DirectX;

// テクスチャ情報
std::vector<TEXRGBA> g_Texture(256 * 256);


HRESULT Object3D::CreateModel(const char* Filename, const char* Motionname, MODEL_DX12* Model)
{
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// ラムダ式初期化
	CreateLambdaTable();

	// pmdファイル読み込み
	PMDHeader pmdheader;

	char signature[3] = {};	// シグネチャ
	std::string ModelPath = Filename;
	auto fp = fopen(ModelPath.c_str(), "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	// 頂点読み込み
	unsigned int vertNum = 0;	// 頂点数
	fread(&vertNum, sizeof(vertNum), 1, fp);

	constexpr size_t pmdVertex_size = 38;	// 1頂点あたりのサイズ
	std::vector<PMDVertex> vertices(vertNum);	// バッファー確保
	for (unsigned int i = 0; i < vertNum; i++)
	{
		fread(&vertices[i], pmdVertex_size, 1, fp);
	}
	Model->sub.vertNum = vertNum;

	// インデックスバッファー読み込み
	unsigned int indicesNum = 0;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	std::vector<unsigned short> indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	Model->sub.indecesNum = indicesNum;

	// 頂点バッファー作成
	hr = CreateVertexBuffer(Model, vertices);

	// 頂点バッファービュー設定
	hr = SettingVertexBufferView(Model, vertices, pmdVertex_size);

	// インデックスバッファー生成
	hr = CreateIndexBuffer(Model, indices);

	// インデックスバッファビュー設定
	hr = SettingIndexBufferView(Model, indices);

	// マテリアル読み込み
	hr = LoadMaterial(fp, Model, ModelPath);

	// ボーンの読み込み
	hr = CreateBone(fp, Model);

	// モーションデータ読み込み
	hr = LoadVMDData(fopen(Motionname, "rb"), Model);

	// 定数バッファー&ビュー生成
	hr = CreateSceneCBuffer(Model);

	hr = CreateTransformCBuffer(Model);

	hr = CreateMaterialView(Model);
	
	// ファイルクローズ
	fclose(fp);
	return hr;

}

HRESULT Object3D::CreateVertexBuffer(MODEL_DX12* Model, std::vector<PMDVertex> vertices)
{
	HRESULT hr;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMDVertex));		// d3d12x.h

	// 頂点バッファー生成
	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,	// UPLOADヒープ
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,		// サイズに応じて適切な設定をする
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(Model->VertexBuffer.ReleaseAndGetAddressOf())
	);

	// 頂点バッファーマップ
	PMDVertex* vertMap = nullptr;
	hr = Model->VertexBuffer.Get()->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	Model->VertexBuffer.Get()->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::SettingVertexBufferView(MODEL_DX12* Model, std::vector<PMDVertex> vertices, size_t pmdVertex_size)
{
	Model->vbView.BufferLocation = Model->VertexBuffer.Get()->GetGPUVirtualAddress(); // バッファの仮想アドレス
	Model->vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMDVertex));	// 全バイト数
	Model->vbView.StrideInBytes = sizeof(PMDVertex);	// 1頂点あたりのバイト数

	if (Model->vbView.SizeInBytes <= 0 || Model->vbView.StrideInBytes <= 0)
	{
		return ERROR;
	}

	return S_OK;
}

HRESULT Object3D::CreateIndexBuffer(MODEL_DX12* Model, std::vector<unsigned short> index)
{
	HRESULT hr;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(index.size() * sizeof(index[0]));		// d3d12x.h

	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(Model->IndexBuffer.ReleaseAndGetAddressOf())
	);
	// インデックスバッファーマップ
	unsigned short* indMap = nullptr;
	Model->IndexBuffer.Get()->Map(0, nullptr, (void**)&indMap);
	std::copy(std::begin(index), std::end(index), indMap);
	Model->IndexBuffer.Get()->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::SettingIndexBufferView(MODEL_DX12* Model, std::vector<unsigned short> index)
{
	Model->ibView.BufferLocation = Model->IndexBuffer.Get()->GetGPUVirtualAddress();
	Model->ibView.Format = DXGI_FORMAT_R16_UINT;
	Model->ibView.SizeInBytes = static_cast<UINT>(index.size() * sizeof(index[0]));
	return S_OK;
}

HRESULT Object3D::CreateSceneCBuffer(MODEL_DX12* Model)
{
	auto buffersize = sizeof(SCENEMATRIX);
	buffersize = (buffersize + 0xff) & ~0xff;
	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(buffersize);		// d3d12x.h

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(Model->SceneConstBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		assert(SUCCEEDED(hr));
		return hr;
	}

	hr = Model->SceneConstBuffer->Map(0, nullptr, (void**)&Model->SceneMatrix);

	// 視線
	XMFLOAT3 eye(0, 10, -35);
	// 注視点
	XMFLOAT3 target(0, 10, 0);
	// 上ベクトル
	XMFLOAT3 v_up(0, 1, 0);

	Model->SceneMatrix->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&v_up));
	Model->SceneMatrix->proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,//画角は90°
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//アス比
		0.1f,//近い方
		1000.0f//遠い方
	);

	Model->SceneMatrix->eye = eye;

	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダーから見える
	dhd.NodeMask = 0;	// マスク0
	dhd.NumDescriptors = 1;	// ビューは今のところ１つだけ
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(Model->sceneDescHeap.ReleaseAndGetAddressOf()));

	auto heapHandle = Model->sceneDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {};
	cbvd.BufferLocation = Model->SceneConstBuffer.Get()->GetGPUVirtualAddress();
	cbvd.SizeInBytes = static_cast<UINT>(Model->SceneConstBuffer.Get()->GetDesc().Width);

	DX12Renderer::GetDevice()->CreateConstantBufferView(&cbvd, heapHandle);

	return hr;
}

HRESULT Object3D::CreateTransformCBuffer(MODEL_DX12* Model)
{
	auto bufferSize = sizeof(TRANSFORM) * (1 + Model->BoneMatrix.size());
	bufferSize = (bufferSize + 0xff) & ~0xff;
	auto cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto cd_rd = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(Model->TransfromConstBuffer.ReleaseAndGetAddressOf()));

	XMMATRIX WorldMatrix = XMMatrixIdentity();

	// マップ
	hr = Model->TransfromConstBuffer.Get()->Map(0, nullptr, (void**)&Model->mappedMatrices);
	//XMStoreFloat4x4(&Model->mappedMatrices[0].world, WorldMatrix);
	Model->mappedMatrices[0] = WorldMatrix;

	RecursiveMatrixMultiply(Model, &m_BoneNodeTable["センター"], XMMatrixIdentity());

	// マトリクスのコピー
	std::copy(Model->BoneMatrix.begin(), Model->BoneMatrix.end(), Model->mappedMatrices + 1);

	D3D12_DESCRIPTOR_HEAP_DESC transform_dhd = {};
	transform_dhd.NumDescriptors = 1;
	transform_dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transform_dhd.NodeMask = 0;
	transform_dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(&transform_dhd, IID_PPV_ARGS(Model->transformDescHeap.ReleaseAndGetAddressOf()));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdvDesc = {};
	cdvDesc.BufferLocation = Model->TransfromConstBuffer.Get()->GetGPUVirtualAddress();
	cdvDesc.SizeInBytes = static_cast<UINT>(bufferSize);

	auto heapHandle = Model->transformDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	DX12Renderer::GetDevice()->CreateConstantBufferView(&cdvDesc, heapHandle);

	return hr;
}

HRESULT Object3D::CreateTextureData(MODEL_DX12* Model)
{

	////for (TEXRGBA& rgba : g_Texture)
	////{
	////	rgba.R = 0;
	////	rgba.G = 0;
	////	rgba.B = 0;
	////	rgba.A = 255;
	////}
	//ScratchImage ScratchImg = {};

	//HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	//// WICテクスチャのロード
	//hr = LoadFromWICFile(
	//	L"img/textest.png",
	//	WIC_FLAGS_NONE,
	//	&Model->MetaData,
	//	ScratchImg
	//);

	//// 生データ抽出
	//const Image* img = ScratchImg.GetImage(0, 0, 0);

	//// WriteToSubresourceで転送するためのヒープ設定
	//D3D12_HEAP_PROPERTIES heapprop = {};

	//// 特殊な設定なのでDEFAULTでもUPLOADでもない
	//heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;

	////ライトバック
	//heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

	//// 転送はL0、CPU側から直接行う
	//heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	//// 単一アダプターのため0
	//heapprop.CreationNodeMask = 0;
	//heapprop.VisibleNodeMask = 0;

	//// リソース設定
	//D3D12_RESOURCE_DESC rd = {};

	////RBGAフォーマット
	////rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	////rd.Width = 256;		// テクスチャの幅
	////rd.Height = 256;	// テクスチャの高さ
	////rd.DepthOrArraySize = 1;	// 2Dで配列でもないので１
	////rd.SampleDesc.Count = 1;	// 通常テクスチャなのでアンチエイリアシングしない
	////rd.SampleDesc.Quality = 0;	// クオリティは最低
	////rd.MipLevels = 1;	// 読み込んだメタデータのミップレベル参照
	////rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2Dテクスチャ用
	////rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// レイアウトは決定しない
	////rd.Flags = D3D12_RESOURCE_FLAG_NONE;	// フラグなし

	//// テクスチャフォーマット
	//rd.Format = Model->MetaData.format;
	//rd.Width = Model->MetaData.width;		// テクスチャの幅
	//rd.Height = Model->MetaData.height;	// テクスチャの高さ
	//rd.DepthOrArraySize = Model->MetaData.arraySize;	// 2Dで配列でもないので１
	//rd.SampleDesc.Count = 1;	// 通常テクスチャなのでアンチエイリアシングしない
	//rd.SampleDesc.Quality = 0;	// クオリティは最低
	//rd.MipLevels = Model->MetaData.mipLevels;	// 読み込んだメタデータのミップレベル参照
	//rd.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(Model->MetaData.dimension);	// 2Dテクスチャ用
	//rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// レイアウトは決定しない
	//rd.Flags = D3D12_RESOURCE_FLAG_NONE;	// フラグなし

	//hr = DX12Renderer::GetDevice()->CreateCommittedResource(
	//	&heapprop,
	//	D3D12_HEAP_FLAG_NONE,
	//	&rd,
	//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//	nullptr,
	//	IID_PPV_ARGS(&Model->TextureBuffer)
	//);

	//// RGBA
	//hr = Model->TextureBuffer->WriteToSubresource(
	//	0,
	//	nullptr,
	//	g_Texture.data(),	// 元データアドレス
	//	sizeof(TEXRGBA) * 256,	// 1ラインサイズ
	//	sizeof(TEXRGBA) * g_Texture.size()
	//);


	//// テクスチャ
	////hr = Model->TextureBuffer->WriteToSubresource(
	////	0,
	////	nullptr,
	////	img->pixels,	// 元データアドレス
	////	img->rowPitch,	// 1ラインサイズ
	////	img->slicePitch
	////);

	return S_OK;
}

ID3D12Resource* Object3D::LoadTextureFromFile(MODEL_DX12* Model, std::string& texPath)
{
	auto it = m_resourceTable.find(texPath);
	if (it != m_resourceTable.end())
	{
		// テーブル内にあったらロードするのではなくマップ内のリソースを返す
		return m_resourceTable[texPath];
	}


	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	auto wtexpath = GetWideStringFromString(texPath); // テクスチャファイルのパス
	auto ext = GetExtension(texPath);	// 拡張子を取得

	HRESULT hr = m_loadLambdaTable[ext](wtexpath, &metadata, scratchImg);

	if (FAILED(hr))
	{
		return nullptr;
	}

	const Image* img = scratchImg.GetImage(0, 0, 0);	// 生データ抽出

	// WriteToSubresourceで転送する用のヒープ設定
	
	//D3D12_HEAP_PROPERTIES texhp = {};
	//texhp.Type = D3D12_HEAP_TYPE_CUSTOM;
	//texhp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	//texhp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	//texhp.CreationNodeMask = 0;	// 単一アダプタのため0
	//texhp.VisibleNodeMask = 0;	// 単一アダプタのため0

	CD3DX12_HEAP_PROPERTIES texhp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	

	// リソース設定

	//D3D12_RESOURCE_DESC rd = {};
	//rd.Format = metadata.format;
	//rd.Width = metadata.width;
	//rd.Height = metadata.height;
	//rd.DepthOrArraySize = metadata.arraySize;
	//rd.SampleDesc.Count = 1;	// 通常テクスチャなのでアンチエイリアシングしない
	//rd.SampleDesc.Quality = 0;	// クオリティは0
	//rd.MipLevels = metadata.mipLevels;
	//rd.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	//rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	//rd.Flags = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format, 
		metadata.width, 
		metadata.height,
		metadata.arraySize,
		metadata.mipLevels);

	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&texhp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(Model->TextureBuffer.ReleaseAndGetAddressOf())
	);

	hr = Model->TextureBuffer.Get()->WriteToSubresource(
		0, nullptr, img->pixels, img->rowPitch, img->slicePitch
	);

	if (FAILED(hr))
	{
		return nullptr;
	}

	m_resourceTable[texPath] = Model->TextureBuffer.Get();

	return Model->TextureBuffer.Get();
}

HRESULT Object3D::LoadMaterial(FILE* file, MODEL_DX12* Model, std::string ModelPath)
{
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, file);
	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, file);

	Model->material.resize(materialNum);
	Model->TextureResource.resize(materialNum); // マテリアルの数分テクスチャのリソース分確保
	Model->sphResource.resize(materialNum);	// 同様
	Model->spaResource.resize(materialNum);
	Model->toonResource.resize(materialNum);

	// コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		Model->material[i].indicesNum = pmdMaterials[i].indicesNum;
		Model->material[i].material.diffuse = pmdMaterials[i].diffuse;
		Model->material[i].material.alpha = pmdMaterials[i].alpha;
		Model->material[i].material.specular = pmdMaterials[i].specular;
		Model->material[i].material.specularity = pmdMaterials[i].specularity;
		Model->material[i].material.ambient = pmdMaterials[i].ambient;
		Model->material[i].additional.toonIdx = pmdMaterials[i].toonIdx;
	}
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		// トゥーンリソース読み込み
		std::string toonFilePath = "Assets/Texture/Toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, 16,"toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		toonFilePath += toonFileName;
		Model->toonResource[i] = LoadTextureFromFile(Model, toonFilePath);

		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			Model->TextureResource[i] = nullptr;
			//continue;
		}

		// モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";
		
		if(std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{ 
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph")
			{
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else
			{
				texFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph")
				{
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa")
				{
					spaFileName = namepair.second;
				}
			}
		}
		else
		{
			if (GetExtension(pmdMaterials[i].texFilePath) == "sph")
			{
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (GetExtension(pmdMaterials[i].texFilePath) == "spa")
			{
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else
			{
				texFileName = pmdMaterials[i].texFilePath;
			}
		}
		if (texFileName != "")
		{
			auto texFilePath = GetTexturePathFromModelandTexPath(ModelPath, texFileName.c_str());
			Model->TextureResource[i] = LoadTextureFromFile(Model, texFilePath);
		}
		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelandTexPath(ModelPath, sphFileName.c_str());
			Model->sphResource[i] = LoadTextureFromFile(Model, sphFilePath);
		}
		if (spaFileName != "")
		{
			auto spaFilePath = GetTexturePathFromModelandTexPath(ModelPath, spaFileName.c_str());
			Model->spaResource[i] = LoadTextureFromFile(Model, spaFilePath);
		}

	}

	Model->sub.materialNum = materialNum;

	// マテリアルバッファー作成
	HRESULT hr = CreateMaterialBuffer(Model);

	return hr;
}

HRESULT Object3D::CreateMaterialBuffer(MODEL_DX12* Model)
{
	unsigned long long materialBufferSize = sizeof(MaterialForHlsl);
	materialBufferSize = (materialBufferSize + 0xff) & ~0xff;
	Model->sub.materialBufferSize = materialBufferSize;

	CD3DX12_HEAP_PROPERTIES hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer(materialBufferSize * Model->sub.materialNum);

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(Model->MaterialBuffer.ReleaseAndGetAddressOf())
	);

	char* mapMaterial = nullptr;

	hr = Model->MaterialBuffer.Get()->Map(0, nullptr, (void**)&mapMaterial);

	for (auto& m : Model->material)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;	// データコピー
		mapMaterial += materialBufferSize;	// 次のアライメント位置まで進める
	}
	Model->MaterialBuffer.Get()->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::CreateMaterialView(MODEL_DX12* Model)
{
	// マテリアル用ディスクリプター作成
	D3D12_DESCRIPTOR_HEAP_DESC mat_dhd = {};
	mat_dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	mat_dhd.NodeMask = 0;
	mat_dhd.NumDescriptors = Model->sub.materialNum * 5;	// マテリアル数分（スフィアマテリアル追加）+ テクスチャ
	mat_dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	HRESULT hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&mat_dhd,
		IID_PPV_ARGS(Model->materialDescHeap.ReleaseAndGetAddressOf())
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = 1;

	// ビュー作成
	auto materialBufsize = sizeof(MaterialForHlsl);
	materialBufsize = (materialBufsize + 0xff) & ~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbvd = {};
	mat_cbvd.BufferLocation = Model->MaterialBuffer.Get()->GetGPUVirtualAddress();
	mat_cbvd.SizeInBytes = materialBufsize;

	// 先頭を記録
	D3D12_CPU_DESCRIPTOR_HANDLE mat_dHandle = Model->materialDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	UINT inc = DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (unsigned int i = 0; i < Model->sub.materialNum; i++)
	{
		DX12Renderer::GetDevice()->CreateConstantBufferView(
			&mat_cbvd,
			mat_dHandle);

		mat_dHandle.ptr += inc;
		mat_cbvd.BufferLocation += materialBufsize;

		if (Model->TextureResource[i] == nullptr)
		{
			srvd.Format = CreateWhiteTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				CreateWhiteTexture(),
				&srvd,
				mat_dHandle);
		}
		else
		{
			srvd.Format = Model->TextureResource[i].Get()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				Model->TextureResource[i].Get(),
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;

		// 拡張子がsphだったとき、sphResourceにポインタを入れる
		if (Model->sphResource[i] == nullptr)
		{
			srvd.Format = CreateWhiteTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				CreateWhiteTexture(),
				&srvd,
				mat_dHandle);
		}
		else
		{
			srvd.Format = Model->sphResource[i].Get()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				Model->sphResource[i].Get(),
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;

		// 拡張子がspaだったとき、sphResourceにポインタを入れる
		if (Model->spaResource[i] == nullptr)
		{
			srvd.Format = CreateBlackTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				CreateBlackTexture(),
				&srvd,
				mat_dHandle);
		}
		else
		{
			srvd.Format = Model->spaResource[i].Get()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				Model->spaResource[i].Get(),
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;

		if (Model->toonResource[i] == nullptr)
		{
			srvd.Format = CreateGrayGradationTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(CreateGrayGradationTexture(), &srvd, mat_dHandle);
		}
		else
		{
			srvd.Format = Model->toonResource[i].Get()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				Model->toonResource[i].Get(),
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;
	}

	return S_OK;
}

void Object3D::RecursiveMatrixMultiply(MODEL_DX12* Model, BoneNode* node, const XMMATRIX& mat)
{
	Model->BoneMatrix[node->boneIdx] *= mat;
	
	for (auto& cnode : node->children)
	{
		RecursiveMatrixMultiply(Model, cnode, Model->BoneMatrix[node->boneIdx]);
	}
}

std::string Object3D::GetTexturePathFromModelandTexPath(const std::string& modelPath, const char* texPath)
{
	int pathInd1 = modelPath.rfind('/');
	int pathInd2 = modelPath.rfind('\\');
	auto pathIndex = max(pathInd1, pathInd2);
	auto folderPath = modelPath.substr(0, pathIndex+1);
	return folderPath + texPath;
}

std::wstring Object3D::GetWideStringFromString(const std::string& str)
{
	// 呼び出し一回目
	int num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr;	// stringのwchar_t版
	wstr.resize(num1);	// 得られた文字列数でリサイズ

	// 呼び出し二回目
	int num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1);

	assert(num1 == num2);

	return wstr;
}

ID3D12Resource* Object3D::CreateDefaultTexture(size_t width, size_t height)
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	ID3D12Resource* buff = nullptr;
	auto result = DX12Renderer::GetDevice()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&buff)
	);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return nullptr;
	}
	return buff;
}

ID3D12Resource* Object3D::CreateWhiteTexture()
{
	CD3DX12_HEAP_PROPERTIES tex_hp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 1, 0);

	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ID3D12Resource* whiteBuff = CreateDefaultTexture(4, 4);

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&tex_hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff)
	);

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	// データ転送
	hr = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size()
	);

	return whiteBuff;
}

ID3D12Resource* Object3D::CreateBlackTexture()
{
	CD3DX12_HEAP_PROPERTIES tex_hp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 1, 0);
	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ID3D12Resource* blackBuff = CreateDefaultTexture(4, 4);

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&tex_hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&blackBuff)
	);

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	// データ転送
	hr = blackBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size()
	);

	return blackBuff;
}

ID3D12Resource* Object3D::CreateGrayGradationTexture()
{
	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	// 上が白くて下が黒いテクスチャ作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		auto col = (c << 0xff) | (c << 16) | (c << 80) | c;
		std::fill(it, it + 4, col);
		--c;
	}

	ID3D12Resource* gradBuff = CreateDefaultTexture(4, 256);

	HRESULT hr = gradBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size()
	);

	return gradBuff;
}

std::string Object3D::GetExtension(const std::string& path)
{
	int ind = path.rfind('.');

	return path.substr(ind + 1, path.length() - ind - 1);
}

std::pair<std::string, std::string> Object3D::SplitFileName(const std::string& path, const char splitter)
{
	long idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	
	return ret;
}

void Object3D::UnInit(MODEL_DX12* Model)
{
	Model->VertexBuffer.ReleaseAndGetAddressOf();
	Model->IndexBuffer.ReleaseAndGetAddressOf();
	Model->SceneConstBuffer.ReleaseAndGetAddressOf();
	Model->TextureBuffer.ReleaseAndGetAddressOf();
	Model->MaterialBuffer.ReleaseAndGetAddressOf();
	Model->materialDescHeap.ReleaseAndGetAddressOf();
	Model->sceneDescHeap.ReleaseAndGetAddressOf();
	Model->transformDescHeap.ReleaseAndGetAddressOf();
}

float Object3D::GetYFromXonBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)
	{
		return x;	// 計算不要
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;	// t^3の係数
	const float k1 = 3 * b.x - 6 * a.x;		// t^2の係数
	const float k2 = 3 * a.x;				// tの係数

	// 誤差の範囲内かどうかに使用する定数
	constexpr float epsilon = 0.0005f;

	// tを禁じで求める
	for (int i = 0; i < n; i++)
	{
		// f(t)を求める
		float ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		// もし結果が０に近いなら打ち切る
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}

		t -= ft / 2;
	}

	float r = 1 - t;

	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

void Object3D::MotionUpdate(MODEL_DX12* Model)
{
	auto elapsedTime = timeGetTime() - m_StartTime;	// 経過時間
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);	// 経過フレーム数計算

	// ループするため
	if (frameNo > m_duration)
	{
		m_StartTime = timeGetTime();
		frameNo = 0;
	}

	// 行列情報クリア（クリアしていないと前フレームのポーズが重ねがけされてモデルが壊れる）
	std::fill(Model->BoneMatrix.begin(), Model->BoneMatrix.end(), XMMatrixIdentity());

	// モーションデータ更新
	for (auto& bonemotion : Model->MotionData)
	{
		auto node = m_BoneNodeTable[bonemotion.first];
		
		// 合致するものを探す
		auto motions = bonemotion.second;
		// リバースイテレーターで逆順を取得
		auto rit = std::find_if(
			motions.rbegin(), motions.rend(),
			[frameNo](const KeyFrame& motion) {return motion.frameNo <= frameNo; });

		// 合地するものがなけらば処理を飛ばす
		if (rit == motions.rend())
		{
			continue;
		}

		// t = 現在フレーム - 直前キーフレーム / 直後キーフレーム - 直前キーフレーム

		XMMATRIX rotation;
		auto it = rit.base();

		if (it != motions.end())
		{
			auto t = static_cast<float>(frameNo - rit->frameNo) / static_cast<float>(it->frameNo - rit->frameNo);
			// 線形補間
			//rotation = XMMatrixRotationQuaternion(rit->quaternion)
			//	* (1 - t)
			//	+ XMMatrixRotationQuaternion(it->quaternion)
			//	* t;

			t = GetYFromXonBezier(t, it->p1, it->p2, 12);
			// 球面線形補間
			rotation = XMMatrixRotationQuaternion(XMQuaternionSlerp(rit->quaternion, it->quaternion, t));
		}
		else
		{
			rotation = XMMatrixRotationQuaternion(rit->quaternion);
		}

		XMFLOAT3& pos = node.startPos;

		XMMATRIX mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* XMMatrixTranslation(pos.x, pos.y, pos.z);
		Model->BoneMatrix[node.boneIdx] = mat;
	}
	RecursiveMatrixMultiply(Model, &m_BoneNodeTable["センター"], XMMatrixIdentity());

	// マトリクスのコピー
	std::copy(Model->BoneMatrix.begin(), Model->BoneMatrix.end(), Model->mappedMatrices + 1);

}

HRESULT Object3D::CreateBone(FILE* file, MODEL_DX12* Model)
{
	unsigned short boneNum;
	fread(&boneNum, sizeof(boneNum), 1, file);

	std::vector<PMDBone> pmdBone(boneNum);
	fread(pmdBone.data(), sizeof(PMDBone), boneNum, file);

	std::vector<std::string> boneNames(pmdBone.size());

	// ボーンノードマップを作成
	for (int index = 0; index < pmdBone.size(); ++index)
	{
		auto& pmdbones = pmdBone[index];
		boneNames[index] = pmdbones.boneName;
		auto& node = m_BoneNodeTable[pmdbones.boneName];
		node.boneIdx = index;
		node.startPos = pmdbones.pos;
	}

	// 親子関係を構築
	for (auto& pmdbones : pmdBone)
	{
		// 親インデックスをチェック
		if (pmdbones.parentNo >= pmdBone.size())
		{
			continue;
		}

		auto parentName = boneNames[pmdbones.parentNo];
		m_BoneNodeTable[parentName].children.emplace_back(&m_BoneNodeTable[pmdbones.boneName]);
	}

	Model->BoneMatrix.resize(pmdBone.size());

	std::fill(Model->BoneMatrix.begin(), Model->BoneMatrix.end(), XMMatrixIdentity());

	return S_OK;
}

HRESULT Object3D::LoadVMDData(FILE* file, MODEL_DX12* Model)
{
	fseek(file, 50, SEEK_SET);

	unsigned int motionDataNum = 0;
	fread(&motionDataNum, sizeof(motionDataNum), 1, file);

	std::vector<VMDMotion> vmdMotionData(motionDataNum);

	// モーションデータ読み込み
	for (auto& motion : vmdMotionData)
	{
		fread(motion.boneName, sizeof(motion.boneName), 1, file);	// ボーン名取得
		fread(&motion.frameNo,
			sizeof(motion.frameNo)			//フレーム番号
			+ sizeof(motion.location)		// 
			+ sizeof(motion.quaternion)
			+ sizeof(motion.bezier),
			1, file);
		m_duration = std::max<unsigned int>(m_duration, motion.frameNo);
	}

	for (auto& vmdmotion : vmdMotionData)
	{
		XMVECTOR q = XMLoadFloat4(&vmdmotion.quaternion);
		Model->MotionData[vmdmotion.boneName].emplace_back(
			KeyFrame(
				vmdmotion.frameNo, 
				q, 
				XMFLOAT2((float)vmdmotion.bezier[3] / 127.0f, (float)vmdmotion.bezier[7] / 127.0f),
				XMFLOAT2((float)vmdmotion.bezier[11] / 127.0f, (float)vmdmotion.bezier[15] / 127.0f)));
	}

	for (auto& motion : Model->MotionData)
	{
		sort(motion.second.begin(), motion.second.end(),
			[](const KeyFrame& lval, const KeyFrame& rval) {return lval.frameNo <= rval.frameNo; });
	}

	// クォータニオン適応
	for (auto& bonemotion : Model->MotionData)
	{
		// アニメーションデータからボーン名検索
		auto itBonenode = m_BoneNodeTable.find(bonemotion.first);
		if (itBonenode == m_BoneNodeTable.end())
		{
			continue;
		}

		auto& node = itBonenode->second;
		auto& pos = node.startPos;
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
				 * XMMatrixRotationQuaternion(bonemotion.second[0].quaternion)
				 * XMMatrixTranslation(pos.x, pos.y, pos.z);
		Model->BoneMatrix[node.boneIdx] = mat;
	}

	return S_OK;
}

void Object3D::CreateLambdaTable()
{
	// WIC系
	m_loadLambdaTable["sph"]
		= m_loadLambdaTable["spa"]
		= m_loadLambdaTable["bmp"]
		= m_loadLambdaTable["png"]
		= m_loadLambdaTable["jpg"]
		= [](const std::wstring& path,
			TexMetadata* meta,
			ScratchImage& img)-> HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	m_loadLambdaTable["tga"] = [](
		const std::wstring& path,
		TexMetadata* meta,
		ScratchImage& img)-> HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	m_loadLambdaTable["dds"] = [](
		const std::wstring& path,
		TexMetadata* meta,
		ScratchImage& img)-> HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE,meta, img);
	};
}

void Object3D::PlayAnimation()
{
	m_StartTime = timeGetTime();
}
