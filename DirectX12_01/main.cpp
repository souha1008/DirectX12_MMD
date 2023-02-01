//==============================================================
// Filename: Task01_Main.cpp
// Description: 中央管理
// Copyright (C)  Silicon Studio Co., Ltd. All rights reserved.
//==============================================================

#include "framework.h"
#include "DirectX12_01.h"
#include "MainManager.h"
#include "input.h"

/// キャプション定義
const char* g_ClassName = "AppClass";

const char* g_WindowName = "DirectX12";

/// プロトタイプ宣言

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HWND g_Window;

HWND GetWindow()
{
	return g_Window;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcex =
	{
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc,
		0,
		0,
		hInstance,
		NULL,
		LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		NULL,
		g_ClassName,
		NULL
	};

	RegisterClassEx(&wcex);

	RECT wrc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

	g_Window = CreateWindow(
		g_ClassName,
		g_WindowName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(wrc.right - wrc.left),
		(wrc.bottom - wrc.top),
		NULL,
		NULL,
		hInstance,
		NULL);


	/// 使用オブジェクトの初期化
	MainManager::Init();

	Input::Init();


	ShowWindow(g_Window, nCmdShow);
	UpdateWindow(g_Window);




	DWORD dwExecLastTime;
	DWORD dwCurrentTime;
	timeBeginPeriod(1);
	dwExecLastTime = timeGetTime();
	dwCurrentTime = 0;



	MSG msg;
	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			dwCurrentTime = timeGetTime();

			if ((dwCurrentTime - dwExecLastTime) >= (1000 / 60))
			{
				dwExecLastTime = dwCurrentTime;

				// 更新処理
				Input::Update();
				MainManager::Update();
				// 描画処理
				MainManager::Draw();
			}
		}
	}

	timeEndPeriod(1);

	UnregisterClass(g_ClassName, wcex.hInstance);

	// 終了処理
	MainManager::Uninit();

	Input::Uninit();

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(hWnd);
			break;
		}
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}