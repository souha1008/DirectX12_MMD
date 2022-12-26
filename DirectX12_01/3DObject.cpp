#include "framework.h"
#include "DX12Renderer.h"
#include "3DObject.h"

using namespace DirectX;

// テクスチャ情報
//std::vector<TEXRGBA> g_Texture(256 * 256);

namespace
{
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, const XMFLOAT3& up, const XMFLOAT3& right)
	{
		// 向かせたい方向（Z軸）
		XMVECTOR vz = lookat;

		// （向かせたい方向を向かせたときの）仮y軸ベクトル
		XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

		//（向かせたい方向を向かせたときの）y軸
		XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		// LookAtとupが同じ方向を向いていたらrightを金順にして作り直す
		if (std::abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
		{
			// 仮のx方向を定義
			vx = XMVector3Normalize(XMLoadFloat3(&right));

			// 向かせたい方向を向かせたときのY軸を計算
			vy = XMVector3Normalize(XMVector3Cross(vz, vx));

			// 真のx軸を計算
			vx = XMVector3Normalize(XMVector3Cross(vy, vz));

		}

		XMMATRIX ret = XMMatrixIdentity();
		ret.r[0] = vx;
		ret.r[1] = vy;
		ret.r[2] = vz;

		return ret;
	}

	
	XMMATRIX LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, const XMFLOAT3& up, const XMFLOAT3& right)
	{
		return XMMatrixTranspose(LookAtMatrix(origin, up, right)) * LookAtMatrix(lookat, up, right);
	}

	enum class BoneType
	{
		Rotation,		// 回転
		RotAndMove,		// 回転＆移動
		IK,				// IK
		Undefined,		// 未定義
		IKChild,		// IK影響ボーン
		RotationChild,	// 回転影響ボーン
		IKDestination,	// IK接続先
		Invisible		// 見えないボーン
	};
}


HRESULT Object3D::CreateModel(const char* Filename, const char* Motionname)
{
	//HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// ラムダ式初期化
	CreateLambdaTable();

	std::string ModelPath = Filename;
	auto fp = fopen(ModelPath.c_str(), "rb");

	// PMDヘッダー読み込み
	LoadPMDHeader(fp);

	// 頂点バッファー&ビュー作成
	HRESULT hr = CreateVertexBuffer(fp);

	// インデックスバッファー&ビュー生成
	hr = CreateIndexBuffer(fp);

	// マテリアル読み込み
	hr = LoadMaterial(fp, ModelPath);

	// ボーンの読み込み
	hr = LoadPMDBone(fp);

	// IKデータ読み込み
	hr = LoadIK(fp);

	// モーションデータ読み込み
	hr = LoadVMDData(fopen(Motionname, "rb"));

	// ワールド用定数バッファ作成
	hr = CreateTransformCBuffer();

	// マテリアル用定数バッファ作成
	hr = CreateMaterialView();
	
	// ファイルクローズ
	fclose(fp);
	return hr;

}

HRESULT Object3D::LoadPMDHeader(FILE* file)
{
	typedef struct
	{
		float version;          // 例：00 00 80 3F == 1.00
		char model_name[20];    // モデル名
		char comment[256];      // モデルコメント
	}PMDHeader;

	// pmdファイル読み込み
	PMDHeader pmdheader;

	char signature[3] = {};	// シグネチャ
	fread(signature, sizeof(signature), 1, file);
	fread(&pmdheader, sizeof(pmdheader), 1, file);
	return S_OK;
}

HRESULT Object3D::CreateVertexBuffer(FILE* file)
{
	HRESULT hr;

	// 頂点読み込み
	unsigned int vertNum = 0;	// 頂点数
	fread(&vertNum, sizeof(vertNum), 1, file);

#pragma pack(push, 1)
	typedef struct
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
		uint16_t boneNo[2];   // ボーン番号
		uint8_t boneWeight;   // ボーン影響度
		uint8_t edgeFlag;     // 輪郭線フラグ 
		uint16_t dummy;
	}PMDVertex;
