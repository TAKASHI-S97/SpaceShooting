#pragma once

#include <cstdio>
#include "Game.h"

namespace SpaceShooting
{

class App
{
public:
	App(HINSTANCE);
	~App();

	int Run(void);
private:
	const int WINDOW_WIDTH = 480;
	const int WINDOW_HEIGHT = 800;

	HINSTANCE m_hInst;
	HWND m_hwnd;

	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

}