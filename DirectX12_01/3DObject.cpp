#include "framework.h"
#include "DX12Renderer.h"
#include "3DObject.h"

using namespace DirectX;

// テクスチャ情報
std::vector<TEXRGBA> g_Texture(256 * 256);

HRESULT Object3D::CreateModel(const char* Filename, MODEL_DX12* Model)
{
	PMDHeader pmdheader;

	char signature[3] = {};	// シグネチャ
	std::string ModelPath = Filename;
	auto fp = fopen(ModelPath.c_str(), "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	// 頂点読み込み
	unsigned int vertNum;	// 頂点数
	fread(&vertNum, sizeof(vertNum), 1, fp);

	constexpr size_t pmdVertex_size = 38;	// 1頂点あたりのサイズ
	std::vector<PMDVertex> vertices(vertNum);	// バッファー確保
	for (int i = 0; i < vertNum; i++)
	{
		fread(&vertices[i], pmdVertex_size, 1, fp);
	}
	Model->sub.vertNum = vertNum;

	// インデックスバッファー読み込み
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	std::vector<unsigned short> indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	Model->sub.indecesNum = indicesNum;



	// 頂点バッファー作成
	HRESULT hr = CreateVertexBuffer(Model, vertices);

	// 頂点バッファービュー設定
	hr = SettingVertexBufferView(Model, vertices, pmdVertex_size);

	// マテリアル読み込み
	hr = LoadMaterial(fp, Model, ModelPath);

	// インデックスバッファー生成
	hr = CreateIndexBuffer(Model, indices);

	// インデックスバッファビュー設定
	hr = SettingIndexBufferView(Model, indices);

	// テクスチャデータセット
	//hr = CreateTextureData(Model);

	// 定数バッファー生成
	hr = CreateConstBuffer(Model);

	// ディスクリプター生成
	hr = CreateBasicDescriptorHeap(Model);

	// シェーダーリソースビュー生成
	hr = CreateShaderResourceView(Model);

	// 定数バッファービュー設定
	D3D12_CPU_DESCRIPTOR_HANDLE basicHeapHandle = Model->basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	//basicHeapHandle.ptr += DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	// ポインタずらし気を付けよう！
	hr = SettingConstBufferView(&basicHeapHandle, Model);

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
		IID_PPV_ARGS(&Model->VertexBuffer)
	);

	// 頂点バッファーマップ
	PMDVertex* vertMap = nullptr;
	hr = Model->VertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	Model->VertexBuffer->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::SettingVertexBufferView(MODEL_DX12* Model, std::vector<PMDVertex> vertices, size_t pmdVertex_size)
{
	Model->vbView.BufferLocation = Model->VertexBuffer->GetGPUVirtualAddress(); // バッファの仮想アドレス
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
		IID_PPV_ARGS(&Model->IndexBuffer)
	);
	// インデックスバッファーマップ
	unsigned short* indMap = nullptr;
	Model->IndexBuffer->Map(0, nullptr, (void**)&indMap);
	std::copy(std::begin(index), std::end(index), indMap);
	Model->IndexBuffer->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::SettingIndexBufferView(MODEL_DX12* Model, std::vector<unsigned short> index)
{
	Model->ibView.BufferLocation = Model->IndexBuffer->GetGPUVirtualAddress();
	Model->ibView.Format = DXGI_FORMAT_R16_UINT;
	Model->ibView.SizeInBytes = static_cast<UINT>(index.size() * sizeof(index[0]));
	return S_OK;
}

HRESULT Object3D::CreateConstBuffer(MODEL_DX12* Model)
{
	XMMATRIX WorldMatrix = XMMatrixRotationY(XM_PIDIV4);

	// 視線
	XMFLOAT3 eye(0, 12, -25);
	// 注視点
	XMFLOAT3 target(0, 12, 0);
	// 上ベクトル
	XMFLOAT3 v_up(0, 1, 0);

	XMMATRIX viewMat;
	XMMATRIX projMat;

	viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&v_up));
	projMat = XMMatrixPerspectiveFovLH(XM_PIDIV4,//画角は90°
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//アス比
		1.0f,//近い方
		100.0f//遠い方
	);


	// スクリーン座標にする
	//matrix.r[0].m128_f32[0] = 2.0f / SCREEN_WIDTH;
	//matrix.r[1].m128_f32[1] = 2.0f / SCREEN_HEIGHT;
	//matrix.r[3].m128_f32[3] = -1.0f;
	//matrix.r[3].m128_f32[3] = 1.0f;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MATRIXDATA) + 0xff) & ~0xff);		// d3d12x.h


	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Model->ConstBuffer)
	);

	hr = Model->ConstBuffer->Map(0, nullptr, (void**)&Model->MapMatrix);

	XMStoreFloat4x4(&Model->MapMatrix->world, WorldMatrix);
	XMStoreFloat4x4(&Model->MapMatrix->viewproj, viewMat * projMat);

	return hr;
}

