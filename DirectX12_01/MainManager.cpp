#include "framework.h"
#include "MainManager.h"

// �����_�[���W���[��
#include "DX12Renderer.h"

// �T�E���h���W���[��
#include "Audio.h"

// �I�u�W�F�N�g
#include "Polygon2D.h"
#include "Obj_HatsuneMiku.h"

// �C���X�^���X
Obj_HatsuneMiku* g_HatsuneMiku;
Audio* g_bgm;


void MainManager::Init()
{
    // �I�[�f�B�I������
    Audio::InitMaster();

    // �����_�[������
    DX12Renderer::Init();

    g_HatsuneMiku = new Obj_HatsuneMiku();
    g_bgm = new Audio();

    //g_Polygon2D->Init();
    g_HatsuneMiku->Init();
    
    g_bgm->Load("Assets/Audio/�A���m�E���E�}�U�[�O�[�X.wav");
    //g_bgm->Play();
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
    g_HatsuneMiku->Update();
}

void MainManager::Draw()
{

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

    // �V���h�E�o�b�t�@�̍쐬
    //DX12Renderer::BeginDepthShadow();
    // ���C�g�̃J�����s����Z�b�g
    //DX12Renderer::SetView(&light.ViewMatrix);
    //DX12Renderer::SetProj(&light.ProjMatrix);

    // �����ɃI�u�W�F�N�g�̕`��
    DX12Renderer::Begin();

    DX12Renderer::DrawPera();
    //g_HatsuneMiku->Draw();

    DX12Renderer::End();

}
