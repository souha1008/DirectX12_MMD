#include "framework.h"
#include "MainManager.h"

// レンダーモジュール
#include "DX12Renderer.h"

// サウンドモジュール
#include "Audio.h"

// インプットモジュール
#include "input.h"

// シーン
#include "Title.h"

// オブジェクト
#include "Polygon2D.h"
#include "Obj_HatsuneMiku.h"

NowScene MainManager::m_nowscene = NowScene::Title;


// インスタンス
Obj_HatsuneMiku* g_HatsuneMiku;
Audio* g_bgm;
PeraPolygon* g_Pera;

void MainManager::Init()
{
    switch (m_nowscene)
    {
    case Title:
        Title::Init();
        break;
    case Main:
        // オーディオ初期化
        Audio::InitMaster();

        // レンダー初期化
        DX12Renderer::Init();

        g_HatsuneMiku = new Obj_HatsuneMiku();
        g_bgm = new Audio();
        g_Pera = new PeraPolygon();

        g_HatsuneMiku->Init();
        g_Pera->CreatePeraResorce();
        g_Pera->CreatePeraVertex();
        g_Pera->CreatePeraPipeline();

        g_bgm->Load("Assets/Audio/アンノウン・マザーグース.wav");
        //g_bgm->Play();
        break;
    default:
        break;
    }
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
    switch (m_nowscene)
    {
    case Title:
        Title::Update();
        if (Input::GetKeyTrigger(VK_SPACE))
        {
            m_nowscene = NowScene::Main;
            Init();
        }
        break;
    case Main:
        g_HatsuneMiku->Update();
        break;
    default:
        break;
    }


}

void MainManager::Draw()
{
    switch (m_nowscene)
    {
    case Title:
        Title::Draw();
        break;
    case Main:
        DX12Renderer::Draw3D();
        // ペラポリゴン描画準備
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
        XMStoreFloat4x4(&light.ProjMatrix, XMMatrixPerspectiveFovLH(XM_PIDIV4,//画角は90°
            static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),//アス比
            0.1f,//近い方
            1000.0f//遠い方
        ));

        DX12Renderer::SetLight(light);

        // 3D描画
        g_HatsuneMiku->Draw();
        g_Pera->PostPeraDraw();

        // シャドウバッファの作成
        //DX12Renderer::BeginDepthShadow();
        // ライトのカメラ行列をセット
        //DX12Renderer::SetView(&light.ViewMatrix);
        //DX12Renderer::SetProj(&light.ProjMatrix);

        // ここにオブジェクトの描画


        DX12Renderer::Begin();

        //g_HatsuneMiku->Draw();
        g_Pera->PeraDraw1();

        DX12Renderer::End();
        break;
    default:
        break;
    }



}
