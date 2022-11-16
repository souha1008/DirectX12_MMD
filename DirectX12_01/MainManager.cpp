#include "framework.h"
#include "MainManager.h"

// �����_�[���W���[��
#include "DX12Renderer.h"

// �I�u�W�F�N�g
#include "Polygon2D.h"

// �C���X�^���X
Polygon2D* g_Polygon2D;

void MainManager::Init()
{
    // �����_�[������
    DX12Renderer::Init();

    g_Polygon2D = new Polygon2D();

    g_Polygon2D->Init();
}

void MainManager::Uninit()
{
    g_Polygon2D->Uninit();

    // �����_�[�I��
    DX12Renderer::Uninit();
}

void MainManager::Update()
{
    g_Polygon2D->Update();
}

void MainManager::Draw()
{
    DX12Renderer::Begin();

    // �����ɃI�u�W�F�N�g�̕`��
    g_Polygon2D->Draw();

    DX12Renderer::End();

}
