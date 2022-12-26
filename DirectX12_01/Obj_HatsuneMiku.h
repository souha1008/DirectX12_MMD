#pragma once
#include "3DObject.h"
class Obj_HatsuneMiku
{
public:
    void Init();
    void Uninit();
    void Update();
    void Draw();

private:
    Object3D* m_Object;
};

