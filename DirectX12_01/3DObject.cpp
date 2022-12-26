#include "framework.h"
#include "DX12Renderer.h"
#include "3DObject.h"

using namespace DirectX;

// �e�N�X�`�����
//std::vector<TEXRGBA> g_Texture(256 * 256);

namespace
{
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, const XMFLOAT3& up, const XMFLOAT3& right)
	{
		// ���������������iZ���j
		XMVECTOR vz = lookat;

		// �i�������������������������Ƃ��́j��y���x�N�g��
		XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

		//�i�������������������������Ƃ��́jy��
		XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		// LookAt��up�����������������Ă�����right�������ɂ��č�蒼��
		if (std::abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
		{
			// ����x�������`
			vx = XMVector3Normalize(XMLoadFloat3(&right));

			// �������������������������Ƃ���Y�����v�Z
			vy = XMVector3Normalize(XMVector3Cross(vz, vx));

			// �^��x�����v�Z
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
		Rotation,		// ��]
		RotAndMove,		// ��]���ړ�
		IK,				// IK
		Undefined,		// ����`
		IKChild,		// IK�e���{�[��
		RotationChild,	// ��]�e���{�[��
		IKDestination,	// IK�ڑ���
		Invisible		// �����Ȃ��{�[��
	};
}


HRESULT Object3D::CreateModel(const char* Filename, const char* Motionname)
{
	//HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// �����_��������
	CreateLambdaTable();

	std::string ModelPath = Filename;
	auto fp = fopen(ModelPath.c_str(), "rb");

	// PMD�w�b�_�[�ǂݍ���
	LoadPMDHeader(fp);

	// ���_�o�b�t�@�[&�r���[�쐬
	HRESULT hr = CreateVertexBuffer(fp);

	// �C���f�b�N�X�o�b�t�@�[&�r���[����
	hr = CreateIndexBuffer(fp);

	// �}�e���A���ǂݍ���
	hr = LoadMaterial(fp, ModelPath);

	// �{�[���̓ǂݍ���
	hr = LoadPMDBone(fp);

	// IK�f�[�^�ǂݍ���
	hr = LoadIK(fp);

	// ���[�V�����f�[�^�ǂݍ���
	hr = LoadVMDData(fopen(Motionname, "rb"));

	// ���[���h�p�萔�o�b�t�@�쐬
	hr = CreateTransformCBuffer();

	// �}�e���A���p�萔�o�b�t�@�쐬
	hr = CreateMaterialView();
	
	// �t�@�C���N���[�Y
	fclose(fp);
	return hr;

}

HRESULT Object3D::LoadPMDHeader(FILE* file)
{
	typedef struct
	{
		float version;          // ��F00 00 80 3F == 1.00
		char model_name[20];    // ���f����
		char comment[256];      // ���f���R�����g
	}PMDHeader;

	// pmd�t�@�C���ǂݍ���
	PMDHeader pmdheader;

	char signature[3] = {};	// �V�O�l�`��
	fread(signature, sizeof(signature), 1, file);
	fread(&pmdheader, sizeof(pmdheader), 1, file);
	return S_OK;
}

HRESULT Object3D::CreateVertexBuffer(FILE* file)
{
	HRESULT hr;

	// ���_�ǂݍ���
	unsigned int vertNum = 0;	// ���_��
	fread(&vertNum, sizeof(vertNum), 1, file);

#pragma pack(push, 1)
	typedef struct
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
		uint16_t boneNo[2];   // �{�[���ԍ�
		uint8_t boneWeight;   // �{�[���e���x
		uint8_t edgeFlag;     // �֊s���t���O 
		uint16_t dummy;
	}PMDVertex;
#pragma pack(pop)

	constexpr size_t pmdVertex_size = 38;	// 1���_������̃T�C�Y
	std::vector<PMDVertex> vertices(vertNum);	// �o�b�t�@�[�m��
	for (unsigned int i = 0; i < vertNum; i++)
	{
		fread(&vertices[i], pmdVertex_size, 1, file);
	}
	sub.vertNum = vertNum;

	CD3DX12_HEAP_PROPERTIES cd_hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	// d3d12x.h
	CD3DX12_RESOURCE_DESC cd_buffer = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMDVertex));		// d3d12x.h

	// ���_�o�b�t�@�[����
	hr = DX12Renderer::GetDevice()->CreateCommittedResource(
		&cd_hp,	// UPLOAD�q�[�v
		D3D12_HEAP_FLAG_NONE,
		&cd_buffer,		// �T�C�Y�ɉ����ēK�؂Ȑݒ������
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(VertexBuffer.ReleaseAndGetAddressOf())
	);

	// ���_�o�b�t�@�[�}�b�v
	PMDVertex* vertMap = nullptr;
	hr = VertexBuffer.Get()->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	VertexBuffer.Get()->Unmap(0, nullptr);

	vbView.BufferLocation = VertexBuffer.Get()->GetGPUVirtualAddress(); // �o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMDVertex));	// �S�o�C�g��
	vbView.StrideInBytes = sizeof(PMDVertex);	// 1���_������̃o�C�g��

	if (vbView.SizeInBytes <= 0 || vbView.StrideInBytes <= 0)
	{
		return ERROR;
	}

	return hr;
}

