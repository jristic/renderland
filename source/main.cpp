#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define null nullptr
#define assert(expression, message, ...) 				\
	do { 												\
		if (!(expression)) {							\
			fprintf(stderr, "/* ---- ASSERT ---- */\n");\
			fprintf(stderr,								\
				"LOCATION:   %s@%d\nCONDITION:  %s \n",	\
				__FILE__, __LINE__, #expression); 		\
			fprintf(stderr, "MESSAGE:    ");			\
			fprintf(stderr, message, ##__VA_ARGS__);	\
			fprintf(stderr, "\n");						\
			DebugBreak();								\
		}												\
	__pragma(warning(suppress:4127))						\
	} while (0);										\


LRESULT CALLBACK WndProc(HWND Hwnd, UINT Msg, WPARAM Wparam, LPARAM Lparam)
{
	switch(Msg)
	{
		// Check if the window is being closed.
		case WM_CLOSE:
		{
			PostQuitMessage(0);		
			return 0;
		}
		// Any other messages send to the default message handler as our application won't make use of them.
		default:
		{
			return DefWindowProc(Hwnd, Msg, Wparam, Lparam);
		}
	}
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CmdLine, int CmdShow)
{
	(void)PrevInstance;
	(void)CmdLine;
	(void)CmdShow;
	
	WNDCLASSEX WindowClass;

	// Setup the windows class with default settings.
	WindowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc   = WndProc;
	WindowClass.cbClsExtra    = 0;
	WindowClass.cbWndExtra    = 0;
	WindowClass.hInstance     = Instance;
	WindowClass.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
	WindowClass.hIconSm       = WindowClass.hIcon;
	WindowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WindowClass.lpszMenuName  = NULL;
	WindowClass.lpszClassName = "Renderland";
	WindowClass.cbSize        = sizeof(WNDCLASSEX);
	
	// Register the window class.
	RegisterClassEx(&WindowClass);
	
	// Temp window for ogl init
	HWND Hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		WindowClass.lpszClassName,
		WindowClass.lpszClassName,
		WS_POPUP,
		0, 0, 640, 480,
		null,
		null,
		Instance,
		null);
	assert(Hwnd, "failed to create ogl init window");
	
	ShowWindow(Hwnd, SW_HIDE);
	// TODO(jovanr): add opengl initialization
	DestroyWindow(Hwnd);
	
	Hwnd = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		WindowClass.lpszClassName,
		WindowClass.lpszClassName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		800,
		600,
		null,
		null,
		Instance,
		null);
	assert(Hwnd, "failed to create program window");
	
	ShowWindow(Hwnd, SW_SHOW);
	
	MSG Msg;
	bool Done = false;

	// Initialize the message structure.
	ZeroMemory(&Msg, sizeof(MSG));
	
	// Loop until there is a quit message from the window or the user.
	while (!Done)
	{
		// Handle the windows messages.
		if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}

		// If windows signals to end the application then exit out.
		if(Msg.message == WM_QUIT)
		{
			Done = true;
		}
	}
	
	DestroyWindow(Hwnd);
	UnregisterClass(WindowClass.lpszClassName, Instance);
}
