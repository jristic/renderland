
namespace main {


const int LAYOUT_VERSION = 1;

// Forward declarations of helper functions
void LoadRlf(State* s);
void UnloadRlf(State* s);
void ReportError(State* s, const std::string& message);

std::string RlfFileLocation(const char* buffer_start, u32 buffer_size, 
	const char* filename, const char* location)
{
	Assert(filename, "File must be provided.");
	if (!location)
		return "";
	u32 line = 1;
	u32 chr = 0;
	const char* lineStart = buffer_start;
	for (const char* b = buffer_start; b < location ; ++b)
	{
		if (*b == '\n')
		{
			lineStart = b+1;
			++line;
			chr = 0;
		}
		else if (*b == '\t')
			chr += 4;
		else
			++chr;
	}

	const char* lineEnd = lineStart;
	while (lineEnd < buffer_start + buffer_size && *lineEnd != '\n')
		++lineEnd;

	char buf[512];
	sprintf_s(buf, 512, "%s(%u,%u):\n%.*s \n", filename, line, chr, (u32)(lineEnd-lineStart),
		lineStart);
	std::string str = buf;
	if (chr + 2 < 512)
	{
		for (u32 i = 0 ; i < chr ; ++i)
			buf[i] = ' ';
		buf[chr] = '^';
		buf[chr+1] = '\0';
		str += buf;
	}
	return str;
}

void LoadRlf(State* s)
{
	s->RlfCompileSuccess = true;
	s->RlfCompileWarning = false;
	s->RlfValidationError = false;

	char* filename = s->Cfg.FilePath;

	std::string filePath(filename);
	std::string dirPath;
	size_t pos = filePath.find_last_of("/\\");
	if (pos != std::string::npos)
	{
		dirPath = filePath.substr(0, pos+1);
	}

	HANDLE rlf = fileio::OpenFileOptional(filename, GENERIC_READ);

	if (rlf == INVALID_HANDLE_VALUE) // file not found
	{
		ReportError(s, std::string("Couldn't find ") + filename);
		return;
	}

	s->RlfFileSize = fileio::GetFileSize(rlf);

	Assert(!s->RlfFile, "Leak");
	s->RlfFile = (char*)malloc(s->RlfFileSize);	
	Assert(s->RlfFile != nullptr, "failed to alloc");

	fileio::ReadFile(rlf, s->RlfFile, s->RlfFileSize);

	CloseHandle(rlf);

	Assert(s->CurrentRenderDesc == nullptr, "leaking data");
	rlf::ErrorState es = {};
	s->CurrentRenderDesc = rlf::ParseBuffer(s->RlfFile, s->RlfFileSize, dirPath.c_str(), &es);

	if (es.Success == false)
	{
		ReportError(s, std::string("Failed to parse RLF:\n") + es.Info.Message +
			"\n" + RlfFileLocation(s->RlfFile, s->RlfFileSize, filename, es.Info.Location));
		return;
	}

	es = {};
	rlf::ExecuteResources Res;
	Res.MainRtTex = &s->RlfDisplayTex;
	Res.MainRtv = s->RlfDisplayRtv;
	Res.MainRtUav = s->RlfDisplayUav;
	Res.DefaultDepthTex = &s->RlfDepthStencilTex;
	Res.DefaultDepthView = s->RlfDepthStencilView;

	rlf::InitD3D(s->GfxCtx, &Res, s->CurrentRenderDesc, s->DisplaySize, 
		dirPath.c_str(), &es);

	if (es.Success == false)
	{
		ReportError(s, std::string("Failed to create RLF scene:\n") +
			es.Info.Message + "\n" + RlfFileLocation(s->RlfFile, s->RlfFileSize, 
				filename, es.Info.Location));
		return;
	}

	s->RlfCompileWarning = es.Warning;
	s->RlfCompileWarningMessage = es.Info.Message;
}

void UnloadRlf(State* s)
{
	if (s->OnBeforeUnload)
		s->OnBeforeUnload(s);
	if (s->CurrentRenderDesc)
	{
		rlf::ReleaseD3D(s->GfxCtx, s->CurrentRenderDesc);
		rlf::ReleaseData(s->CurrentRenderDesc);
		s->CurrentRenderDesc = nullptr;
	}
	if (s->RlfFile)
	{
		free(s->RlfFile);
		s->RlfFile = nullptr;
	}
}

void ReportError(State* s, const std::string& message)
{
	s->RlfCompileSuccess = false;
	s->RlfCompileErrorMessage = message;
	UnloadRlf(s);
}

void Initialize(State* s, const char* config_path)
{
	*s = {};
	
	s->Cfg = { "", false, 0, 0, 1280, 800 };
	s->ConfigPath = config_path;

	config::LoadConfig(config_path, &s->Cfg);
}


void Shutdown(State* s)
{
	config::SaveConfig(s->ConfigPath.c_str(), &s->Cfg);

	UnloadRlf(s);
}

bool DoUpdate(State* s)
{
	bool Reload = s->FirstLoad || ImGui::IsKeyReleased(ImGuiKey_F5);
	bool Quit = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyReleased(ImGuiKey_Q);
	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyReleased(ImGuiKey_O))
		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
			"Choose File", ".rlf", ".");

	bool TuneablesChanged = false;
	bool ResetLayout = false;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Reload", "F5"))
				Reload = true;
			if (ImGui::MenuItem("Open", "Ctrl+O"))
				ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
					"Choose File", ".rlf", ".");
			ImGui::Separator();
			if (ImGui::MenuItem("Quit", "Ctrl+Q"))
				Quit = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Windows"))
		{
			ImGui::MenuItem("Event Viewer", "", &s->ShowEventsWindow);
			ImGui::MenuItem("Playback", "", &s->ShowPlaybackWindow);
			ImGui::MenuItem("Parameters", "", &s->ShowParametersWindow);
			ImGui::Separator();
			if (ImGui::MenuItem("Reset layout"))
				ResetLayout = true;
			ImGui::EndMenu();
		}
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		ImGui::Text("Loaded: %s", s->Cfg.FilePath);
		ImGui::EndMainMenuBar();
	}

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_PassthruCentralNode);

	if (s->Cfg.LayoutVersionApplied != LAYOUT_VERSION || ResetLayout)
	{
		// Clear out existing layout (includes children)
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

		ImGuiID dock_main_id = dockspace_id;
		ImGuiID dockspace_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, 
			ImGuiDir_Down, 0.20f, nullptr, &dock_main_id);
		ImGuiID dockspace_left_id = ImGui::DockBuilderSplitNode(dock_main_id, 
			ImGuiDir_Left, 0.15f, nullptr, &dock_main_id);
		ImGuiID dockspace_right_id = ImGui::DockBuilderSplitNode(dock_main_id,
			ImGuiDir_Right, 0.30f, nullptr, &dock_main_id);
		ImGuiID dockspace_right_top_id = ImGui::DockBuilderSplitNode(dockspace_right_id,
			ImGuiDir_Up, 0.10f, nullptr, &dockspace_right_id);

		ImGui::DockBuilderDockWindow("Display", dock_main_id);
		ImGui::DockBuilderDockWindow("Compile Output", dockspace_bottom_id);
		ImGui::DockBuilderDockWindow("Event Viewer", dockspace_left_id);
		ImGui::DockBuilderDockWindow("Playback", dockspace_right_top_id);
		ImGui::DockBuilderDockWindow("Parameters", dockspace_right_id);
		ImGui::DockBuilderFinish(dockspace_id);

		s->Cfg.LayoutVersionApplied = LAYOUT_VERSION;
	}

	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) 
	{
		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
			memcpy_s(s->Cfg.FilePath, sizeof(s->Cfg.FilePath), filePathName.c_str(), 
				filePathName.length());
			s->Cfg.FilePath[filePathName.length()] = '\0';
			Reload = true;
		}
		ImGuiFileDialog::Instance()->Close();
	}

	// if (s->ShowDemoWindow) ImGui::ShowDemoWindow(&showDemoWindow);
	if (Quit)
		return true;

	ImGui::Begin("Display", nullptr, ImGuiWindowFlags_NoCollapse);
	{
		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();
		s->DisplaySize.x = (u32)max(1, vMax.x - vMin.x);
		s->DisplaySize.y = (u32)max(1, vMax.y - vMin.y);

		ImTextureID display_tex = s->RetrieveDisplayTextureID(s);
		
		ImGui::Image(display_tex, ImVec2(vMax.x-vMin.x, vMax.y-vMin.y));
	}
	ImGui::End();

	if (ImGui::Begin("Compile Output"))
	{
		if (!s->RlfCompileSuccess)
		{
			ImVec4 color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::PushTextWrapPos(0.f);

			ImGui::TextUnformatted(s->RlfCompileErrorMessage.c_str());

			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
		}
		if (s->RlfCompileWarning)
		{
			ImVec4 color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::PushTextWrapPos(0.f);

			ImGui::TextUnformatted(s->RlfCompileWarningMessage.c_str());

			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
		}
		if (s->RlfValidationError)
		{
			ImVec4 color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::PushTextWrapPos(0.f);

			ImGui::TextUnformatted(s->RlfValidationErrorMessage.c_str());

			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();

	if (s->ShowEventsWindow)
	{
		if (ImGui::Begin("Event Viewer", &s->ShowEventsWindow))
		{
			if (s->RlfCompileSuccess)
			{
				gui::DisplayShaderPasses(s->CurrentRenderDesc);
			}
		}
		ImGui::End();
	}

	if (s->ShowPlaybackWindow)
	{
		if (ImGui::Begin("Playback", &s->ShowPlaybackWindow))
		{
			if (ImGui::Button("<<"))
				s->Speed = -2;
			ImGui::SameLine();
			if (ImGui::Button("<"))
				s->Speed = -1;
			ImGui::SameLine();
			if (ImGui::Button("||"))
				s->Speed = 0;
			ImGui::SameLine();
			if (ImGui::Button(">"))
				s->Speed = 1;
			ImGui::SameLine();
			if (ImGui::Button(">>"))
				s->Speed = 2;
			ImGui::Text("Time = %f", s->Time);
		}
		ImGui::End();
	}

	if (s->ShowParametersWindow)
	{
		if (ImGui::Begin("Parameters", &s->ShowParametersWindow))
		{
			ImGui::ColorEdit3("Clear color", (float*)&s->ClearColor);
			ImGui::Text("DisplaySize = %u / %u", s->DisplaySize.x, s->DisplaySize.y);
			if (s->RlfCompileSuccess)
			{
				ImGui::Text("Tuneables:");
				for (rlf::Tuneable* tune : s->CurrentRenderDesc->Tuneables)
				{
					bool ch = false;
					if (tune->Type == rlf::BoolType)
						ch = ImGui::Checkbox(tune->Name, &tune->Value.BoolVal);
					else if (tune->Type == rlf::FloatType)
						ch = ImGui::DragFloat(tune->Name, &tune->Value.FloatVal, 0.01f,
							tune->Min.FloatVal, tune->Max.FloatVal);
					else if (tune->Type == rlf::Float2Type)
						ch = ImGui::DragFloat2(tune->Name, (float*)&tune->Value.Float4Val.m, 
							0.01f, tune->Min.FloatVal, tune->Max.FloatVal);
					else if (tune->Type == rlf::Float3Type)
						ch = ImGui::DragFloat3(tune->Name, (float*)&tune->Value.Float4Val.m, 
							0.01f, tune->Min.FloatVal, tune->Max.FloatVal);
					else if (tune->Type == rlf::IntType)
						ch = ImGui::DragInt(tune->Name, &tune->Value.IntVal, 1.f, 
							tune->Min.IntVal, tune->Max.IntVal);
					else if (tune->Type == rlf::UintType)
					{
						i32 max = (tune->Min.UintVal == tune->Max.UintVal && 
							tune->Min.UintVal == 0) ? INT_MAX : tune->Max.UintVal;
						ch = ImGui::DragInt(tune->Name, (i32*)&tune->Value.UintVal,
							1, tune->Min.IntVal, max, "%d", ImGuiSliderFlags_AlwaysClamp);
					}
					else
						Unimplemented();
					TuneablesChanged |= ch;
				}
			}
		}
		ImGui::End();
	}

	u32 changed = 0;
	changed |= (s->DisplaySize != s->PrevDisplaySize) ? rlf::ast::VariesBy_DisplaySize : 0;
	changed |= TuneablesChanged ? rlf::ast::VariesBy_Tuneable : 0;
	changed |= s->LastTime != s->Time ? rlf::ast::VariesBy_Time : 0;
	s->ChangedThisFrameFlags = changed;

	u32 VariesByForTexture = rlf::ast::VariesBy_Tuneable | rlf::ast::VariesBy_DisplaySize;

	rlf::ExecuteContext ctx = {};
	ctx.GfxCtx = s->GfxCtx;
	ctx.Res.MainRtTex = &s->RlfDisplayTex;
	ctx.Res.MainRtv = s->RlfDisplayRtv;
	ctx.Res.MainRtUav = s->RlfDisplayUav;
	ctx.Res.DefaultDepthTex = &s->RlfDepthStencilTex;
	ctx.Res.DefaultDepthView = s->RlfDepthStencilView;
	ctx.EvCtx.DisplaySize = s->DisplaySize;
	ctx.EvCtx.Time = s->Time;
	ctx.EvCtx.ChangedThisFrameFlags = changed;

	if (s->RlfCompileSuccess && (changed & VariesByForTexture) != 0)
	{
		if (s->OnBeforeUnload)
			s->OnBeforeUnload(s);

		rlf::ErrorState ies = {};
		rlf::HandleTextureParametersChanged(s->CurrentRenderDesc, 
			&ctx, &ies);
		if (!ies.Success)
		{
			ReportError(s, std::string("Error resizing textures:\n") + ies.Info.Message + 
				"\n" + RlfFileLocation(s->RlfFile, s->RlfFileSize, 
					s->Cfg.FilePath, ies.Info.Location));
		}
	}

	if (Reload)
	{
		s->FirstLoad = false;
		s->Time = 0;
		UnloadRlf(s);
		LoadRlf(s);
	}

	return false;
}

void DoRender(State* s)
{
	if (s->RlfCompileSuccess)
	{
		rlf::ExecuteContext exctx = {};
		exctx.GfxCtx = s->GfxCtx;
		exctx.Res.MainRtTex = &s->RlfDisplayTex;
		exctx.Res.MainRtv = s->RlfDisplayRtv;
		exctx.Res.MainRtUav = s->RlfDisplayUav;
		exctx.Res.DefaultDepthTex = &s->RlfDepthStencilTex;
		exctx.Res.DefaultDepthView = s->RlfDepthStencilView;
		exctx.EvCtx.DisplaySize = s->DisplaySize;
		exctx.EvCtx.Time = s->Time;
		exctx.EvCtx.ChangedThisFrameFlags = s->ChangedThisFrameFlags;

		rlf::ErrorState es = {};
		rlf::Execute(&exctx, s->CurrentRenderDesc, &es);
		if (!es.Success)
		{
			ReportError(s, "RLF execution error: \n" + es.Info.Message +
				"\n" + RlfFileLocation(s->RlfFile, s->RlfFileSize, 
					s->Cfg.FilePath, es.Info.Location));
		}
	}
}

void PostFrame(State* s)
{
	s->LastTime = s->Time;
	s->Time = max(0, s->Time + s->Speed * ImGui::GetIO().DeltaTime);

	s->RlfValidationErrorMessage = "Validation error:\n";
	s->RlfValidationError = s->CheckD3DValidation(s->GfxCtx, s->RlfValidationErrorMessage);

	s->PrevDisplaySize = s->DisplaySize;
}


} // namespace main
