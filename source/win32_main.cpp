#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <GL/gl.h>
#include "glext.h"
#include "wglext.h"

// Library headers
#include "imgui.h"

// Local headers
#include "file.h"
#include "win32_imgui.h"

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
typedef unsigned int uint;
#define Assert(expression, message, ...) 				\
	do { 												\
		__pragma(warning(suppress:4127))				\
		if (!(expression)) {							\
			Print("/* ---- Assert ---- */");			\
			Print("LOCATION:  %s@%d",					\
				__FILE__, __LINE__);			 		\
			Print("CONDITION:  %s", #expression);		\
			Printnln("MESSAGE:    ");					\
			Print(message, ##__VA_ARGS__);				\
			if (IsDebuggerPresent())					\
			{											\
				DebugBreak();							\
			}											\
			else										\
			{											\
				exit(-1);								\
			}											\
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
GL_PROC_ENTRY( PFNGLBLENDEQUATIONPROC, glBlendEquation) \
GL_PROC_ENTRY( PFNGLMAPBUFFERPROC, glMapBuffer) \
GL_PROC_ENTRY( PFNGLUNMAPBUFFERPROC, glUnmapBuffer) \

#define GL_PROC_ENTRY(type, name) type name;
GL_PROC_TUPLE
#undef GL_PROC_ENTRY


static int FramebufferWidth;
static int FramebufferHeight;


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
		case WM_LBUTTONDOWN:
			ImGui_MouseButtonCallback(VK_LBUTTON, true);
			return 0;
		case WM_RBUTTONDOWN:
			ImGui_MouseButtonCallback(VK_RBUTTON, true);
			return 0;
		case WM_MBUTTONDOWN:
			ImGui_MouseButtonCallback(VK_MBUTTON, true);
			return 0;
		case WM_KEYDOWN:
			ImGui_KeyCallback((int)Wparam, true);
			return 0;
		case WM_KEYUP:
			ImGui_KeyCallback((int)Wparam, false);
			return 0;
		case WM_CHAR:
	        ImGui_CharCallback((char)Wparam);
			return 0;
		case WM_MOUSEWHEEL:
	        ImGui_ScrollCallback(0, GET_WHEEL_DELTA_WPARAM(Wparam));
	        return 0;
	    case WM_SIZE:
	    {
    		RECT Client;
			GetClientRect(Hwnd, &Client);
			FramebufferWidth = Client.right - Client.left;
			FramebufferHeight = Client.bottom - Client.top;
			return 0;
		}
		// Any other messages send to the default message handler as our application won't make use of them.
		default:
			return DefWindowProc(Hwnd, Msg, Wparam, Lparam);
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
	Assert(Hwnd, "failed to create ogl init window");
	
	ShowWindow(Hwnd, SW_HIDE);
	
	/* ---- Load extension list ---- */
	{
		HDC DeviceContext = GetDC(Hwnd);
		Assert(DeviceContext, "no dc");
		PIXELFORMATDESCRIPTOR PixelFormat;
		int Error = SetPixelFormat(DeviceContext, 1, &PixelFormat);
		Assert(Error == 1, "error on SetPixelFormat");
		HGLRC RenderContext = wglCreateContext(DeviceContext);
		Assert(RenderContext, "no rdc");
		Error = wglMakeCurrent(DeviceContext, RenderContext);
		
#define GL_PROC_ENTRY(type, name) name = (type)wglGetProcAddress(#name); \
								  Assert(name, "failed to get %s", #name);
		GL_PROC_TUPLE
#undef GL_PROC_ENTRY
		
		wglMakeCurrent(null, null);
		wglDeleteContext(RenderContext);
		ReleaseDC(Hwnd, DeviceContext);
	}
	/* ----------------------------- */
	
	DestroyWindow(Hwnd);
	
	const int WindowWidth = 800;
	const int WindowHeight = 600;
	
	Hwnd = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		WindowClass.lpszClassName,
		WindowClass.lpszClassName,
		WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		WindowWidth,
		WindowHeight,
		null,
		null,
		Instance,
		null);
	Assert(Hwnd, "failed to create program window");
	
	/* ---- OpenGL Initialization ---- */
	HDC DeviceContext;
	HGLRC RenderContext;
	{
		DeviceContext = GetDC(Hwnd);
		Assert(DeviceContext, "no dc");
		
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
		Assert(Result == 1, "failed wglChoosePixelFormatARB");
		
		PIXELFORMATDESCRIPTOR PixelFormatDescriptor;
		Result = SetPixelFormat(
			DeviceContext,
			PixelFormat,
			&PixelFormatDescriptor);
		Assert(Result == 1, "failed SetPixelFormat");
		
		int GLAttributeList[5];
		GLAttributeList[0] = WGL_CONTEXT_MAJOR_VERSION_ARB;
		GLAttributeList[1] = 4;
		GLAttributeList[2] = WGL_CONTEXT_MINOR_VERSION_ARB;
		GLAttributeList[3] = 5;
		// Null terminate the attribute list.
		GLAttributeList[4] = 0;
		
		RenderContext = wglCreateContextAttribsARB(
			DeviceContext,
			0,
			GLAttributeList);
		Assert(RenderContext, "no rdc");
		
		Result = wglMakeCurrent(DeviceContext, RenderContext);
		Assert(Result == 1, "failed wglmakeCurrent");
		
		Print("GPU Info: %s-%s",
			(char*)glGetString(GL_VENDOR),
			(char*)glGetString(GL_RENDERER));
		Print("OpenGL version: %s",
			(char*)glGetString(GL_VERSION));
		
		// @@@vsync
		Result = wglSwapIntervalEXT(1);
		Assert(Result == 1, "failed wglSwapIntervalEXT");
	}
	/* ------------------------------- */
	
	/* ----- Create temp shader ----- */
	const int MaxFileLen = 2048;
	int VertexShaderLen, PixelShaderLen;
	char VertexShaderBuffer[MaxFileLen];
	char PixelShaderBuffer[MaxFileLen];
	File::ReadFile("shader/color.vs", VertexShaderBuffer, &VertexShaderLen, MaxFileLen);
	File::ReadFile("shader/color.ps", PixelShaderBuffer, &PixelShaderLen, MaxFileLen);
	GLenum ColorVertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLenum ColorPixelShader = glCreateShader(GL_FRAGMENT_SHADER);
	char* VSPtr = VertexShaderBuffer;
	char* PSPtr = PixelShaderBuffer;
	glShaderSource(ColorVertexShader, 1, &VSPtr, null);
	glShaderSource(ColorPixelShader, 1, &PSPtr, null);
	glCompileShader(ColorVertexShader);
	int Status;
	glGetShaderiv(ColorVertexShader, GL_COMPILE_STATUS, &Status);
	if (Status != GL_TRUE)
	{
		char buf[1024];
		glGetShaderInfoLog(ColorVertexShader, 1024, null, buf);
		Assert(false, "Failed to compile shader: color.vs \n%s", buf);
	}
	glCompileShader(ColorPixelShader);
	glGetShaderiv(ColorPixelShader, GL_COMPILE_STATUS, &Status);
	if (Status != GL_TRUE)
	{
		char buf[1024];
		glGetShaderInfoLog(ColorPixelShader, 1024, null, buf);
		Assert(false, "Failed to compile shader: color.ps \n%s", buf);
	}
	GLenum ColorShaderProgram = glCreateProgram();
	glAttachShader(ColorShaderProgram, ColorVertexShader);
	glAttachShader(ColorShaderProgram, ColorPixelShader);
	glBindAttribLocation(ColorShaderProgram, 0, "gInputPosition");
	glBindAttribLocation(ColorShaderProgram, 1, "gInputColor");
	glLinkProgram(ColorShaderProgram);
	glGetProgramiv(ColorShaderProgram, GL_LINK_STATUS, &Status);
	if (Status != 1)
	{
		char buf[1024];
		glGetProgramInfoLog(ColorShaderProgram, 1024, null, buf);
		Assert(false, "Failed to link shader program: \n%s", buf);
	}
	/* ------------------------------ */
	
	/* ---- Set shader constants ---- */
	{
		float Matrix[16] = {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		};
		glUseProgram(ColorShaderProgram);
		int location = glGetUniformLocation(ColorShaderProgram, "gProjViewWorldMatrix");
		glUniformMatrix4fv(location, 1, false, Matrix);
	}
	/* ------------------------------ */
	
	/* ----- Create temp buffers ---- */
	float Vertices[18] = {
		-1, -1, 0,	// Position 0
		1, 0, 0,	// Color 0
		0, 1, 0,	// Position 1
		0, 1, 0,	// Color 1
		1, -1, 0,	// Position 2
		0, 0, 1		// Color 2
	};
	GLenum VertexArray, VertexBuffer;
	glGenVertexArrays(1, &VertexArray);
	glBindVertexArray(VertexArray);
	glGenBuffers(1, &VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), Vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 6 * sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	uint Indices[3] = { 0, 1, 2 };
	GLenum IndexBuffer;
	glGenBuffers(1, &IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(uint), Indices, GL_STATIC_DRAW);
	/* ------------------------------ */
	
	RECT Client;
	GetClientRect(Hwnd, &Client);
	FramebufferWidth = Client.right - Client.left;
	FramebufferHeight = Client.bottom - Client.top;
	
	Assert(FramebufferWidth > 0, "");
	Assert(FramebufferHeight > 0, "");
	
	ImGui_Init(Hwnd);
	
	ShowWindow(Hwnd, SW_SHOW);
	
	MSG Msg;
	bool Done = false;
	float ClearColor[4] = {0.2f, 0.2f, 0.2f};

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
		
		ImGui_NewFrame(FramebufferWidth, FramebufferHeight);
		
        ImGui::ColorEdit3("clear color", (float*)ClearColor);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
			1000.0f / ImGui::GetIO().Framerate,
			ImGui::GetIO().Framerate);

        POINT Point;
    	GetCursorPos(&Point);
        ScreenToClient(Hwnd, &Point);
        // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
    	ImGui::LabelText("things", "%d %d", Point.x, Point.y);
		
		glViewport(0, 0, FramebufferWidth, FramebufferHeight);
		glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
		glBindVertexArray(VertexArray);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 6 * sizeof(float), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, false, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glUseProgram(ColorShaderProgram);
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
		
		ImGui::Render();

		SwapBuffers(DeviceContext);
	}
	
	ImGui_Shutdown();
	
	/* ----- Clean up temps ----- */
	glDetachShader(ColorShaderProgram, ColorVertexShader);
	glDetachShader(ColorShaderProgram, ColorPixelShader);
	glDeleteShader(ColorVertexShader);
	glDeleteShader(ColorPixelShader);
	glDeleteProgram(ColorShaderProgram);
	
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &VertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &IndexBuffer);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &VertexArray);
	/* ----- Clean up temps ----- */
	
	wglMakeCurrent(null, null);
	wglDeleteContext(RenderContext);
	ReleaseDC(Hwnd, DeviceContext);
	
	DestroyWindow(Hwnd);
	UnregisterClass(WindowClass.lpszClassName, Instance);
}


// Library source
#include "imgui.cpp"

// Local source
#include "file.cpp"
#include "win32_imgui.cpp"