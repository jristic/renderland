#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <GL/gl.h>
#include "glext.h"
#include "wglext.h"

void Print(const char *str, ...)
{
	char buf[2048];

	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,str,ptr);
	va_end(ptr);

	OutputDebugString(buf);
	OutputDebugString("\n");
}

void Printnln(const char *str, ...)
{
	char buf[2048];

	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,str,ptr);
	va_end(ptr);

	OutputDebugString(buf);
}

#define null nullptr
#define assert(expression, message, ...) 				\
	do { 												\
		if (!(expression)) {							\
			Print("/* ---- ASSERT ---- */");			\
			Print("LOCATION:  %s@%d",					\
				__FILE__, __LINE__);			 		\
			Print("CONDITION:  %s", #expression);		\
			Printnln("MESSAGE:    ");					\
			Print(message, ##__VA_ARGS__);				\
			DebugBreak();								\
		}												\
	__pragma(warning(suppress:4127))					\
	} while (0);										\


#define GL_PROC_TUPLE \
GL_PROC_ENTRY( PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB) \
GL_PROC_ENTRY( PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB) \
GL_PROC_ENTRY( PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT) \
GL_PROC_ENTRY( PFNGLATTACHSHADERPROC, glAttachShader) \
GL_PROC_ENTRY( PFNGLBINDBUFFERPROC, glBindBuffer) \
GL_PROC_ENTRY( PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
GL_PROC_ENTRY( PFNGLBUFFERDATAPROC, glBufferData) \
GL_PROC_ENTRY( PFNGLCOMPILESHADERPROC, glCompileShader) \
GL_PROC_ENTRY( PFNGLCREATEPROGRAMPROC, glCreateProgram) \
GL_PROC_ENTRY( PFNGLCREATESHADERPROC, glCreateShader) \
GL_PROC_ENTRY( PFNGLDELETEBUFFERSPROC, glDeleteBuffers) \
GL_PROC_ENTRY( PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
GL_PROC_ENTRY( PFNGLDELETESHADERPROC, glDeleteShader) \
GL_PROC_ENTRY( PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays) \
GL_PROC_ENTRY( PFNGLDETACHSHADERPROC, glDetachShader) \
GL_PROC_ENTRY( PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
GL_PROC_ENTRY( PFNGLGENBUFFERSPROC, glGenBuffers) \
GL_PROC_ENTRY( PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
GL_PROC_ENTRY( PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation) \
GL_PROC_ENTRY( PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
GL_PROC_ENTRY( PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
GL_PROC_ENTRY( PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
GL_PROC_ENTRY( PFNGLGETSHADERIVPROC, glGetShaderiv) \
GL_PROC_ENTRY( PFNGLLINKPROGRAMPROC, glLinkProgram) \
GL_PROC_ENTRY( PFNGLSHADERSOURCEPROC, glShaderSource) \
GL_PROC_ENTRY( PFNGLUSEPROGRAMPROC, glUseProgram) \
GL_PROC_ENTRY( PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
GL_PROC_ENTRY( PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation) \
GL_PROC_ENTRY( PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
GL_PROC_ENTRY( PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) \
GL_PROC_ENTRY( PFNGLACTIVETEXTUREPROC, glActiveTexture) \
GL_PROC_ENTRY( PFNGLUNIFORM1IPROC, glUniform1i) \
GL_PROC_ENTRY( PFNGLGENERATEMIPMAPPROC, glGenerateMipmap) \
GL_PROC_ENTRY( PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray) \
GL_PROC_ENTRY( PFNGLUNIFORM3FVPROC, glUniform3fv) \
GL_PROC_ENTRY( PFNGLUNIFORM4FVPROC, glUniform4fv) \

#define GL_PROC_ENTRY(type, name) type name;
GL_PROC_TUPLE
#undef GL_PROC_ENTRY

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

int WINAPI WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	PSTR CmdLine,
	int CmdShow)
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
	
	/* ---- Load extension list ---- */
	{
		HDC DeviceContext = GetDC(Hwnd);
		assert(DeviceContext, "no dc");
		PIXELFORMATDESCRIPTOR PixelFormat;
		int Error = SetPixelFormat(DeviceContext, 1, &PixelFormat);
		assert(Error == 1, "error on SetPixelFormat");
		HGLRC RenderContext = wglCreateContext(DeviceContext);
		assert(RenderContext, "no rdc");
		Error = wglMakeCurrent(DeviceContext, RenderContext);
		
#define GL_PROC_ENTRY(type, name) name = (type)wglGetProcAddress(#name); \
								  assert(name, "failed to get %s", #name);
		GL_PROC_TUPLE
#undef GL_PROC_ENTRY
		
		wglMakeCurrent(null, null);
		wglDeleteContext(RenderContext);
		ReleaseDC(Hwnd, DeviceContext);
	}
	/* ----------------------------- */
	
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
	
	/* ---- OpenGL Initialization ---- */
	HDC DeviceContext;
	HGLRC RenderContext;
	{
		DeviceContext = GetDC(Hwnd);
		assert(DeviceContext, "no dc");
		
		int AttributeList[19];
		// Support for OpenGL rendering.
		AttributeList[0] = WGL_SUPPORT_OPENGL_ARB;
		AttributeList[1] = TRUE;
		// Support for rendering to a window.
		AttributeList[2] = WGL_DRAW_TO_WINDOW_ARB;
		AttributeList[3] = TRUE;
		// Support for hardware acceleration.
		AttributeList[4] = WGL_ACCELERATION_ARB;
		AttributeList[5] = WGL_FULL_ACCELERATION_ARB;
		// Support for 24bit color.
		AttributeList[6] = WGL_COLOR_BITS_ARB;
		AttributeList[7] = 24;
		// Support for 24 bit depth buffer.
		AttributeList[8] = WGL_DEPTH_BITS_ARB;
		AttributeList[9] = 24;
		// Support for double buffer.
		AttributeList[10] = WGL_DOUBLE_BUFFER_ARB;
		AttributeList[11] = TRUE;
		// Support for swapping front and back buffer.
		AttributeList[12] = WGL_SWAP_METHOD_ARB;
		AttributeList[13] = WGL_SWAP_EXCHANGE_ARB;
		// Support for the RGBA pixel type.
		AttributeList[14] = WGL_PIXEL_TYPE_ARB;
		AttributeList[15] = WGL_TYPE_RGBA_ARB;
		// Support for a 8 bit stencil buffer.
		AttributeList[16] = WGL_STENCIL_BITS_ARB;
		AttributeList[17] = 8;
		// Null terminate the attribute list.
		AttributeList[18] = 0;
		
		unsigned int FormatCount;
		int PixelFormat;
		int Result = wglChoosePixelFormatARB(
			DeviceContext,
			AttributeList,
			null,
			1,
			&PixelFormat,
			&FormatCount);
		assert(Result == 1, "failed wglChoosePixelFormatARB");
		
		PIXELFORMATDESCRIPTOR PixelFormatDescriptor;
		Result = SetPixelFormat(
			DeviceContext,
			PixelFormat,
			&PixelFormatDescriptor);
		assert(Result == 1, "failed SetPixelFormat");
		
		int GLAttributeList[5];
		GLAttributeList[0] = WGL_CONTEXT_MAJOR_VERSION_ARB;
		GLAttributeList[1] = 4;
		GLAttributeList[2] = WGL_CONTEXT_MINOR_VERSION_ARB;
		GLAttributeList[3] = 4;
		// Null terminate the attribute list.
		GLAttributeList[4] = 0;
		
		RenderContext = wglCreateContextAttribsARB(
			DeviceContext,
			0,
			GLAttributeList);
		assert(RenderContext, "no rdc");
		
		Result = wglMakeCurrent(DeviceContext, RenderContext);
		assert(Result == 1, "failed wglmakeCurrent");
		
		Print("%s-%s\n",
			(char*)glGetString(GL_VENDOR),
			(char*)glGetString(GL_RENDERER));
		
		// @@@vsync
		Result = wglSwapIntervalEXT(1);
		assert(Result ==1, "failed wglSwapIntervalEXT");
	}
	/* ------------------------------- */
	
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
	
	wglMakeCurrent(null, null);
	wglDeleteContext(RenderContext);
	ReleaseDC(Hwnd, DeviceContext);
	
	DestroyWindow(Hwnd);
	UnregisterClass(WindowClass.lpszClassName, Instance);
}
