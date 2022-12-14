#include "framework.h"
#include "MainManager.h"

// �����_�[���W���[��
#include "DX12Renderer.h"

// �T�E���h���W���[��
#include "Audio.h"

// �I�u�W�F�N�g
#include "Polygon2D.h"

// �C���X�^���X
Polygon2D* g_Polygon2D;
Audio* g_bgm;


void MainManager::Init()
{
    // �I�[�f�B�I������
    Audio::InitMaster();

    // �����_�[������
    DX12Renderer::Init();

    g_Polygon2D = new Polygon2D();
    g_bgm = new Audio();

    g_Polygon2D->Init();
    g_bgm->Load("Assets/Audio/�A���m�E���E�}�U�[�O�[�X.wav");
    g_bgm->Play();
}

void MainManager::Uninit()
{
    g_Polygon2D->Uninit();
    g_bgm->Uninit();

    // �����_�[�I��
    DX12Renderer::Uninit();

    // �I�[�f�B�I�I��
    Audio::UninitMaster();
}

void MainManager::Update()
{
    g_Polygon2D->Update();
}

void MainManager::Draw()
{
    DX12Renderer::Begin();

    DX12Renderer::LIGHT light;
    light.Enable = true;
    light.Direction = XMVECTOR{ 1.0f, -1.0f, 1.0f, 0.0f };
    XMVector4Normalize(light.Direction);
    light.Ambient = XMFLOAT4{ 0.1f, 0.1f, 0.1f, 1.0f };
    light.Diffuse = XMFLOAT4{ 1.0f, -1.0f, 1.0f, 1.0f };
    DX12Renderer::SetLight(light);

    // �����ɃI�u�W�F�N�g�̕`��
    g_Polygon2D->Draw();

    DX12Renderer::End();

}
