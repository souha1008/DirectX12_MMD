#include "framework.h"
#include "DX12Renderer.h"
#include "3DObject.h"

using namespace DirectX;

// �e�N�X�`�����
std::vector<TEXRGBA> g_Texture(256 * 256);

HRESULT Object3D::CreateModel(const char* Filename, MODEL_DX12* Model)
{
	PMDHeader pmdheader;

	char signature[3] = {};	// �V�O�l�`��
	std::string ModelPath = Filename;
	auto fp = fopen(ModelPath.c_str(), "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	// ���_�ǂݍ���
	unsigned int vertNum;	// ���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);

	constexpr size_t pmdVertex_size = 38;	// 1���_������̃T�C�Y
	std::vector<PMDVertex> vertices(vertNum);	// �o�b�t�@�[�m��
	for (int i = 0; i < vertNum; i++)
	{
		fread(&vertices[i], pmdVertex_size, 1, fp);
	}
	Model->sub.vertNum = vertNum;

	// �C���f�b�N�X�o�b�t�@�[�ǂݍ���
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	std::vector<unsigned short> indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	Model->sub.indecesNum = indicesNum;



	// ���_�o�b�t�@�[�쐬
	HRESULT hr = CreateVertexBuffer(Model, vertices);

	// ���_�o�b�t�@�[�r���[�ݒ�
	hr = SettingVertexBufferView(Model, vertices, pmdVertex_size);

	// �}�e���A���ǂݍ���
	hr = LoadMaterial(fp, Model, ModelPath);

	// �C���f�b�N�X�o�b�t�@�[����
	hr = CreateIndexBuffer(Model, indices);

	// �C���f�b�N�X�o�b�t�@�r���[�ݒ�
	hr = SettingIndexBufferView(Model, indices);

	// �e�N�X�`���f�[�^�Z�b�g
	//hr = CreateTextureData(Model);

	// �萔�o�b�t�@�[����
	hr = CreateConstBuffer(Model);

	// �f�B�X�N���v�^�[����
	hr = CreateBasicDescriptorHeap(Model);

	// �V�F�[�_�[���\�[�X�r���[����
	hr = CreateShaderResourceView(Model);

	// �萔�o�b�t�@�[�r���[�ݒ�
	D3D12_CPU_DESCRIPTOR_HANDLE basicHeapHandle = Model->basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	//basicHeapHandle.ptr += DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	// �|�C���^���炵�C��t���悤�I
	hr = SettingConstBufferView(&basicHeapHandle, Model);

	// �t�@�C���N���[�Y
	fclose(fp);
	return hr;

}

HRESULT Object3D::CreateVertexBuffer(MODEL_DX12* Model, std::vector<PMDVertex> vertices)
{
	HRESULT hr;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMDVertex));		// d3d12x.h

	// ���_�o�b�t�@�[����
	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,	// UPLOAD�q�[�v
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,		// �T�C�Y�ɉ����ēK�؂Ȑݒ������
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Model->VertexBuffer)
	);

	// ���_�o�b�t�@�[�}�b�v
	PMDVertex* vertMap = nullptr;
	hr = Model->VertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	Model->VertexBuffer->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::SettingVertexBufferView(MODEL_DX12* Model, std::vector<PMDVertex> vertices, size_t pmdVertex_size)
{
	Model->vbView.BufferLocation = Model->VertexBuffer->GetGPUVirtualAddress(); // �o�b�t�@�̉��z�A�h���X
	Model->vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMDVertex));	// �S�o�C�g��
	Model->vbView.StrideInBytes = sizeof(PMDVertex);	// 1���_������̃o�C�g��

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
	// �C���f�b�N�X�o�b�t�@�[�}�b�v
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

	// ����
	XMFLOAT3 eye(0, 12, -25);
	// �����_
	XMFLOAT3 target(0, 12, 0);
	// ��x�N�g��
	XMFLOAT3 v_up(0, 1, 0);

	XMMATRIX viewMat;
	XMMATRIX projMat;

	viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&v_up));
	projMat = XMMatrixPerspectiveFovLH(XM_PIDIV4,//��p��90��
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//�A�X��
		1.0f,//�߂���
		100.0f//������
	);


	// �X�N���[�����W�ɂ���
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
	// �f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// �V�F�[�_�[���猩����
	dhd.NodeMask = 0;	// �}�X�N0
	dhd.NumDescriptors = 2;	// �r���[�͍��̂Ƃ���P����
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

	// WIC�e�N�X�`���̃��[�h
	hr = LoadFromWICFile(
		L"img/textest.png",
		WIC_FLAGS_NONE,
		&Model->MetaData,
		ScratchImg
	);

	// ���f�[�^���o
	const Image* img = ScratchImg.GetImage(0, 0, 0);

	// WriteToSubresource�œ]�����邽�߂̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES heapprop = {};

	// ����Ȑݒ�Ȃ̂�DEFAULT�ł�UPLOAD�ł��Ȃ�
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;

	//���C�g�o�b�N
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

	// �]����L0�ACPU�����璼�ڍs��
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	// �P��A�_�v�^�[�̂���0
	heapprop.CreationNodeMask = 0;
	heapprop.VisibleNodeMask = 0;

	// ���\�[�X�ݒ�
	D3D12_RESOURCE_DESC rd = {};

	//RBGA�t�H�[�}�b�g
	//rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//rd.Width = 256;		// �e�N�X�`���̕�
	//rd.Height = 256;	// �e�N�X�`���̍���
	//rd.DepthOrArraySize = 1;	// 2D�Ŕz��ł��Ȃ��̂łP
	//rd.SampleDesc.Count = 1;	// �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	//rd.SampleDesc.Quality = 0;	// �N�I���e�B�͍Œ�
	//rd.MipLevels = 1;	// �ǂݍ��񂾃��^�f�[�^�̃~�b�v���x���Q��
	//rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2D�e�N�X�`���p
	//rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// ���C�A�E�g�͌��肵�Ȃ�
	//rd.Flags = D3D12_RESOURCE_FLAG_NONE;	// �t���O�Ȃ�

	// �e�N�X�`���t�H�[�}�b�g
	rd.Format = Model->MetaData.format;
	rd.Width = Model->MetaData.width;		// �e�N�X�`���̕�
	rd.Height = Model->MetaData.height;	// �e�N�X�`���̍���
	rd.DepthOrArraySize = Model->MetaData.arraySize;	// 2D�Ŕz��ł��Ȃ��̂łP
	rd.SampleDesc.Count = 1;	// �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	rd.SampleDesc.Quality = 0;	// �N�I���e�B�͍Œ�
	rd.MipLevels = Model->MetaData.mipLevels;	// �ǂݍ��񂾃��^�f�[�^�̃~�b�v���x���Q��
	rd.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(Model->MetaData.dimension);	// 2D�e�N�X�`���p
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// ���C�A�E�g�͌��肵�Ȃ�
	rd.Flags = D3D12_RESOURCE_FLAG_NONE;	// �t���O�Ȃ�

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
		g_Texture.data(),	// ���f�[�^�A�h���X
		sizeof(TEXRGBA) * 256,	// 1���C���T�C�Y
		sizeof(TEXRGBA) * g_Texture.size()
	);


	// �e�N�X�`��
	//hr = Model->TextureBuffer->WriteToSubresource(
	//	0,
	//	nullptr,
	//	img->pixels,	// ���f�[�^�A�h���X
	//	img->rowPitch,	// 1���C���T�C�Y
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

	const Image* img = scratchImg.GetImage(0, 0, 0);	// ���f�[�^���o

	// WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texhp = {};
	texhp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texhp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texhp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texhp.CreationNodeMask = 0;	// �P��A�_�v�^�̂���0
	texhp.VisibleNodeMask = 0;	// �P��A�_�v�^�̂���0

	// ���\�[�X�ݒ�
	D3D12_RESOURCE_DESC rd = {};
	rd.Format = metadata.format;
	rd.Width = metadata.width;
	rd.Height = metadata.height;
	rd.DepthOrArraySize = metadata.arraySize;
	rd.SampleDesc.Count = 1;	// �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	rd.SampleDesc.Quality = 0;	// �N�I���e�B��0
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
	// �V�F�[�_�[���\�[�X�r���[���쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = Model->MetaData.format;	// RGBA(0~1�ɐ��K��)
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
	Model->TextureResource.resize(materialNum); // �}�e���A���̐����e�N�X�`���̃��\�[�X���m��
	Model->sphResource.resize(materialNum);	// ���l
	Model->spaResource.resize(materialNum);

	// �R�s�[
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

		// ���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
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

	// �}�e���A���o�b�t�@�[�쐬
	HRESULT hr = CreateMaterialBuffer(Model);

	// �}�e���A���o�b�t�@�[�r���[�쐬
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
		*((MaterialForHlsl*)mapMaterial) = m.material;	// �f�[�^�R�s�[
		mapMaterial += materialBufferSize;	// ���̃A���C�����g�ʒu�܂Ői�߂�
	}
	Model->MaterialBuffer->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::CreateMaterialView(MODEL_DX12* Model)
{
	// �}�e���A���p�f�B�X�N���v�^�[�쐬
	D3D12_DESCRIPTOR_HEAP_DESC mat_dhd = {};
	mat_dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	mat_dhd.NodeMask = 0;
	mat_dhd.NumDescriptors = Model->sub.materialNum * 4;	// �}�e���A�������i�X�t�B�A�}�e���A���ǉ��j
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

	// �r���[�쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbvd = {};
	mat_cbvd.BufferLocation = Model->MaterialBuffer->GetGPUVirtualAddress();
	mat_cbvd.SizeInBytes = static_cast<UINT>(Model->sub.materialBufferSize);

	// �擪���L�^
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

		// �g���q��sph�������Ƃ��AsphResource�Ƀ|�C���^������
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

		// �g���q��spa�������Ƃ��AsphResource�Ƀ|�C���^������
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
	// �Ăяo������
	int num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr;	// string��wchar_t��
	wstr.resize(num1);	// ����ꂽ�����񐔂Ń��T�C�Y

	// �Ăяo������
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

	// �f�[�^�]��
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

	// �f�[�^�]��
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


