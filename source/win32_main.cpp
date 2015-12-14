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
GL_PROC_ENTRY( PFNGLBLENDEQUATIONSEPARATEPROC, glBlendEquationSeparate) \

#define GL_PROC_ENTRY(type, name) type name;
GL_PROC_TUPLE
#undef GL_PROC_ENTRY


static int FramebufferWidth;
static int FramebufferHeight;



#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
// NB: You can use math functions/operators on ImVec2 if you #define IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h"
// Here we only declare simple +/- operators so others don't leak into the demo code.
// static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
// static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }

// Really dumb data structure provided for the example.
// Note that we storing links are INDICES (not ID) to make example code shorter, obviously a bad idea for any general purpose code.
static void ShowExampleAppCustomNodeGraph(bool* opened)
{
	ImGui::SetNextWindowSize(ImVec2(700,600), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("Example: Custom Node Graph", opened))
	{
		ImGui::End();
		return;
	}

	// Dummy
	struct Node
	{
		int     ID;
		char    Name[32];
		ImVec2  Pos, Size;
		float   Value;
		ImVec4  Color;
		int     InputsCount, OutputsCount;
		bool 	Enabled;

		Node() {}
		Node(int id, const char* name, const ImVec2& pos, float value, const ImVec4& color, int inputs_count, int outputs_count) { ID = id; strncpy(Name, name, 31); Name[31] = 0; Pos = pos; Value = value; Color = color; InputsCount = inputs_count; OutputsCount = outputs_count; Enabled = true; }

		ImVec2 GetInputSlotPos(int slot_no) const   { return ImVec2(Pos.x, Pos.y + Size.y * ((float)slot_no+1) / ((float)InputsCount+1)); }
		ImVec2 GetOutputSlotPos(int slot_no) const  { return ImVec2(Pos.x + Size.x, Pos.y + Size.y * ((float)slot_no+1) / ((float)OutputsCount+1)); }
	};
	struct NodeLink
	{
		int     InputIdx, InputSlot, OutputIdx, OutputSlot;

		NodeLink(int input_idx, int input_slot, int output_idx, int output_slot) { InputIdx = input_idx; InputSlot = input_slot; OutputIdx = output_idx; OutputSlot = output_slot; }
	};

	static ImVector<Node> nodes;
	static ImVector<NodeLink> links;
	static bool inited = false;
	static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
	static bool show_grid = true;
	static int node_selected = -1;
	static bool HasCopyNode = false;
	static Node CopyNode;
	static bool RenameFirstRun = false;
	static int RenameNodeID = -1;
	if (!inited)
	{
		nodes.push_back(Node(0, "MainTex", ImVec2(40,50), 0.5f, ImColor(255,100,100), 1, 1));
		nodes.push_back(Node(1, "BumpMap", ImVec2(40,150), 0.42f, ImColor(200,100,200), 1, 1));
		nodes.push_back(Node(2, "Combine", ImVec2(270,80), 1.0f, ImColor(0,200,100), 2, 2));
		links.push_back(NodeLink(0, 0, 2, 0));
		links.push_back(NodeLink(1, 0, 2, 1));
		inited = true;
	}

	// Draw a list of nodes on the left side
	bool open_context_menu = false;
	int node_hovered_in_list = -1;
	int node_hovered_in_scene = -1;
	ImGui::BeginChild("node_list", ImVec2(100,0));
	ImGui::Text("Nodes");
	ImGui::Separator();
	for (int node_idx = 0; node_idx < nodes.Size; node_idx++)
	{
		Node* node = &nodes[node_idx];
		if (!node->Enabled) {
			continue;
		}
		ImGui::PushID(node->ID);
		if (ImGui::Selectable(node->Name, node->ID == node_selected))
			node_selected = node->ID;
		if (ImGui::IsItemHovered())
		{
			node_hovered_in_list = node->ID;
			open_context_menu |= ImGui::IsMouseClicked(IMGUI_RBUTTON);
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();

	const float NODE_SLOT_RADIUS = 4.0f;
	const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

	// Create our child canvas
	ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
	ImGui::SameLine(ImGui::GetWindowWidth()-100);
	ImGui::Checkbox("Show grid", &show_grid);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1,1));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
	ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(60,60,70,200));
	ImGui::BeginChild("scrolling_region", ImVec2(0,0), true, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
	ImGui::PushItemWidth(120.0f);

	ImVec2 offset = ImGui::GetCursorScreenPos() - scrolling;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->ChannelsSplit(2);

	// Display grid
	if (show_grid)
	{
		ImU32 GRID_COLOR = ImColor(200,200,200,40);
		float GRID_SZ = 64.0f;
		ImVec2 win_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_sz = ImGui::GetWindowSize();
		for (float x = fmodf(offset.x,GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
			draw_list->AddLine(ImVec2(x,0.0f)+win_pos, ImVec2(x,canvas_sz.y)+win_pos, GRID_COLOR);
		for (float y = fmodf(offset.y,GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
			draw_list->AddLine(ImVec2(0.0f,y)+win_pos, ImVec2(canvas_sz.x,y)+win_pos, GRID_COLOR);
	}

	// Display links
	draw_list->ChannelsSetCurrent(0); // Background
	for (int link_idx = 0; link_idx < links.Size; link_idx++)
	{
		NodeLink* link = &links[link_idx];
		Node* node_inp = &nodes[link->InputIdx];
		Node* node_out = &nodes[link->OutputIdx];
		ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot);
		ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot);		
		draw_list->AddBezierCurve(p1, p1+ImVec2(+50,0), p2+ImVec2(-50,0), p2, ImColor(200,200,100), 3.0f);
	}

	// Display nodes
	for (int node_idx = 0; node_idx < nodes.Size; node_idx++)
	{
		Node* node = &nodes[node_idx];
		if (!node->Enabled) {
			continue;
		}
		ImGui::PushID(node->ID);
		ImVec2 node_rect_min = offset + node->Pos;

		// Display node contents first
		draw_list->ChannelsSetCurrent(1); // Foreground
		bool old_any_active = ImGui::IsAnyItemActive();
		ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
		ImGui::BeginGroup(); // Lock horizontal position
		if (RenameNodeID == node->ID) {
			if (RenameFirstRun) {
				ImGui::SetKeyboardFocusHere();
				RenameFirstRun = false;
			}
			if (ImGui::InputText("##name", node->Name, 32, 
				ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)) 
			{
				RenameNodeID = -1;
			}
		}
		else {
			ImGui::Text("%s", node->Name);
		}
		ImGui::SliderFloat("##value", &node->Value, 0.0f, 1.0f, "Alpha %.2f");
		ImGui::ColorEdit3("##color", &node->Color.x);
		ImGui::EndGroup();

		// Save the size of what we have emitted and whether any of the widgets are being used
		bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
		node->Size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
		ImVec2 node_rect_max = node_rect_min + node->Size;

		// Display node box
		draw_list->ChannelsSetCurrent(0); // Background
		ImGui::SetCursorScreenPos(node_rect_min);
		ImGui::InvisibleButton("node", node->Size);
		if (ImGui::IsItemHovered())
		{
			node_hovered_in_scene = node->ID;
			open_context_menu |= ImGui::IsMouseClicked(IMGUI_RBUTTON);
		}
		bool node_moving_active = ImGui::IsItemActive();
		if (node_widgets_active || node_moving_active)
			node_selected = node->ID;
		if (node_moving_active && ImGui::IsMouseDragging(IMGUI_LBUTTON))
			node->Pos = node->Pos + ImGui::GetIO().MouseDelta;

		ImU32 node_bg_color = (node_hovered_in_list == node->ID || node_hovered_in_scene == node->ID || (node_hovered_in_list == -1 && node_selected == node->ID)) ? ImColor(75,75,75) : ImColor(60,60,60);
		draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f); 
		draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100,100,100), 4.0f); 
		for (int slot_idx = 0; slot_idx < node->InputsCount; slot_idx++)
			draw_list->AddCircleFilled(offset + node->GetInputSlotPos(slot_idx), NODE_SLOT_RADIUS, ImColor(150,150,150,150));
		for (int slot_idx = 0; slot_idx < node->OutputsCount; slot_idx++)
			draw_list->AddCircleFilled(offset + node->GetOutputSlotPos(slot_idx), NODE_SLOT_RADIUS, ImColor(150,150,150,150));

		ImGui::PopID();
	}
	draw_list->ChannelsMerge();

	// Open context menu
	if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(IMGUI_RBUTTON))
	{
		node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
		open_context_menu = true;
	}
	if (open_context_menu)
	{
		ImGui::OpenPopup("context_menu");
		if (node_hovered_in_list != -1)
			node_selected = node_hovered_in_list;
		if (node_hovered_in_scene != -1)
			node_selected = node_hovered_in_scene;
	}
	
	// Draw context menu
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,8));
	if (ImGui::BeginPopup("context_menu"))
	{
		Node* node = node_selected != -1 ? &nodes[node_selected] : NULL;
		ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
		if (node)
		{
			ImGui::Text("Node '%s'", node->Name);
			ImGui::Separator();
			if (ImGui::MenuItem("Rename..", NULL, false, true)) {
				RenameNodeID = node->ID;
				RenameFirstRun = true;
			}
			if (ImGui::MenuItem("Delete", NULL, false, true)) {
				node->Enabled = false;
				for (int link_idx = 0; link_idx < links.Size; )
				{
					NodeLink* link = &links[link_idx];
					if (link->InputIdx == node_selected || link->OutputIdx == node_selected) {
						*link = links[links.Size-1];
						--links.Size;
					}
					else {
						++link_idx;
					}
				}
			}
			if (ImGui::MenuItem("Copy", NULL, false, true)) {
				CopyNode = *node;
				HasCopyNode = true;
			}
		}
		else
		{
			if (ImGui::MenuItem("Add")) {
				nodes.push_back(Node(nodes.Size, "New node", scene_pos, 0.5f, ImColor(100,100,200), 2, 2));
			}
			if (ImGui::MenuItem("Paste", NULL, false, HasCopyNode)) {
				CopyNode.Pos = scene_pos;
				CopyNode.ID = nodes.Size;
				nodes.push_back(CopyNode);
			}
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
	
	// Scrolling
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(IMGUI_MBUTTON, 0.0f))
		scrolling = scrolling - ImGui::GetIO().MouseDelta;

	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::EndGroup();

	ImGui::End();
}


