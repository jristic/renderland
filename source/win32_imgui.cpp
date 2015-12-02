
// Data
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static size_t       g_VboSize = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0;
static HWND gWindow;
static char gClipboard[512];

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
static void ImGui_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    GLint last_program, last_texture;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float ortho_projection[4][4] =
    {
        { 2.0f/width,	0.0f,			0.0f,		0.0f },
        { 0.0f,			2.0f/-height,	0.0f,		0.0f },
        { 0.0f,			0.0f,			-1.0f,		0.0f },
        { -1.0f,		1.0f,			0.0f,		1.0f },
    };
    glUseProgram(g_ShaderHandle);
    glUniform1i(g_AttribLocationTex, 0);
    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

    // Grow our buffer according to what we need
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)
        total_vtx_count += cmd_lists[n]->vtx_buffer.size();
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    size_t needed_vtx_size = total_vtx_count * sizeof(ImDrawVert);
    if (g_VboSize < needed_vtx_size)
    {
        g_VboSize = needed_vtx_size + 5000 * sizeof(ImDrawVert);  // Grow buffer
        glBufferData(GL_ARRAY_BUFFER, g_VboSize, NULL, GL_STREAM_DRAW);
    }

    // Copy and convert all vertices into a single contiguous buffer
    unsigned char* buffer_data = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!buffer_data)
        return;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        memcpy(buffer_data, &cmd_list->vtx_buffer[0], cmd_list->vtx_buffer.size() * sizeof(ImDrawVert));
        buffer_data += cmd_list->vtx_buffer.size() * sizeof(ImDrawVert);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(g_VaoHandle);

    int cmd_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        int vtx_offset = cmd_offset;
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
            if (pcmd->user_callback)
            {
                pcmd->user_callback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->texture_id);
                glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
                glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            }
            vtx_offset += pcmd->vtx_count;
        }
        cmd_offset = vtx_offset;
    }

    // Restore modified state
    glBindVertexArray(0);
    glUseProgram(last_program);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, last_texture);
}

static const char* ImGui_GetClipboardText()
{
    OpenClipboard(NULL);
    HANDLE clip0 = GetClipboardData(CF_TEXT); 
    char* c = (char*)GlobalLock(clip0);
    strcpy_s(gClipboard, 512, c);
    GlobalUnlock(clip0);
    CloseClipboard();
    return gClipboard;
}

static void ImGui_SetClipboardText(const char* text)
{
    const size_t Len = strlen(text) + 1;
    HGLOBAL Mem =  GlobalAlloc(GMEM_MOVEABLE, Len);
    memcpy(GlobalLock(Mem), text, Len);
    GlobalUnlock(Mem);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, Mem);
    CloseClipboard();
}

void ImGui_MouseButtonCallback(int button, bool down)
{
    if (down)
        g_MousePressed[button] = true;
}

void ImGui_ScrollCallback(double /*xoffset*/, double yoffset)
{
    g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

void ImGui_KeyCallback(int key, bool down)
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[key] = down;

    io.KeyCtrl = io.KeysDown[VK_LCONTROL] || io.KeysDown[VK_RCONTROL];
    io.KeyShift = io.KeysDown[VK_LSHIFT] || io.KeysDown[VK_RSHIFT];
    io.KeyAlt = io.KeysDown[VK_LMENU] || io.KeysDown[VK_RMENU];
}

void ImGui_CharCallback(unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

void ImGui_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();
}

bool ImGui_CreateDeviceObjects()
{
    const GLchar *vertex_shader =
        "#version 330\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 330\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";

    g_ShaderHandle = glCreateProgram();
    g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
    g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
    glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
    glCompileShader(g_VertHandle);
    glCompileShader(g_FragHandle);
    glAttachShader(g_ShaderHandle, g_VertHandle);
    glAttachShader(g_ShaderHandle, g_FragHandle);
    glLinkProgram(g_ShaderHandle);

    g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

    glGenBuffers(1, &g_VboHandle);

    glGenVertexArrays(1, &g_VaoHandle);
    glBindVertexArray(g_VaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glEnableVertexAttribArray(g_AttribLocationPosition);
    glEnableVertexAttribArray(g_AttribLocationUV);
    glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ImGui_CreateFontsTexture();

    return true;
}

void ImGui_Init(HWND window) {
    gWindow = window;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;                 // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 0x41;
    io.KeyMap[ImGuiKey_C] = 0x43;
    io.KeyMap[ImGuiKey_V] = 0x56;
    io.KeyMap[ImGuiKey_X] = 0x58;
    io.KeyMap[ImGuiKey_Y] = 0x59;
    io.KeyMap[ImGuiKey_Z] = 0x5A;

    io.RenderDrawListsFn = ImGui_RenderDrawLists;
    io.SetClipboardTextFn = ImGui_SetClipboardText;
    io.GetClipboardTextFn = ImGui_GetClipboardText;
#ifdef _MSC_VER
    io.ImeWindowHandle = window;
#endif
}

void ImGui_Shutdown()
{
    if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
    if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
    g_VaoHandle = 0;
    g_VboHandle = 0;

    glDetachShader(g_ShaderHandle, g_VertHandle);
    glDeleteShader(g_VertHandle);
    g_VertHandle = 0;

    glDetachShader(g_ShaderHandle, g_FragHandle);
    glDeleteShader(g_FragHandle);
    g_FragHandle = 0;

    glDeleteProgram(g_ShaderHandle);
    g_ShaderHandle = 0;

    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
    ImGui::Shutdown();
}

void ImGui_NewFrame(int FramebufferWidth, int FramebufferHeight)
{
    if (!g_FontTexture)
        ImGui_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2((float)FramebufferWidth, (float)FramebufferHeight);

    // Setup time step
    static bool first = true;
    static LARGE_INTEGER Freq;
    static LARGE_INTEGER LastTimer;
    if (first)
    {
        QueryPerformanceFrequency(&Freq);
        QueryPerformanceCounter(&LastTimer);
        first = false;
    }
        
    LARGE_INTEGER CurrentTimer;
    QueryPerformanceCounter(&CurrentTimer);
    float Delta = (float)((double)(CurrentTimer.QuadPart - LastTimer.QuadPart) / (double)Freq.QuadPart);
    
    LastTimer = CurrentTimer;
    
    io.DeltaTime = max(Delta, (float)(1.0f/60.0f));

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (GetForegroundWindow() == gWindow)
    {
        POINT Point;
    	GetCursorPos(&Point);
        ScreenToClient(gWindow, &Point);
        // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
    	io.MousePos = ImVec2(
            (float)Point.x ,
            (float)Point.y );
    }
    else
    {
    	io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't 
        //  miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = g_MousePressed[i] || ((GetKeyState(VK_LBUTTON) & 0x100) != 0);    
        g_MousePressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    // glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}
