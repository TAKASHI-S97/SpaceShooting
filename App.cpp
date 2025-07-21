#include "App.h"

int WINAPI WinMain(
    _In_        HINSTANCE   hInstance,
    _In_opt_    HINSTANCE   hPrevInstance,
    _In_        LPSTR       lpCmdLine,
    _In_        int         nShowCmd
)
{
	using namespace SpaceShooting;

	App m_app(hInstance);
	int returncode = m_app.Run();

	return returncode;
}

using namespace SpaceShooting;

App::App(HINSTANCE hInstance) :m_hInst(hInstance), m_hwnd(nullptr), gdiplusToken(0)
{
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}

App::~App()
{
	Gdiplus::GdiplusShutdown(gdiplusToken);
}

int App::Run()
{
	WNDCLASS wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = App::WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInst;
	wc.hIcon = LoadIcon(m_hInst, MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor = nullptr;
	wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = TEXT("SpaceShooting");

	RegisterClass(&wc);

	RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE);

	m_hwnd = CreateWindow(
		TEXT("SpaceShooting"),
		TEXT("SpaceShooting"),
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right-rect.left,
		rect.bottom-rect.top,
		nullptr,
		nullptr,
		m_hInst,
		this
	);

	ShowWindow(m_hwnd, SW_SHOW);
	UpdateWindow(m_hwnd);

	// FPS制御下準備
	LARGE_INTEGER frequency = { 0 };
	LARGE_INTEGER lastTime = { 0 };
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastTime);
	const int FPS = 60;

	// ゲーム開始
	Game game(WINDOW_WIDTH, WINDOW_HEIGHT);

	MSG msg = { 0 };
	bool running = true;

	while (running)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		LARGE_INTEGER currentTime = { 0 };
		QueryPerformanceCounter(&currentTime);

		LONGLONG elapsedTime = currentTime.QuadPart - lastTime.QuadPart;
		LONGLONG frameDuration = frequency.QuadPart / FPS;

		if (elapsedTime >= frameDuration)
		{
			lastTime = currentTime;

			HDC hdc = GetDC(m_hwnd);

			// ダブルバッファリングの下準備
			HDC memhdc = CreateCompatibleDC(hdc);
			HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
			SelectObject(memhdc, hBitmap);

			game.Update(memhdc);

#ifdef _DEBUG		// FPS確認
			RECT debugRect = { WINDOW_WIDTH - 120, 0, WINDOW_WIDTH, 20 };
			float checkFPS = static_cast<float>(frequency.QuadPart) / elapsedTime;
			wchar_t debugBuffer[10];
			swprintf(debugBuffer, 10, L"FPS: %.1f", checkFPS);
			DrawText(memhdc, debugBuffer, 10, &debugRect, DT_RIGHT);
#endif
			// スワッピング
			BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memhdc, 0, 0, SRCCOPY);

			// リソース解放
			DeleteObject(hBitmap);
			DeleteDC(memhdc);
			ReleaseDC(m_hwnd, hdc);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK App::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}