#pragma pack(pop)

	constexpr size_t pmdVertex_size = 38;	// 1頂点あたりのサイズ
	std::vector<PMDVertex> vertices(vertNum);	// バッファー確保
	for (unsigned int i = 0; i < vertNum; i++)
	{
		fread(&vertices[i], pmdVertex_size, 1, file);
	}
	sub.vertNum = vertNum;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMDVertex));		// d3d12x.h

	// 頂点バッファー生成
	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,	// UPLOADヒープ
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,		// サイズに応じて適切な設定をする
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(VertexBuffer.ReleaseAndGetAddressOf())
	);

	// 頂点バッファーマップ
	PMDVertex* vertMap = nullptr;
	hr = VertexBuffer.Get()->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	VertexBuffer.Get()->Unmap(0, nullptr);

	vbView.BufferLocation = VertexBuffer.Get()->GetGPUVirtualAddress(); // バッファの仮想アドレス
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMDVertex));	// 全バイト数
	vbView.StrideInBytes = sizeof(PMDVertex);	// 1頂点あたりのバイト数

	if (vbView.SizeInBytes <= 0 || vbView.StrideInBytes <= 0)
	{
		return ERROR;
	}

	return hr;
}

HRESULT Object3D::CreateIndexBuffer(FILE* file)
{
	HRESULT hr;

	// インデックスバッファー読み込み
	unsigned int indicesNum = 0;
	fread(&indicesNum, sizeof(indicesNum), 1, file);
	std::vector<unsigned short> indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, file);

	sub.indecesNum = indicesNum;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));		// d3d12x.h

	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(IndexBuffer.ReleaseAndGetAddressOf())
	);
	// インデックスバッファーマップ
	unsigned short* indMap = nullptr;
	IndexBuffer.Get()->Map(0, nullptr, (void**)&indMap);
	std::copy(std::begin(indices), std::end(indices), indMap);
	IndexBuffer.Get()->Unmap(0, nullptr);

	ibView.BufferLocation = IndexBuffer.Get()->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	return hr;
}

HRESULT Object3D::CreateTransformCBuffer()
{
	auto bufferSize = sizeof(TRANSFORM) * (1 + BoneMatrix.size());
	bufferSize = (bufferSize + 0xff) & ~0xff;
	auto cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto cd_rd = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,
		D3D12_HEAP_FLAG_NONE,
		&cd_rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(TransfromConstBuffer.ReleaseAndGetAddressOf()));

	XMMATRIX WorldMatrix = XMMatrixIdentity();

	// マップ
	hr = TransfromConstBuffer.Get()->Map(0, nullptr, (void**)&mappedMatrices);
	//XMStoreFloat4x4(&Model->mappedMatrices[0].world, WorldMatrix);
	mappedMatrices[0] = WorldMatrix;

	RecursiveMatrixMultiply(&m_BoneNodeTable["センター"], XMMatrixIdentity());

	// マトリクスのコピー
	std::copy(BoneMatrix.begin(), BoneMatrix.end(), mappedMatrices + 1);

	D3D12_DESCRIPTOR_HEAP_DESC transform_dhd = {};
	transform_dhd.NumDescriptors = 1;
	transform_dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transform_dhd.NodeMask = 0;
	transform_dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(&transform_dhd, IID_PPV_ARGS(transformDescHeap.ReleaseAndGetAddressOf()));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdvDesc = {};
	cdvDesc.BufferLocation = TransfromConstBuffer.Get()->GetGPUVirtualAddress();
	cdvDesc.SizeInBytes = static_cast<UINT>(bufferSize);

	auto heapHandle = transformDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	DX12Renderer::GetDevice()->CreateConstantBufferView(&cdvDesc, heapHandle);

	
	return hr;
}

ID3D12Resource* Object3D::LoadTextureFromFile(std::string& texPath)
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
	CD3DX12_HEAP_PROPERTIES texhp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	

	// リソース設定
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
		IID_PPV_ARGS(TextureBuffer.ReleaseAndGetAddressOf())
	);

	hr = TextureBuffer.Get()->WriteToSubresource(
		0, nullptr, img->pixels, img->rowPitch, img->slicePitch
	);

	if (FAILED(hr))
	{
		return nullptr;
	}

	m_resourceTable[texPath] = TextureBuffer.Get();

	return TextureBuffer.Get();
}

