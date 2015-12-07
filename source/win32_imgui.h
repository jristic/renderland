// ImGui Win32 binding with OpenGL3 + shaders
// https://github.com/ocornut/imgui

#define IMGUI_LBUTTON 0
#define IMGUI_RBUTTON 1
#define IMGUI_MBUTTON 2

void        ImGui_Init(HWND Hwnd);
void        ImGui_Shutdown();
void        ImGui_NewFrame(int FramebufferWidth, int FramebufferHeight);

void 		ImGui_InvalidateDeviceObjects();

void        ImGui_MouseButtonCallback(int button, bool down);
void        ImGui_ScrollCallback(double xoffset, double yoffset);
void        ImGui_KeyCallback(int key, bool down);
void        ImGui_CharCallback(unsigned int c);
