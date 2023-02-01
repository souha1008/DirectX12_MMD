#include "framework.h"
#include "MainManager.h"

// �����_�[���W���[��
#include "DX12Renderer.h"

// �T�E���h���W���[��
#include "Audio.h"

// �C���v�b�g���W���[��
#include "input.h"

// �I�u�W�F�N�g
#include "Polygon2D.h"
#include "Obj_HatsuneMiku.h"


// �C���X�^���X
Obj_HatsuneMiku* g_HatsuneMiku;
Audio* g_bgm;
PeraPolygon* g_Pera;

void MainManager::Init()
{
    // �I�[�f�B�I������
    Audio::InitMaster();
    // �����_�[������
    DX12Renderer::Init();


    g_HatsuneMiku = new Obj_HatsuneMiku();
    g_bgm = new Audio();
    g_Pera = new PeraPolygon();

    g_HatsuneMiku->Init();
    g_Pera->CreateBokehParamResource();
    g_Pera->CreatePeraResorce();
    g_Pera->CreatePeraVertex();
    g_Pera->CreatePeraPipeline();

    g_bgm->Load("Assets/Audio/�A���m�E���E�}�U�[�O�[�X.wav");

    // �P�t���[�������i�܂���
    g_HatsuneMiku->Update();
}

void MainManager::Uninit()
{
    g_HatsuneMiku->Uninit();
    
    g_bgm->Uninit();

    // �����_�[�I��
    DX12Renderer::Uninit();

    // �I�[�f�B�I�I��
    Audio::UninitMaster();
}

void MainManager::Update()
{
#ifdef _DEBUG
    static bool isplay;
    
    if (Input::GetKeyTrigger(VK_SPACE))
    {
        isplay = !isplay;
    }
    
    if (isplay)
    {
        g_HatsuneMiku->Update();

    }
#else
    g_HatsuneMiku->Update();
#endif // _DEBUG


    g_Pera->Update();
}

void MainManager::Draw()
{

    DX12Renderer::Draw3D();
    // �y���|���S���`�揀��
    g_Pera->PrePeraDraw();

    DX12Renderer::LIGHT light;
    light.Enable = true;
    light.Direction = XMVECTOR{ 1.0f, -1.0f, 1.0f, 0.0f };
    XMVector4Normalize(light.Direction);
    light.Ambient = XMFLOAT4{ 0.1f, 0.1f, 0.1f, 1.0f };
    light.Diffuse = XMFLOAT4{ 1.0f, -1.0f, 1.0f, 1.0f };
    XMFLOAT3 eye = XMFLOAT3{ -10.0f, 10.0f, -10.0f };
    XMFLOAT3 at = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
    XMFLOAT3 up = XMFLOAT3{ 0.0f, 1.0f, 0.0f };
    XMStoreFloat4x4(&light.ViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&at), XMLoadFloat3(&up)));
    XMStoreFloat4x4(&light.ProjMatrix, XMMatrixPerspectiveFovLH(XM_PIDIV4,//��p��90��
        static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//�A�X��
        0.1f,//�߂���
        1000.0f//������
    ));

    DX12Renderer::SetLight(light);

    // 3D�`��
    g_HatsuneMiku->Draw();
    g_Pera->PostPeraDraw();

    // �V���h�E�o�b�t�@�̍쐬
    //DX12Renderer::BeginDepthShadow();
    // ���C�g�̃J�����s����Z�b�g
    //DX12Renderer::SetView(&light.ViewMatrix);
    //DX12Renderer::SetProj(&light.ProjMatrix);

    // �����ɃI�u�W�F�N�g�̕`��
    DX12Renderer::Begin();

    g_Pera->PeraDraw1();

    DX12Renderer::End();

}