HRESULT Object3D::LoadMaterial(FILE* file, std::string ModelPath)
{
#pragma pack(push, 1) // ここから1バイトパッキングとなり、アライメントは発生しない
	typedef struct
	{
		XMFLOAT3 diffuse;   // ディフューズ色
		float alpha;        // ディフューズ
		float specularity;   // スペキュラの強さ
		XMFLOAT3 specular;  // スペキュラ色
		XMFLOAT3 ambient;   // アンビエント色
		unsigned char toonIdx;  // トゥーン番号
		unsigned char edgeFlg;  // マテリアルごとの輪郭線フラグ

		// 2バイトのパディングがある
		unsigned int indicesNum;
		char texFilePath[20];   // テクスチャファイルパス + α
	}PMDMaterial;   // パディングが発生しないため70バイト
#pragma pack(pop)  // パッキング指定を解除

	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, file);
	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, file);

	material.resize(materialNum);
	TextureResource.resize(materialNum); // マテリアルの数分テクスチャのリソース分確保
	sphResource.resize(materialNum);	// 同様
	spaResource.resize(materialNum);
	toonResource.resize(materialNum);

	// コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		material[i].indicesNum = pmdMaterials[i].indicesNum;
		material[i].material.diffuse = pmdMaterials[i].diffuse;
		material[i].material.alpha = pmdMaterials[i].alpha;
		material[i].material.specular = pmdMaterials[i].specular;
		material[i].material.specularity = pmdMaterials[i].specularity;
		material[i].material.ambient = pmdMaterials[i].ambient;
		material[i].additional.toonIdx = pmdMaterials[i].toonIdx;
	}
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		// トゥーンリソース読み込み
		std::string toonFilePath = "Assets/Texture/Toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, 16,"toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		toonFilePath += toonFileName;
		toonResource[i] = LoadTextureFromFile(toonFilePath);

		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			TextureResource[i] = nullptr;
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
			TextureResource[i] = LoadTextureFromFile(texFilePath);
		}
		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelandTexPath(ModelPath, sphFileName.c_str());
			sphResource[i] = LoadTextureFromFile(sphFilePath);
		}
		if (spaFileName != "")
		{
			auto spaFilePath = GetTexturePathFromModelandTexPath(ModelPath, spaFileName.c_str());
			spaResource[i] = LoadTextureFromFile(spaFilePath);
		}

	}

	sub.materialNum = materialNum;

	// マテリアルバッファー作成
	HRESULT hr = CreateMaterialBuffer();

	return hr;
}