HRESULT Object3D::CreateIndexBuffer(FILE* file)
{
	HRESULT hr;

	// �C���f�b�N�X�o�b�t�@�[�ǂݍ���
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
	// �C���f�b�N�X�o�b�t�@�[�}�b�v
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

	// �}�b�v
	hr = TransfromConstBuffer.Get()->Map(0, nullptr, (void**)&mappedMatrices);
	//XMStoreFloat4x4(&Model->mappedMatrices[0].world, WorldMatrix);
	mappedMatrices[0] = WorldMatrix;

	RecursiveMatrixMultiply(&m_BoneNodeTable["�Z���^�["], XMMatrixIdentity());

	// �}�g���N�X�̃R�s�[
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
		// �e�[�u�����ɂ������烍�[�h����̂ł͂Ȃ��}�b�v���̃��\�[�X��Ԃ�
		return m_resourceTable[texPath];
	}


	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	auto wtexpath = GetWideStringFromString(texPath); // �e�N�X�`���t�@�C���̃p�X
	auto ext = GetExtension(texPath);	// �g���q���擾

	HRESULT hr = m_loadLambdaTable[ext](wtexpath, &metadata, scratchImg);

	if (FAILED(hr))
	{
		return nullptr;
	}

	const Image* img = scratchImg.GetImage(0, 0, 0);	// ���f�[�^���o

	// WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	CD3DX12_HEAP_PROPERTIES texhp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	

	// ���\�[�X�ݒ�
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
#pragma pack(push, 1) // ��������1�o�C�g�p�b�L���O�ƂȂ�A�A���C�����g�͔������Ȃ�
	typedef struct
	{
		XMFLOAT3 diffuse;   // �f�B�t���[�Y�F
		float alpha;        // �f�B�t���[�Y
		float specularity;   // �X�y�L�����̋���
		XMFLOAT3 specular;  // �X�y�L�����F
		XMFLOAT3 ambient;   // �A���r�G���g�F
		unsigned char toonIdx;  // �g�D�[���ԍ�
		unsigned char edgeFlg;  // �}�e���A�����Ƃ̗֊s���t���O

		// 2�o�C�g�̃p�f�B���O������
		unsigned int indicesNum;
		char texFilePath[20];   // �e�N�X�`���t�@�C���p�X + ��
	}PMDMaterial;   // �p�f�B���O���������Ȃ�����70�o�C�g
#pragma pack(pop)  // �p�b�L���O�w�������

	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, file);
	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, file);

	material.resize(materialNum);
	TextureResource.resize(materialNum); // �}�e���A���̐����e�N�X�`���̃��\�[�X���m��
	sphResource.resize(materialNum);	// ���l
	spaResource.resize(materialNum);
	toonResource.resize(materialNum);

	// �R�s�[
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
		// �g�D�[�����\�[�X�ǂݍ���
		std::string toonFilePath = "Assets/Texture/Toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, 16,"toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		toonFilePath += toonFileName;
		toonResource[i] = LoadTextureFromFile(toonFilePath);

		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			TextureResource[i] = nullptr;
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

	// �}�e���A���o�b�t�@�[�쐬
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
		*((MaterialForHlsl*)mapMaterial) = m.material;	// �f�[�^�R�s�[
		mapMaterial += materialBufferSize;	// ���̃A���C�����g�ʒu�܂Ői�߂�
	}
	MaterialConstBuffer.Get()->Unmap(0, nullptr);

	return hr;
}

