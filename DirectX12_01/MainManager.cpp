#include "framework.h"
#include "MainManager.h"

// レンダーモジュール
#include "DX12Renderer.h"
#include "FinalResource.h"

// サウンドモジュール
#include "Audio.h"

// インプットモジュール
#include "input.h"

// オブジェクト
#include "Polygon2D.h"
#include "Obj_HatsuneMiku.h"

// インスタンス
Obj_HatsuneMiku* g_HatsuneMiku;
Audio* g_bgm;
PeraPolygon* g_Pera;
FinalResource* g_FinalResource;

void MainManager::Init()
{
    // オーディオ初期化
    Audio::InitMaster();
    // レンダー初期化
    DX12Renderer::Init();


    g_HatsuneMiku = new Obj_HatsuneMiku();
    g_bgm = new Audio();
    g_Pera = new PeraPolygon();

    g_HatsuneMiku->Init();
    g_Pera->CreateBokehParamResource();
    g_Pera->CreatePeraResorce();
    g_Pera->CreatePeraVertex();
    g_Pera->CreatePeraPipeline();

    g_FinalResource = new FinalResource();
    g_FinalResource->Init();

    g_bgm->Load("Assets/Audio/アンノウン・マザーグース.wav");

    // １フレームだけ進ませる
    g_HatsuneMiku->Update();
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
    // ペラポリゴン描画準備
    g_Pera->PrePeraDraw1();

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
    g_Pera->PostPeraDraw1();

    // フィルタ二枚目
    g_Pera->PrePeraDraw2();
    g_Pera->PeraDraw1();
    g_Pera->PostPeraDraw2();

    // 最終的に画面に映すもの
    g_FinalResource->PreFinalDraw();
    g_Pera->PeraDraw2();
    g_FinalResource->PostFinalDraw();

    // シャドウバッファの作成
    //DX12Renderer::BeginDepthShadow();
    // ライトのカメラ行列をセット
    //DX12Renderer::SetView(&light.ViewMatrix);
    //DX12Renderer::SetProj(&light.ProjMatrix);

    // ここにオブジェクトの描画
    DX12Renderer::Begin();

    g_FinalResource->FinalDraw();

    DX12Renderer::End();

}