HRESULT Object3D::CreateMaterialBuffer()
{
	unsigned long long materialBufferSize = sizeof(MaterialForHlsl);
	materialBufferSize = (materialBufferSize + 0xff) & ~0xff;
	sub.materialBufferSize = materialBufferSize;

	CD3DX12_HEAP_PROPERTIES hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer(materialBufferSize * sub.materialNum);

	HRESULT hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(MaterialConstBuffer.ReleaseAndGetAddressOf())
	);

	char* mapMaterial = nullptr;

	hr = MaterialConstBuffer.Get()->Map(0, nullptr, (void**)&mapMaterial);

	for (auto& m : material)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;	// データコピー
		mapMaterial += materialBufferSize;	// 次のアライメント位置まで進める
	}
	MaterialConstBuffer.Get()->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::CreateMaterialView()
{
	// マテリアル用ディスクリプター作成
	D3D12_DESCRIPTOR_HEAP_DESC mat_dhd = {};
	mat_dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	mat_dhd.NodeMask = 0;
	mat_dhd.NumDescriptors = sub.materialNum * 5;	// マテリアル数分（スフィアマテリアル追加）+ テクスチャ
	mat_dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	HRESULT hr = DX12Renderer::GetDevice()->CreateDescriptorHeap(
		&mat_dhd,
		IID_PPV_ARGS(materialDescHeap.ReleaseAndGetAddressOf())
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
	mat_cbvd.BufferLocation = MaterialConstBuffer.Get()->GetGPUVirtualAddress();
	mat_cbvd.SizeInBytes = materialBufsize;

	// 先頭を記録
	D3D12_CPU_DESCRIPTOR_HANDLE mat_dHandle = materialDescHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	UINT inc = DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (unsigned int i = 0; i < sub.materialNum; i++)
	{
		DX12Renderer::GetDevice()->CreateConstantBufferView(
			&mat_cbvd,
			mat_dHandle);

		mat_dHandle.ptr += inc;
		mat_cbvd.BufferLocation += materialBufsize;

		if (TextureResource[i] == nullptr)
		{
			srvd.Format = CreateWhiteTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				CreateWhiteTexture(),
				&srvd,
				mat_dHandle);
		}
		else
		{
			srvd.Format = TextureResource[i].Get()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				TextureResource[i].Get(),
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;

		// 拡張子がsphだったとき、sphResourceにポインタを入れる
		if (sphResource[i] == nullptr)
		{
			srvd.Format = CreateWhiteTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				CreateWhiteTexture(),
				&srvd,
				mat_dHandle);
		}
		else
		{
			srvd.Format = sphResource[i].Get()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				sphResource[i].Get(),
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;

		// 拡張子がspaだったとき、sphResourceにポインタを入れる
		if (spaResource[i] == nullptr)
		{
			srvd.Format = CreateBlackTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				CreateBlackTexture(),
				&srvd,
				mat_dHandle);
		}
		else
		{
			srvd.Format = spaResource[i].Get()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				spaResource[i].Get(),
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;

		if (toonResource[i] == nullptr)
		{
			srvd.Format = CreateGrayGradationTexture()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(CreateGrayGradationTexture(), &srvd, mat_dHandle);
		}
		else
		{
			srvd.Format = toonResource[i].Get()->GetDesc().Format;
			DX12Renderer::GetDevice()->CreateShaderResourceView(
				toonResource[i].Get(),
				&srvd,
				mat_dHandle
			);
		}
		mat_dHandle.ptr += inc;
	}

	return S_OK;
}

void Object3D::SolveCosineIK(const PMDIK& ik)
{
	std::vector<XMVECTOR> positions; // IK構成点を保存
	std::array<float, 2> edgeLens;	// IKEAのそれぞれのボーン間の距離を保存

	// ターゲット（末端ボーンではなく、末端ボーンが近づく目標ボーンの座標を取得）
	auto& targetNode = m_boneNodeAddressArray[ik.boneIdx];
	auto targetPos = XMVector3Transform(XMLoadFloat3(&targetNode->startPos), BoneMatrix[ik.boneIdx]);

	// IKチェーンが逆順なので、逆に並ぶようにしている
	// 末端ボーン
	BoneNode* endNode = m_boneNodeAddressArray[ik.targetIdx];
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));

	// 中間及びルートボーン
	for (auto& chainBoneIndex : ik.nodeIdx)
	{
		BoneNode* boneNode = m_boneNodeAddressArray[chainBoneIndex];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	// わかりづらいので逆にする
	std::reverse(positions.begin(), positions.end());

	// もとの長さを測っておく
	edgeLens[0] = XMVector3Length(XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edgeLens[1] = XMVector3Length(XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	// ルートボーン座標変換（逆順になっているため使用するインデックスに注意）
	positions[0] = XMVector3Transform(positions[0], BoneMatrix[ik.nodeIdx[1]]);

	// 真ん中は自動計算

	// 先端ボーン
	positions[2] = XMVector3Transform(positions[2], BoneMatrix[ik.boneIdx]);

	// ルートから先端へのベクトルを作っておく
	XMVECTOR linearVec = XMVectorSubtract(positions[2], positions[0]);

	float A = XMVector3Length(linearVec).m128_f32[0];
	float B = edgeLens[0];
	float C = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);

	// ルートから真ん中への角度計算
	float theta1 = acosf((A * A + B * B - C * C) / (2 * A * B));

	// 真ん中からターゲットへの角度計算
	float theta2 = acosf((B * B + C * C - A * A) / (2 * B * C));

	// 軸を求める
	// もし真ん中が「ひざ」で合った場合には強制的にX軸とする

	XMVECTOR axis;
	if (find(m_kneeIdxes.begin(), m_kneeIdxes.end(), ik.nodeIdx[0]) == m_kneeIdxes.end())
	{
		XMVECTOR vm = XMVector3Normalize(XMVectorSubtract(positions[2], positions[0]));

		XMVECTOR vt = XMVector3Normalize(XMVectorSubtract(targetPos, positions[0]));

		axis = XMVector3Cross(vt, vm);
	}
	else
	{
		XMFLOAT3 right = XMFLOAT3(1, 0, 0);
		axis = XMLoadFloat3(&right);
	}

	// 注意点：IKチェーンはルートに向かってから数えられるため１がルートに近い
	XMMATRIX mat1 = XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= XMMatrixRotationAxis(axis, theta1);
	mat1 *= XMMatrixTranslationFromVector(positions[0]);

	XMMATRIX mat2 = XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= XMMatrixRotationAxis(axis, theta2 - XM_PI);
	mat2 *= XMMatrixTranslationFromVector(positions[1]);

	BoneMatrix[ik.nodeIdx[1]] *= mat1;
	BoneMatrix[ik.nodeIdx[0]] *= mat2 * BoneMatrix[ik.nodeIdx[1]];
	BoneMatrix[ik.targetIdx] = BoneMatrix[ik.nodeIdx[0]];


}

void Object3D::SolveLookAtIK(const PMDIK& ik)
{
	// この関数に来た時点でノードは１つしかなく、
	// チェーンに入っているノード番号はIKEAのルートノードのものなので
	// このルートノードから末端に向かうベクトルを考える

	BoneNode* rootNode = m_boneNodeAddressArray[ik.nodeIdx[0]];
	BoneNode* targetNode = m_boneNodeAddressArray[ik.targetIdx];

	XMVECTOR rpos1 = XMLoadFloat3(&rootNode->startPos);
	XMVECTOR tpos1 = XMLoadFloat3(&targetNode->startPos);

	XMVECTOR rpos2 = XMVector3TransformCoord(rpos1, BoneMatrix[ik.nodeIdx[0]]);
	XMVECTOR tpos2 = XMVector3TransformCoord(tpos1, BoneMatrix[ik.boneIdx]);

	XMVECTOR originVec = XMVectorSubtract(tpos1, rpos1);
	XMVECTOR targetVec = XMVectorSubtract(tpos2, rpos2);

	originVec = XMVector3Normalize(originVec);
	targetVec = XMVector3Normalize(targetVec);
	BoneMatrix[ik.nodeIdx[0]] = LookAtMatrix(originVec, targetVec, XMFLOAT3(0, 1, 0), XMFLOAT3(1, 0, 0));

}

void Object3D::RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat, bool flag)
{
	BoneMatrix[node->boneIdx] *= mat;
	
	for (auto& cnode : node->children)
	{
		RecursiveMatrixMultiply(cnode, BoneMatrix[node->boneIdx]);
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

void Object3D::MotionUpdate()
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
	std::fill(BoneMatrix.begin(), BoneMatrix.end(), XMMatrixIdentity());

	// モーションデータ更新
	for (auto& bonemotion : MotionData)
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
		BoneMatrix[node.boneIdx] = mat;
	}
	RecursiveMatrixMultiply(&m_BoneNodeTable["センター"], XMMatrixIdentity());

	//IKSolve(Model, frameNo);

	// マトリクスのコピー
	std::copy(BoneMatrix.begin(), BoneMatrix.end(), mappedMatrices + 1);

}

