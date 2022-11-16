#include "framework.h"
#include "MainManager.h"

// レンダーモジュール
#include "DX12Renderer.h"

// オブジェクト
#include "Polygon2D.h"

// インスタンス
Polygon2D* g_Polygon2D;

void MainManager::Init()
{
    // レンダー初期化
    DX12Renderer::Init();

    g_Polygon2D = new Polygon2D();

    g_Polygon2D->Init();
}

void MainManager::Uninit()
{
    g_Polygon2D->Uninit();

    // レンダー終了
    DX12Renderer::Uninit();
}

void MainManager::Update()
{
    g_Polygon2D->Update();
}

void MainManager::Draw()
{
    DX12Renderer::Begin();

    // ここにオブジェクトの描画
    g_Polygon2D->Draw();

    DX12Renderer::End();

}
