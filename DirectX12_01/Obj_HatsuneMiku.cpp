#include "framework.h"
#include "DX12Renderer.h"
#include "Obj_HatsuneMiku.h"

void Obj_HatsuneMiku::Init()
{
    m_Object = new Object3D();

    HRESULT hr = m_Object->CreateModel("Assets/Model/シンプルモデル/初音ミク.pmd"
        , "Assets/VMD/Unknown.vmd");

    if (FAILED(hr))
    {
        return;
    }

    m_Object->PlayAnimation();
}

void Obj_HatsuneMiku::Uninit()
{
    m_Object->UnInit();
}

void Obj_HatsuneMiku::Update()
{
    m_Object->MotionUpdate();
}

void Obj_HatsuneMiku::Draw()
{
    m_Object->Draw();
}