HRESULT Object3D::LoadPMDBone(FILE* file)
{
#pragma pack(push, 1)
	typedef struct
	{
		char boneName[20];          // ボーン名
		unsigned short parentNo;    // 親ボーン番号
		unsigned short nextNo;      // 先端のボーン番号
		unsigned char type;         // ボーン種別
		unsigned short  ikBoneNo;   // IKボーン番号
		XMFLOAT3 pos;               // ボーンの基準点座標
	}PMDBone;
#pragma pack(pop)

	unsigned short boneNum;
	fread(&boneNum, sizeof(boneNum), 1, file);

	std::vector<PMDBone> pmdBone(boneNum);
	fread(pmdBone.data(), sizeof(PMDBone), boneNum, file);

	std::vector<std::string> boneNames(pmdBone.size());
	m_BoneNameArray.resize(pmdBone.size());
	m_boneNodeAddressArray.resize(pmdBone.size());
	m_kneeIdxes.clear();
	// ボーンノードマップを作成
	for (int index = 0; index < pmdBone.size(); ++index)
	{
		auto& pmdbones = pmdBone[index];
		boneNames[index] = pmdbones.boneName;
		auto& node = m_BoneNodeTable[pmdbones.boneName];
		node.boneIdx = index;
		node.startPos = pmdbones.pos;

		// インデックス検索しやすいように
		m_BoneNameArray[index] = pmdbones.boneName;
		m_boneNodeAddressArray[index] = &node;
		std::string boneName = pmdbones.boneName;

		if (boneName.find("ひざ") != std::string::npos)
		{
			m_kneeIdxes.emplace_back(index);
		}

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

	BoneMatrix.resize(pmdBone.size());

	std::fill(BoneMatrix.begin(), BoneMatrix.end(), XMMatrixIdentity());

	return S_OK;
}

HRESULT Object3D::LoadVMDData(FILE* file)
{
	typedef struct
	{
		char boneName[15];      // ボーン名
		unsigned int frameNo;   // フレーム番号
		XMFLOAT3 location;      // 位置
		XMFLOAT4 quaternion;    // クォータニオン（回転）
		unsigned char bezier[64]; // ベジェ保管パラメータ
	}VMDMotion;

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

	// 表情データ読み込み
#pragma pack(1)
	struct VMDMorph
	{
		char name[15];		// 名前
		uint32_t frameNo;	// フレーム番号
		float weight;		// ウェイト
	}; // 全部で23バイト
#pragma pack()

	uint32_t vmdMorphCount = 0;
	fread(&vmdMorphCount, sizeof(vmdMorphCount), 1, file);
	std::vector<VMDMorph> vmdMorphsData(vmdMorphCount);
	fread(vmdMorphsData.data(), sizeof(VMDMorph), vmdMorphCount, file);

	// カメラ読み込み
#pragma pack(1)
	struct VMDCamera
	{
		uint32_t frameNo;	// フレーム番号
		float distance;	// 距離
		XMFLOAT3 pos;	// 座標
		XMFLOAT3 eulerAngle; // オイラー角
		uint8_t InterPolation[24]; // 補間
		uint32_t fov;	// 視野角
		uint8_t persFlag;	// パースフラグ
	};	// 61バイト
#pragma pack()

	uint32_t vmdCameraCount = 0;
	fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, file);
	std::vector<VMDCamera> vmdCameraData(vmdCameraCount);
	fread(vmdCameraData.data(), sizeof(VMDCamera), vmdCameraCount, file);

	// ライト読み込み
	struct VMDLight
	{
		uint32_t frameNo;	// フレーム番号
		XMFLOAT3 rgb;	// 色
		XMFLOAT3 vec;	// 光線ベクトル
	};

	uint32_t vmdLightCount = 0;
	fread(&vmdLightCount, sizeof(vmdLightCount), 1, file);
	std::vector<VMDLight> vmdLightData(vmdLightCount);
	fread(vmdLightData.data(), sizeof(VMDLight), vmdLightCount, file);

	// セルフ影データ
#pragma pack(1)
	struct VMDSelfShadow
	{
		uint32_t frameNo;	// フレーム番号
		uint8_t mode;	// 影モード
		float distance;	// 距離
	};
#pragma pack()
	
	uint32_t vmdSelfShadowCount = 0;
	fread(&vmdSelfShadowCount, sizeof(vmdSelfShadowCount), 1, file);
	std::vector<VMDSelfShadow> vmdSelfShadowData(vmdSelfShadowCount);
	fread(vmdSelfShadowData.data(), sizeof(VMDSelfShadow), vmdSelfShadowCount, file);

	// IKオノフ切り替わり数
	uint32_t ikSwitchCount = 0;
	fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, file);

	m_IKEnableData.resize(ikSwitchCount);
	for (auto& IKEnable : m_IKEnableData)
	{
		// キーフレーム情報なのでまずはフレーム番号読み込み
		fread(&IKEnable, sizeof(IKEnable.frameNo), 1, file);

		// 次に可視フラグがあるが使用しない
		uint8_t visibleFlag = 0;
		fread(&visibleFlag, sizeof(visibleFlag), 1, file);

		// 対象ボーン数読み込み
		uint32_t ikBoneCount = 0;
		fread(&ikBoneCount, sizeof(ikBoneCount), 1, file);

		// ループしつつ名前とONOFF情報取得
		for (int i = 0; i < ikBoneCount; i++)
		{
			char ikBoneName[20];
			fread(ikBoneName, sizeof(ikBoneName), 1, file);
			uint8_t flag = 0;
			fread(&flag, sizeof(flag), 1, file);
			IKEnable.ikEnableTable[ikBoneName] = flag;
		}
		
	}

	for (auto& vmdmotion : vmdMotionData)
	{
		XMVECTOR q = XMLoadFloat4(&vmdmotion.quaternion);
		MotionData[vmdmotion.boneName].emplace_back(
			KeyFrame(
				vmdmotion.frameNo, 
				q, 
				vmdmotion.location,
				XMFLOAT2((float)vmdmotion.bezier[3] / 127.0f, (float)vmdmotion.bezier[7] / 127.0f),
				XMFLOAT2((float)vmdmotion.bezier[11] / 127.0f, (float)vmdmotion.bezier[15] / 127.0f)));
	}

	for (auto& motion : MotionData)
	{
		sort(motion.second.begin(), motion.second.end(),
			[](const KeyFrame& lval, const KeyFrame& rval) {return lval.frameNo <= rval.frameNo; });
	}

	// クォータニオン適応
	for (auto& bonemotion : MotionData)
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
		BoneMatrix[node.boneIdx] = mat;
	}

	return S_OK;
}