HRESULT Object3D::SettingConstBufferView(D3D12_CPU_DESCRIPTOR_HANDLE* handle, MODEL_DX12* Model)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {};
	cbvd.BufferLocation = Model->ConstBuffer->GetGPUVirtualAddress();
	cbvd.SizeInBytes = static_cast<UINT>(Model->ConstBuffer->GetDesc().Width);

	DX12Renderer::GetDevice()->CreateConstantBufferView(&cbvd, *handle);

	return S_OK;
}

HRESULT Object3D::CreateBasicDescriptorHeap(MODEL_DX12* Model)
{
	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダーから見える
	dhd.NodeMask = 0;	// マスク0
	dhd.NumDescriptors = 2;	// ビューは今のところ１つだけ
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	HRESULT hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&Model->basicDescHeap));
	return hr;
}

HRESULT Object3D::CreateTextureData(MODEL_DX12* Model)
{

	//for (TEXRGBA& rgba : g_Texture)
	//{
	//	rgba.R = 0;
	//	rgba.G = 0;
	//	rgba.B = 0;
	//	rgba.A = 255;
	//}
	ScratchImage ScratchImg = {};

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// WICテクスチャのロード
	hr = LoadFromWICFile(
		L"img/textest.png",
		WIC_FLAGS_NONE,
		&Model->MetaData,
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

	//RBGAフォーマット
	//rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//rd.Width = 256;		// テクスチャの幅
	//rd.Height = 256;	// テクスチャの高さ
	//rd.DepthOrArraySize = 1;	// 2Dで配列でもないので１
	//rd.SampleDesc.Count = 1;	// 通常テクスチャなのでアンチエイリアシングしない
	//rd.SampleDesc.Quality = 0;	// クオリティは最低
	//rd.MipLevels = 1;	// 読み込んだメタデータのミップレベル参照
	//rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2Dテクスチャ用
	//rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// レイアウトは決定しない
	//rd.Flags = D3D12_RESOURCE_FLAG_NONE;	// フラグなし

	// テクスチャフォーマット
	rd.Format = Model->MetaData.format;
	rd.Width = Model->MetaData.width;		// テクスチャの幅
	rd.Height = Model->MetaData.height;	// テクスチャの高さ
	rd.DepthOrArraySize = Model->MetaData.arraySize;	// 2Dで配列でもないので１
	rd.SampleDesc.Count = 1;	// 通常テクスチャなのでアンチエイリアシングしない
	rd.SampleDesc.Quality = 0;	// クオリティは最低
	rd.MipLevels = Model->MetaData.mipLevels;	// 読み込んだメタデータのミップレベル参照
	rd.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(Model->MetaData.dimension);	// 2Dテクスチャ用
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// レイアウトは決定しない
	rd.Flags = D3D12_RESOURCE_FLAG_NONE;	// フラグなし

	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Model->TextureBuffer)
	);

	// RGBA
	hr = Model->TextureBuffer->WriteToSubresource(
		0,
		nullptr,
		g_Texture.data(),	// 元データアドレス
		sizeof(TEXRGBA) * 256,	// 1ラインサイズ
		sizeof(TEXRGBA) * g_Texture.size()
	);


	// テクスチャ
	//hr = Model->TextureBuffer->WriteToSubresource(
	//	0,
	//	nullptr,
	//	img->pixels,	// 元データアドレス
	//	img->rowPitch,	// 1ラインサイズ
	//	img->slicePitch
	//);

	return hr;
}

ID3D12Resource* Object3D::LoadTextureFromFile(MODEL_DX12* Model, std::string& texPath)
{
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	hr = LoadFromWICFile(
		GetWideStringFromString(texPath).c_str(),
		WIC_FLAGS_NONE,
		&metadata,
		scratchImg
	);

	if (FAILED(hr))
	{
		return nullptr;
	}

	const Image* img = scratchImg.GetImage(0, 0, 0);	// 生データ抽出

	// WriteToSubresourceで転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texhp = {};
	texhp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texhp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texhp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texhp.CreationNodeMask = 0;	// 単一アダプタのため0
	texhp.VisibleNodeMask = 0;	// 単一アダプタのため0

	// リソース設定
	D3D12_RESOURCE_DESC rd = {};
	rd.Format = metadata.format;
	rd.Width = metadata.width;
	rd.Height = metadata.height;
	rd.DepthOrArraySize = metadata.arraySize;
	rd.SampleDesc.Count = 1;	// 通常テクスチャなのでアンチエイリアシングしない
	rd.SampleDesc.Quality = 0;	// クオリティは0
	rd.MipLevels = metadata.mipLevels;
	rd.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rd.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&texhp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Model->TextureBuffer)
	);

	hr = Model->TextureBuffer->WriteToSubresource(
		0, nullptr, img->pixels, img->rowPitch, img->slicePitch
	);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return Model->TextureBuffer;
}

