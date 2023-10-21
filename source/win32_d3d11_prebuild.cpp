// Files which are not frequently changed as part of project development
// 	so they can be built once and reused to speed up build time. 

// External source
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
// #include "imgui/imgui_demo.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#pragma warning( push )
#pragma warning( disable : 4100 )
#include "ImGuiFileDialog/ImGuiFileDialog.cpp"
#pragma warning( pop )

// Imgui example backend
#include "imgui_impl_win32.cpp"

#if D3D11
	#include "imgui_impl_dx11.cpp"
#elif D3D12
	#include "imgui_impl_dx12.cpp"
#else
	#error unimplemented
#endif