HRESULT Object3D::LoadIK(FILE* file)
{
	uint16_t IKNum = 0;

	fread(&IKNum, sizeof(IKNum), 1, file);

	pmdIKData.resize(IKNum);

	for (auto& ik : pmdIKData)
	{
		fread(&ik.boneIdx, sizeof(ik.boneIdx), 1, file);
		fread(&ik.targetIdx, sizeof(ik.targetIdx), 1, file);

		uint8_t chainLen = 0;	// 間にいくつノードがあるか
		fread(&chainLen, sizeof(chainLen), 1, file);
		ik.nodeIdx.resize(chainLen);
		fread(&ik.iterations, sizeof(ik.iterations), 1, file);
		fread(&ik.limit, sizeof(ik.limit), 1, file);

		if (chainLen == 0)
		{
			continue;
		}
		fread(ik.nodeIdx.data(), sizeof(ik.nodeIdx[0]), chainLen, file);
	}

	// デバッグウィンドウにIK出力
	auto getNameFromIdx = [&](uint16_t idx)->std::string
	{
		auto it = find_if(m_BoneNodeTable.begin(),
			m_BoneNodeTable.end(),
			[idx](const std::pair<std::string, BoneNode>& obj)
			{
				return obj.second.boneIdx == idx;
			});

		if (it != m_BoneNodeTable.end())
		{
			return it->first;
		}
		else
		{
			return "";
		}
	};
	for (auto& ik : pmdIKData)
	{
		std::ostringstream oss;
		oss << "IKボーン番号=" << ik.boneIdx << "：" << getNameFromIdx(ik.boneIdx) << std::endl;

		for (auto& node : ik.nodeIdx)
		{
			oss << "\tノードボーン=" << node << "：" << getNameFromIdx(node) << std::endl;
		}

		OutputDebugString(oss.str().c_str());
	}

	return S_OK;
}