HRESULT Object3D::CreateShaderResourceView(MODEL_DX12* Model)
{
	// シェーダーリソースビューを作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = Model->MetaData.format;	// RGBA(0~1に正規化)
	srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = 1;

	DX12Renderer::GetDevice()->CreateShaderResourceView(
		Model->TextureBuffer,
		&srvd,
		Model->basicDescHeap->GetCPUDescriptorHandleForHeapStart()
	);


	return S_OK;
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

	// マテリアルバッファービュー作成
	hr = CreateMaterialView(Model);

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
		IID_PPV_ARGS(&Model->MaterialBuffer)
	);

	char* mapMaterial = nullptr;

	hr = Model->MaterialBuffer->Map(0, nullptr, (void**)&mapMaterial);

	for (auto& m : Model->material)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;	// データコピー
		mapMaterial += materialBufferSize;	// 次のアライメント位置まで進める
	}
	Model->MaterialBuffer->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::CreateMaterialView(MODEL_DX12* Model)
{
	// マテリアル用ディスクリプター作成
	D3D12_DESCRIPTOR_HEAP_DESC mat_dhd = {};
	mat_dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	mat_dhd.NodeMask = 0;
	mat_dhd.NumDescriptors = Model->sub.materialNum * 4;	// マテリアル数分（スフィアマテリアル追加）
	mat_dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	HRESULT hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&mat_dhd,
		IID_PPV_ARGS(&Model->materialDescHeap)
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = 1;

	// ビュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbvd = {};
	mat_cbvd.BufferLocation = Model->MaterialBuffer->GetGPUVirtualAddress();
	mat_cbvd.SizeInBytes = static_cast<UINT>(Model->sub.materialBufferSize);

	// 先頭を記録
	D3D12_CPU_DESCRIPTOR_HANDLE mat_dHandle = Model->materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	UINT inc = DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (int i = 0; i < Model->sub.materialNum; i++)
	{
		DX12Renderer::GetDevice()->CreateConstantBufferView(
			&mat_cbvd,
			mat_dHandle);

		mat_dHandle.ptr += inc;
		mat_cbvd.BufferLocation += Model->sub.materialBufferSize;

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
			srvd.Format = Model->TextureResource[i]->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				Model->TextureResource[i],
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;

		// 拡張子がsphだったとき、sphResourceにポインタを入れる
		if (Model->sphResource[i] == nullptr)
		{
			srvd.Format = srvd.Format = CreateWhiteTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				CreateWhiteTexture(),
				&srvd,
				mat_dHandle);
		}
		else
		{
			srvd.Format = Model->sphResource[i]->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				Model->sphResource[i],
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;

		// 拡張子がspaだったとき、sphResourceにポインタを入れる
		if (Model->spaResource[i] == nullptr)
		{
			srvd.Format = srvd.Format = CreateBlackTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				CreateBlackTexture(),
				&srvd,
				mat_dHandle);
		}
		else
		{
			srvd.Format = Model->spaResource[i]->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				Model->spaResource[i],
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;
	}

	return S_OK;
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

ID3D12Resource* Object3D::CreateWhiteTexture()
{
	D3D12_HEAP_PROPERTIES tex_hp = {};

	tex_hp.Type = D3D12_HEAP_TYPE_CUSTOM;
	tex_hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	tex_hp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	tex_hp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC rd = {};
	rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rd.Width = 4;
	rd.Height = 4;
	rd.DepthOrArraySize = 1;
	rd.SampleDesc.Count = 1;
	rd.SampleDesc.Quality = 0;
	rd.MipLevels = 1;
	rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rd.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* whiteBuff = nullptr;

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
	D3D12_HEAP_PROPERTIES tex_hp = {};

	tex_hp.Type = D3D12_HEAP_TYPE_CUSTOM;
	tex_hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	tex_hp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	tex_hp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC rd = {};
	rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rd.Width = 4;
	rd.Height = 4;
	rd.DepthOrArraySize = 1;
	rd.SampleDesc.Count = 1;
	rd.SampleDesc.Quality = 0;
	rd.MipLevels = 1;
	rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rd.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* blackBuff = nullptr;

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

std::string Object3D::GetExtension(const std::string& path)
{
	int ind = path.rfind('.');

	return path.substr(ind + 1, path.length() - ind - 1);
}

std::pair<std::string, std::string> Object3D::SplitFileName(const std::string& path, const char splitter)
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	
	return ret;
}

void Object3D::UnInit(MODEL_DX12* Model)
{
	Model->VertexBuffer->Release();
	Model->IndexBuffer->Release();
	Model->ConstBuffer->Release();
	Model->TextureBuffer->Release();
	Model->MaterialBuffer->Release();
	Model->materialDescHeap->Release();
	Model->basicDescHeap->Release();
}


