#include "framework.h"
#include "MainManager.h"

// レンダーモジュール
#include "DX12Renderer.h"

// サウンドモジュール
#include "Audio.h"

// オブジェクト
#include "Polygon2D.h"
#include "Obj_HatsuneMiku.h"

// インスタンス
Obj_HatsuneMiku* g_HatsuneMiku;
Audio* g_bgm;


void MainManager::Init()
{
    // オーディオ初期化
    Audio::InitMaster();

    // レンダー初期化
    DX12Renderer::Init();

    g_HatsuneMiku = new Obj_HatsuneMiku();
    g_bgm = new Audio();

    //g_Polygon2D->Init();
    g_HatsuneMiku->Init();
    
    g_bgm->Load("Assets/Audio/アンノウン・マザーグース.wav");
    //g_bgm->Play();
}

void MainManager::Uninit()
{
    g_HatsuneMiku->Uninit();
    
    g_bgm->Uninit();

    // レンダー終了
    DX12Renderer::Uninit();

    // オーディオ終了
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
    XMStoreFloat4x4(&light.ProjMatrix, XMMatrixPerspectiveFovLH(XM_PIDIV4,//画角は90°
        static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//アス比
        0.1f,//近い方
        1000.0f//遠い方
    ));

    DX12Renderer::SetLight(light);

    // シャドウバッファの作成
    //DX12Renderer::BeginDepthShadow();
    // ライトのカメラ行列をセット
    //DX12Renderer::SetView(&light.ViewMatrix);
    //DX12Renderer::SetProj(&light.ProjMatrix);

    // ここにオブジェクトの描画
    DX12Renderer::Begin();

    DX12Renderer::DrawPera();
    //g_HatsuneMiku->Draw();

    DX12Renderer::End();

}
