#include "framework.h"
#include "MainManager.h"

// レンダーモジュール
#include "DX12Renderer.h"

// サウンドモジュール
#include "Audio.h"

// オブジェクト
#include "Polygon2D.h"

// インスタンス
Polygon2D* g_Polygon2D;
Audio* g_bgm;


void MainManager::Init()
{
    // オーディオ初期化
    Audio::InitMaster();

    // レンダー初期化
    DX12Renderer::Init();

    g_Polygon2D = new Polygon2D();
    g_bgm = new Audio();

    g_Polygon2D->Init();
    g_bgm->Load("Assets/Audio/アンノウン・マザーグース.wav");
    g_bgm->Play();
}

void MainManager::Uninit()
{
    g_Polygon2D->Uninit();
    g_bgm->Uninit();

    // レンダー終了
    DX12Renderer::Uninit();

    // オーディオ終了
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

    // ここにオブジェクトの描画
    g_Polygon2D->Draw();

    DX12Renderer::End();

}