HRESULT Object3D::CreateMaterialView()
{
	// �}�e���A���p�f�B�X�N���v�^�[�쐬
	D3D12_DESCRIPTOR_HEAP_DESC mat_dhd = {};
	mat_dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	mat_dhd.NodeMask = 0;
	mat_dhd.NumDescriptors = sub.materialNum * 5;	// �}�e���A�������i�X�t�B�A�}�e���A���ǉ��j+ �e�N�X�`��
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

	// �r���[�쐬
	auto materialBufsize = sizeof(MaterialForHlsl);
	materialBufsize = (materialBufsize + 0xff) & ~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbvd = {};
	mat_cbvd.BufferLocation = MaterialConstBuffer.Get()->GetGPUVirtualAddress();
	mat_cbvd.SizeInBytes = materialBufsize;

	// �擪���L�^
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

		// �g���q��sph�������Ƃ��AsphResource�Ƀ|�C���^������
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

		// �g���q��spa�������Ƃ��AsphResource�Ƀ|�C���^������
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
	std::vector<XMVECTOR> positions; // IK�\���_��ۑ�
	std::array<float, 2> edgeLens;	// IKEA�̂��ꂼ��̃{�[���Ԃ̋�����ۑ�

	// �^�[�Q�b�g�i���[�{�[���ł͂Ȃ��A���[�{�[�����߂Â��ڕW�{�[���̍��W���擾�j
	auto& targetNode = m_boneNodeAddressArray[ik.boneIdx];
	auto targetPos = XMVector3Transform(XMLoadFloat3(&targetNode->startPos), BoneMatrix[ik.boneIdx]);

	// IK�`�F�[�����t���Ȃ̂ŁA�t�ɕ��Ԃ悤�ɂ��Ă���
	// ���[�{�[��
	BoneNode* endNode = m_boneNodeAddressArray[ik.targetIdx];
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));

	// ���ԋy�у��[�g�{�[��
	for (auto& chainBoneIndex : ik.nodeIdx)
	{
		BoneNode* boneNode = m_boneNodeAddressArray[chainBoneIndex];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	// �킩��Â炢�̂ŋt�ɂ���
	std::reverse(positions.begin(), positions.end());

	// ���Ƃ̒����𑪂��Ă���
	edgeLens[0] = XMVector3Length(XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edgeLens[1] = XMVector3Length(XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	// ���[�g�{�[�����W�ϊ��i�t���ɂȂ��Ă��邽�ߎg�p����C���f�b�N�X�ɒ��Ӂj
	positions[0] = XMVector3Transform(positions[0], BoneMatrix[ik.nodeIdx[1]]);

	// �^�񒆂͎����v�Z

	// ��[�{�[��
	positions[2] = XMVector3Transform(positions[2], BoneMatrix[ik.boneIdx]);

	// ���[�g�����[�ւ̃x�N�g��������Ă���
	XMVECTOR linearVec = XMVectorSubtract(positions[2], positions[0]);

	float A = XMVector3Length(linearVec).m128_f32[0];
	float B = edgeLens[0];
	float C = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);

	// ���[�g����^�񒆂ւ̊p�x�v�Z
	float theta1 = acosf((A * A + B * B - C * C) / (2 * A * B));

	// �^�񒆂���^�[�Q�b�g�ւ̊p�x�v�Z
	float theta2 = acosf((B * B + C * C - A * A) / (2 * B * C));

	// �������߂�
	// �����^�񒆂��u�Ђ��v�ō������ꍇ�ɂ͋����I��X���Ƃ���

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

	// ���ӓ_�FIK�`�F�[���̓��[�g�Ɍ������Ă��琔�����邽�߂P�����[�g�ɋ߂�
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
	// ���̊֐��ɗ������_�Ńm�[�h�͂P�����Ȃ��A
	// �`�F�[���ɓ����Ă���m�[�h�ԍ���IKEA�̃��[�g�m�[�h�̂��̂Ȃ̂�
	// ���̃��[�g�m�[�h���疖�[�Ɍ������x�N�g�����l����

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

ID3D12Resource* Object3D::CreateDefaultTexture(size_t width, size_t height)
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	ID3D12Resource* buff = nullptr;
	auto result = DX12Renderer::GetDevice()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
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

ID3D12Resource* Object3D::CreateGrayGradationTexture()
{
	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	// �オ�����ĉ��������e�N�X�`���쐬
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
		return x;	// �v�Z�s�v
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;	// t^3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;		// t^2�̌W��
	const float k2 = 3 * a.x;				// t�̌W��

	// �덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.0005f;

	// t���ւ��ŋ��߂�
	for (int i = 0; i < n; i++)
	{
		// f(t)�����߂�
		float ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		// �������ʂ��O�ɋ߂��Ȃ�ł��؂�
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
	auto elapsedTime = timeGetTime() - m_StartTime;	// �o�ߎ���
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);	// �o�߃t���[�����v�Z

	// ���[�v���邽��
	if (frameNo > m_duration)
	{
		m_StartTime = timeGetTime();
		frameNo = 0;
	}

	// �s����N���A�i�N���A���Ă��Ȃ��ƑO�t���[���̃|�[�Y���d�˂�������ă��f��������j
	std::fill(BoneMatrix.begin(), BoneMatrix.end(), XMMatrixIdentity());

	// ���[�V�����f�[�^�X�V
	for (auto& bonemotion : MotionData)
	{
		auto node = m_BoneNodeTable[bonemotion.first];
		
		// ���v������̂�T��
		auto motions = bonemotion.second;
		// ���o�[�X�C�e���[�^�[�ŋt�����擾
		auto rit = std::find_if(
			motions.rbegin(), motions.rend(),
			[frameNo](const KeyFrame& motion) {return motion.frameNo <= frameNo; });

		// ���n������̂��Ȃ���Ώ������΂�
		if (rit == motions.rend())
		{
			continue;
		}

		// t = ���݃t���[�� - ���O�L�[�t���[�� / ����L�[�t���[�� - ���O�L�[�t���[��

		XMMATRIX rotation;
		auto it = rit.base();

		if (it != motions.end())
		{
			auto t = static_cast<float>(frameNo - rit->frameNo) / static_cast<float>(it->frameNo - rit->frameNo);
			// ���`���
			//rotation = XMMatrixRotationQuaternion(rit->quaternion)
			//	* (1 - t)
			//	+ XMMatrixRotationQuaternion(it->quaternion)
			//	* t;

			t = GetYFromXonBezier(t, it->p1, it->p2, 12);
			// ���ʐ��`���
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
	RecursiveMatrixMultiply(&m_BoneNodeTable["�Z���^�["], XMMatrixIdentity());

	//IKSolve(Model, frameNo);

	// �}�g���N�X�̃R�s�[
	std::copy(BoneMatrix.begin(), BoneMatrix.end(), mappedMatrices + 1);

}

HRESULT Object3D::LoadPMDBone(FILE* file)
{
#pragma pack(push, 1)
	typedef struct
	{
		char boneName[20];          // �{�[����
		unsigned short parentNo;    // �e�{�[���ԍ�
		unsigned short nextNo;      // ��[�̃{�[���ԍ�
		unsigned char type;         // �{�[�����
		unsigned short  ikBoneNo;   // IK�{�[���ԍ�
		XMFLOAT3 pos;               // �{�[���̊�_���W
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
	// �{�[���m�[�h�}�b�v���쐬
	for (int index = 0; index < pmdBone.size(); ++index)
	{
		auto& pmdbones = pmdBone[index];
		boneNames[index] = pmdbones.boneName;
		auto& node = m_BoneNodeTable[pmdbones.boneName];
		node.boneIdx = index;
		node.startPos = pmdbones.pos;

		// �C���f�b�N�X�������₷���悤��
		m_BoneNameArray[index] = pmdbones.boneName;
		m_boneNodeAddressArray[index] = &node;
		std::string boneName = pmdbones.boneName;

		if (boneName.find("�Ђ�") != std::string::npos)
		{
			m_kneeIdxes.emplace_back(index);
		}

	}

	// �e�q�֌W���\�z
	for (auto& pmdbones : pmdBone)
	{
		// �e�C���f�b�N�X���`�F�b�N
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
		char boneName[15];      // �{�[����
		unsigned int frameNo;   // �t���[���ԍ�
		XMFLOAT3 location;      // �ʒu
		XMFLOAT4 quaternion;    // �N�H�[�^�j�I���i��]�j
		unsigned char bezier[64]; // �x�W�F�ۊǃp�����[�^
	}VMDMotion;

	fseek(file, 50, SEEK_SET);

	unsigned int motionDataNum = 0;
	fread(&motionDataNum, sizeof(motionDataNum), 1, file);

	std::vector<VMDMotion> vmdMotionData(motionDataNum);

	// ���[�V�����f�[�^�ǂݍ���
	for (auto& motion : vmdMotionData)
	{
		fread(motion.boneName, sizeof(motion.boneName), 1, file);	// �{�[�����擾
		fread(&motion.frameNo,
			sizeof(motion.frameNo)			//�t���[���ԍ�
			+ sizeof(motion.location)		// 
			+ sizeof(motion.quaternion)
			+ sizeof(motion.bezier),
			1, file);
		m_duration = std::max<unsigned int>(m_duration, motion.frameNo);
	}

	// �\��f�[�^�ǂݍ���
#pragma pack(1)
	struct VMDMorph
	{
		char name[15];		// ���O
		uint32_t frameNo;	// �t���[���ԍ�
		float weight;		// �E�F�C�g
	}; // �S����23�o�C�g
#pragma pack()

	uint32_t vmdMorphCount = 0;
	fread(&vmdMorphCount, sizeof(vmdMorphCount), 1, file);
	std::vector<VMDMorph> vmdMorphsData(vmdMorphCount);
	fread(vmdMorphsData.data(), sizeof(VMDMorph), vmdMorphCount, file);

	// �J�����ǂݍ���
#pragma pack(1)
	struct VMDCamera
	{
		uint32_t frameNo;	// �t���[���ԍ�
		float distance;	// ����
		XMFLOAT3 pos;	// ���W
		XMFLOAT3 eulerAngle; // �I�C���[�p
		uint8_t InterPolation[24]; // ���
		uint32_t fov;	// ����p
		uint8_t persFlag;	// �p�[�X�t���O
	};	// 61�o�C�g
#pragma pack()

	uint32_t vmdCameraCount = 0;
	fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, file);
	std::vector<VMDCamera> vmdCameraData(vmdCameraCount);
	fread(vmdCameraData.data(), sizeof(VMDCamera), vmdCameraCount, file);

	// ���C�g�ǂݍ���
	struct VMDLight
	{
		uint32_t frameNo;	// �t���[���ԍ�
		XMFLOAT3 rgb;	// �F
		XMFLOAT3 vec;	// �����x�N�g��
	};

	uint32_t vmdLightCount = 0;
	fread(&vmdLightCount, sizeof(vmdLightCount), 1, file);
	std::vector<VMDLight> vmdLightData(vmdLightCount);
	fread(vmdLightData.data(), sizeof(VMDLight), vmdLightCount, file);

	// �Z���t�e�f�[�^
#pragma pack(1)
	struct VMDSelfShadow
	{
		uint32_t frameNo;	// �t���[���ԍ�
		uint8_t mode;	// �e���[�h
		float distance;	// ����
	};
#pragma pack()
	
	uint32_t vmdSelfShadowCount = 0;
	fread(&vmdSelfShadowCount, sizeof(vmdSelfShadowCount), 1, file);
	std::vector<VMDSelfShadow> vmdSelfShadowData(vmdSelfShadowCount);
	fread(vmdSelfShadowData.data(), sizeof(VMDSelfShadow), vmdSelfShadowCount, file);

	// IK�I�m�t�؂�ւ�萔
	uint32_t ikSwitchCount = 0;
	fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, file);

	m_IKEnableData.resize(ikSwitchCount);
	for (auto& IKEnable : m_IKEnableData)
	{
		// �L�[�t���[�����Ȃ̂ł܂��̓t���[���ԍ��ǂݍ���
		fread(&IKEnable, sizeof(IKEnable.frameNo), 1, file);

		// ���ɉ��t���O�����邪�g�p���Ȃ�
		uint8_t visibleFlag = 0;
		fread(&visibleFlag, sizeof(visibleFlag), 1, file);

		// �Ώۃ{�[�����ǂݍ���
		uint32_t ikBoneCount = 0;
		fread(&ikBoneCount, sizeof(ikBoneCount), 1, file);

		// ���[�v�����O��ONOFF���擾
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

	// �N�H�[�^�j�I���K��
	for (auto& bonemotion : MotionData)
	{
		// �A�j���[�V�����f�[�^����{�[��������
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

		uint8_t chainLen = 0;	// �Ԃɂ����m�[�h�����邩
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

	// �f�o�b�O�E�B���h�E��IK�o��
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
		oss << "IK�{�[���ԍ�=" << ik.boneIdx << "�F" << getNameFromIdx(ik.boneIdx) << std::endl;

		for (auto& node : ik.nodeIdx)
		{
			oss << "\t�m�[�h�{�[��=" << node << "�F" << getNameFromIdx(node) << std::endl;
		}

		OutputDebugString(oss.str().c_str());
	}

	return S_OK;
}

void Object3D::IKSolve(int frameNo)
{
	// �����̋t���猟��
	auto it = std::find_if(m_IKEnableData.rbegin(),
		m_IKEnableData.rend(),
		[frameNo](const VMDIKEnable& ikenable)
		{
			return ikenable.frameNo <= frameNo;
		});

	// IK�����̂��߃��[�v
	for (auto& ik : pmdIKData)
	{
		if (it != m_IKEnableData.rend())
		{
			auto ikEnableit = it->ikEnableTable.find(m_BoneNameArray[ik.boneIdx]);

			if (ikEnableit != it->ikEnableTable.end())
			{
				if (!ikEnableit->second)	// ����OFF�Ȃ�ł��؂�
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

		case 1:	// �Ԃ̃{�[���̐����P�̂Ƃ���LookAt���g�p
			SolveLookAtIK(ik);
			break;

		case 2: // �Ԃ̃{�[���̐����Q�̂Ƃ��͗]���藝IK
			SolveCosineIK(ik);
			break;
		default: // �Ԃ̃{�[�������R�ȏ�̂Ƃ���CCD-IK
			SolveCCDIK(ik);
			break;
		}
	}
}

void Object3D::SolveCCDIK(const PMDIK& ik)
{
	// �덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.00005f;

	BoneNode* targetBoneNode = m_boneNodeAddressArray[ik.boneIdx];
	XMVECTOR targetOriginPos = XMLoadFloat3(&targetBoneNode->startPos);

	// �t�s��Ŗ���������
	XMMATRIX parentMat = BoneMatrix[m_boneNodeAddressArray[ik.boneIdx]->ikParentBone];
	XMVECTOR det;
	XMMATRIX invParentMat = XMMatrixInverse(&det, parentMat);
	XMVECTOR targetNextPos = XMVector3Transform(targetOriginPos, BoneMatrix[ik.boneIdx] * invParentMat);

	std::vector<XMVECTOR> bonePositions;

	// �{�[�����W�𓮂����{�[���̉�]�p�x�����߂邽�ߌ����W��ۑ�
	// ���[�m�[�h
	XMVECTOR endPos = XMLoadFloat3(&m_boneNodeAddressArray[ik.targetIdx]->startPos);

	// ���ԃm�[�h�i���[�g���j
	for (auto& cIndex : ik.nodeIdx)
	{
		bonePositions.push_back(XMLoadFloat3(&m_boneNodeAddressArray[cIndex]->startPos));
	}

	// ���[�ȊO�̃{�[���������s����m��
	std::vector<XMMATRIX> mats(bonePositions.size());
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());
	
	// PMD�f�[�^���L�ŁA�x���@��180���Ŋ��������l���������܂�Ă���̂ŁA���W�A���Ƃ��Ďg�p���邽�߂�XM_PI����Z����
	float ikLimit = ik.limit * XM_PI;

	for (int c = 0; c < ik.iterations; c++)
	{
		// �^�[�Q�b�g�Ɩ��[���قڈ�v�����甲����
		if (XMVector3Length(XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
		{
			break;
		}

		// �{�[����k��Ȃ���p�x�����Ɉ���������Ȃ��悤�ɋȂ��Ă���

		// bonePositions��CCD-IK�ɂ�����e�m�[�h�̍��W��vector�z��ɂ�������

		for (int bIndex = 0; bIndex < bonePositions.size(); bIndex++)
		{
			const XMVECTOR& pos = bonePositions[bIndex];

			// �Ώۃm�[�h���疖�[�m�[�h�܂łƑΏۃm�[�h����^�[�Q�b�g�܂ł̃x�N�g���쐬
			XMVECTOR vecToEnd = XMVectorSubtract(endPos, pos);	// ���[��
			XMVECTOR vecToTarget = XMVectorSubtract(targetNextPos, pos); // �^�[�Q�b�g��
			// �������K��
			vecToEnd = XMVector3Normalize(vecToEnd);
			vecToTarget = XMVector3Normalize(vecToTarget);

			// �قړ����x�N�g���ɂȂ��Ă��܂����ꍇ��
			// �O�ςł��Ȃ����ߎ��̃{�[���Ɉ����n��
			if (XMVector3Length(XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0] <= epsilon)
			{
				continue;
			}

			// �O�όv�Z
			auto cross = XMVector3Normalize(XMVector3Cross(vecToEnd, vecToTarget)); // ���ɂȂ�
			// �֗��Ȋ֐��������g��cos�Ȃ̂�0~90��0~-90�̋�ʂ��Ȃ�
			float angle = XMVector3AngleBetweenVectors(vecToEnd, vecToTarget).m128_f32[0];
			
			// ��]���E�𒴂��Ă��܂����Ƃ��͌��E�l�ɕ␳
			angle = min(angle, ikLimit);
			XMMATRIX rot = XMMatrixRotationAxis(cross, angle); // ��]�s��쐬

			// ���_���S�ł͂Ȃ�pos���S�ɉ�]
			XMMATRIX mat = XMMatrixTranslationFromVector(-pos) * rot * XMMatrixTranslationFromVector(pos);

			// ��]�s���ێ��i��Z�ŉ�]�d�˂���������Ă����j
			mats[bIndex] *= mat;

			// �ΏۂƂȂ�_��S�ĉ�]������B�Ȃ��A��������]������K�v�Ȃ�
			for (int index = bIndex - 1; index >= 0; index--)
			{
				bonePositions[index] = XMVector3Transform(bonePositions[index], mat);
			}

			endPos = XMVector3Transform(endPos, mat);

			// ���������ɋ߂��Ȃ��Ă����烋�[�v�𔲂���
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
	// WIC�n
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
	// �v���~�e�B�u�g�|���W�ݒ�
	DX12Renderer::GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ���_�o�b�t�@�[�r���[�Z�b�g
	DX12Renderer::GetGraphicsCommandList()->IASetVertexBuffers(0, 1, &vbView);

	// �C���f�b�N�X�o�b�t�@�[�r���[�Z�b�g
	DX12Renderer::GetGraphicsCommandList()->IASetIndexBuffer(&ibView);

	ID3D12DescriptorHeap* trans_dh[] = { transformDescHeap.Get() };
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, trans_dh);
	auto transform_handle = transformDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(1, transform_handle);

	// �}�e���A���̃f�B�X�N���v�^�q�[�v�Z�b�g
	ID3D12DescriptorHeap* mdh[] = { materialDescHeap.Get() };
	DX12Renderer::GetGraphicsCommandList()->SetDescriptorHeaps(1, mdh);

	// �}�e���A���p�f�B�X�N���v�^�q�[�v�̐擪�A�h���X���擾
	auto material_handle = materialDescHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	// CBV��SRV��SRV��SRV��SRV��1�}�e���A����`�悷��̂ŃC���N�������g�T�C�Y���T�{�ɂ���
	auto cbvsize = DX12Renderer::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

	for (auto& m : material)
	{
		DX12Renderer::GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(2, material_handle);
		DX12Renderer::GetGraphicsCommandList()->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

		// �q�[�v�|�C���^�ƃC���f�b�N�X�����ɐi�߂�
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