LRESULT CALLBACK WndProc(HWND Hwnd, UINT Msg, WPARAM Wparam, LPARAM Lparam)
{
	bool down = false;
	switch(Msg)
	{
		// Check if the window is being closed.
		case WM_CLOSE:
		{
			PostQuitMessage(0);		
			return 0;
		}
		case WM_LBUTTONDOWN:
			down = true;
		case WM_LBUTTONUP:
			ImGui_MouseButtonCallback(IMGUI_LBUTTON, down);
			return 0;
		case WM_RBUTTONDOWN:
			down = true;
		case WM_RBUTTONUP:
			ImGui_MouseButtonCallback(IMGUI_RBUTTON, down);
			return 0;
		case WM_MBUTTONDOWN:
			down = true;
		case WM_MBUTTONUP:
			ImGui_MouseButtonCallback(IMGUI_MBUTTON, down);
			return 0;
		case WM_KEYDOWN:
			if ((int)Wparam == VK_ESCAPE) {
				PostQuitMessage(0);
			}
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
	
	const int WindowWidth = 1400;
	const int WindowHeight = 900;
	
	Hwnd = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		WindowClass.lpszClassName,
		WindowClass.lpszClassName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		100,
		100,
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
	bool ShowWindow = false;

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
		
		ShowExampleAppCustomNodeGraph(&ShowWindow);
		
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
	
	ImGui_InvalidateDeviceObjects();
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
#include "imgui_draw.cpp"

// Local source
#include "file.cpp"
#include "win32_imgui.cpp"