void Object3D::IKSolve(int frameNo)
{
	// いつもの逆から検索
	auto it = std::find_if(m_IKEnableData.rbegin(),
		m_IKEnableData.rend(),
		[frameNo](const VMDIKEnable& ikenable)
		{
			return ikenable.frameNo <= frameNo;
		});

	// IK解決のためループ
	for (auto& ik : pmdIKData)
	{
		if (it != m_IKEnableData.rend())
		{
			auto ikEnableit = it->ikEnableTable.find(m_BoneNameArray[ik.boneIdx]);

			if (ikEnableit != it->ikEnableTable.end())
			{
				if (!ikEnableit->second)	// もしOFFなら打ち切る
				{
					continue;
				}
			}
		}
		auto childNodesCount = ik.nodeIdx.size();

		switch (childNodesCount)
		{
		case 0:
			assert(0);
			continue;

		case 1:	// 間のボーンの数が１のときはLookAtを使用
			SolveLookAtIK(ik);
			break;

		case 2: // 間のボーンの数が２のときは余弦定理IK
			SolveCosineIK(ik);
			break;
		default: // 間のボーン数が３以上のときはCCD-IK
			SolveCCDIK(ik);
			break;
		}
	}
}

void Object3D::SolveCCDIK(const PMDIK& ik)
{
	// 誤差の範囲内かどうかに使用する定数
	constexpr float epsilon = 0.00005f;

	BoneNode* targetBoneNode = m_boneNodeAddressArray[ik.boneIdx];
	XMVECTOR targetOriginPos = XMLoadFloat3(&targetBoneNode->startPos);

	// 逆行列で無効化する
	XMMATRIX parentMat = BoneMatrix[m_boneNodeAddressArray[ik.boneIdx]->ikParentBone];
	XMVECTOR det;
	XMMATRIX invParentMat = XMMatrixInverse(&det, parentMat);
	XMVECTOR targetNextPos = XMVector3Transform(targetOriginPos, BoneMatrix[ik.boneIdx] * invParentMat);

	std::vector<XMVECTOR> bonePositions;

	// ボーン座標を動かしボーンの回転角度を求めるため現座標を保存
	// 末端ノード
	XMVECTOR endPos = XMLoadFloat3(&m_boneNodeAddressArray[ik.targetIdx]->startPos);

	// 中間ノード（ルート込）
	for (auto& cIndex : ik.nodeIdx)
	{
		bonePositions.push_back(XMLoadFloat3(&m_boneNodeAddressArray[cIndex]->startPos));
	}

	// 末端以外のボーン数だけ行列を確保
	std::vector<XMMATRIX> mats(bonePositions.size());
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());
	
	// PMDデータ特有で、度数法を180°で割った数値が書き込まれているので、ラジアンとして使用するためにXM_PIを乗算する
	float ikLimit = ik.limit * XM_PI;

	for (int c = 0; c < ik.iterations; c++)
	{
		// ターゲットと末端がほぼ一致したら抜ける
		if (XMVector3Length(XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
		{
			break;
		}

		// ボーンを遡りながら角度制限に引っかからないように曲げていく

		// bonePositionsはCCD-IKにおける各ノードの座標をvector配列にしたもの

		for (int bIndex = 0; bIndex < bonePositions.size(); bIndex++)
		{
			const XMVECTOR& pos = bonePositions[bIndex];

			// 対象ノードから末端ノードまでと対象ノードからターゲットまでのベクトル作成
			XMVECTOR vecToEnd = XMVectorSubtract(endPos, pos);	// 末端へ
			XMVECTOR vecToTarget = XMVectorSubtract(targetNextPos, pos); // ターゲットへ
			// 両方正規化
			vecToEnd = XMVector3Normalize(vecToEnd);
			vecToTarget = XMVector3Normalize(vecToTarget);

			// ほぼ同じベクトルになってしまった場合は
			// 外積できないため次のボーンに引き渡す
			if (XMVector3Length(XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0] <= epsilon)
			{
				continue;
			}

			// 外積計算
			auto cross = XMVector3Normalize(XMVector3Cross(vecToEnd, vecToTarget)); // 軸になる
			// 便利な関数だが中身はcosなので0~90と0~-90の区別がない
			float angle = XMVector3AngleBetweenVectors(vecToEnd, vecToTarget).m128_f32[0];
			
			// 回転限界を超えてしまったときは限界値に補正
			angle = min(angle, ikLimit);
			XMMATRIX rot = XMMatrixRotationAxis(cross, angle); // 回転行列作成

			// 原点中心ではなくpos中心に回転
			XMMATRIX mat = XMMatrixTranslationFromVector(-pos) * rot * XMMatrixTranslationFromVector(pos);

			// 回転行列を保持（乗算で回転重ねがけを作っておく）
			mats[bIndex] *= mat;

			// 対象となる点を全て回転させる。なお、自分を回転させる必要ない
			for (int index = bIndex - 1; index >= 0; index--)
			{
				bonePositions[index] = XMVector3Transform(bonePositions[index], mat);
			}

			endPos = XMVector3Transform(endPos, mat);

			// もし正解に近くなっていたらループを抜ける
			if (XMVector3Length(XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
			{
				break;
			}


		}
	}

	int index = 0;
	for (auto& cIndex : ik.nodeIdx)
	{
		BoneMatrix[cIndex] = mats[index];
		index++;
	}
	auto node = m_boneNodeAddressArray[ik.nodeIdx.back()];
	RecursiveMatrixMultiply(node, parentMat, true);

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

void Object3D::Draw()
{
	// プリミティブトポロジ設定
	DX12Renderer::GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点バッファービューセット
	DX12Renderer::GetGraphicsCommandList()->IASetVertexBuffers(0, 1, &vbView);

	// インデックスバッファービューセット
	DX12Renderer::GetGraphicsCommandList()->IASetIndexBuffer(&ibView);

	ID3D12DescriptorHeap* trans_dh[] = { transformDescHeap.Get() };
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, trans_dh);
	auto transform_handle = transformDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(1, transform_handle);

	// マテリアルのディスクリプタヒープセット
	ID3D12DescriptorHeap* mdh[] = { materialDescHeap.Get() };
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, mdh);

	// マテリアル用ディスクリプタヒープの先頭アドレスを取得
	auto material_handle = materialDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	// CBVとSRVとSRVとSRVとSRVで1マテリアルを描画するのでインクリメントサイズを５倍にする
	auto cbvsize = DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

	for (auto& m : material)
	{
		DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(2, material_handle);
		DX12Renderer::GetGraphicsCommandList()->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

		// ヒープポインタとインデックスを次に進める
		material_handle.ptr += cbvsize;
		idxOffset += m.indicesNum;
	}
}

void Object3D::UnInit()
{
	VertexBuffer.ReleaseAndGetAddressOf();
	IndexBuffer.ReleaseAndGetAddressOf();
	TextureBuffer.ReleaseAndGetAddressOf();
	MaterialConstBuffer.ReleaseAndGetAddressOf();
	materialDescHeap.ReleaseAndGetAddressOf();
	transformDescHeap.ReleaseAndGetAddressOf();
}

void Object3D::PlayAnimation()
{
	m_StartTime = timeGetTime();
}
