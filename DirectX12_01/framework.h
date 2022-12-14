// header.h : 標準のシステム インクルード ファイルのインクルード ファイル、
// またはプロジェクト専用のインクルード ファイル
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーからほとんど使用されていない部分を除外する
// Windows ヘッダー ファイル
#include <windows.h>
// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <wrl.h>

using namespace Microsoft::WRL;


// DirectX12
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <D3Dcompiler.h>
#include <DirectXTex.h>

using namespace DirectX;

#include <assert.h>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <unordered_map>

#pragma warning(push)
#pragma warning(disable:4005)
#include "mmsystem.h"

#define DIRECTINPUT_VERSION 0x0800		// 警告対処
#pragma warning(pop)
#pragma comment (lib, "winmm.lib")

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "DirectXTex.lib")



// スクリーンサイズ
#define SCREEN_WIDTH    1280
#define SCREEN_HEIGHT    720

HWND GetWindow();
