
namespace main {

	struct State {
		char* RlfFile = nullptr;
		u32 RlfFileSize = 0;
		bool RlfCompileSuccess = false;
		std::string RlfCompileErrorMessage;
		bool RlfCompileWarning = false;
		std::string RlfCompileWarningMessage;
		bool RlfValidationError = false;
		std::string RlfValidationErrorMessage;

		std::string ConfigPath;
		config::Parameters Cfg = { "", false, 0, 0, 1280, 800 };

		bool StartupComplete = false;

		uint2 DisplaySize;
		uint2 PrevDisplaySize;

		rlf::RenderDescription* CurrentRenderDesc;

		ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		float Time = 0;
		float LastTime = -1;
		float Speed = 1;
		bool FirstLoad = true;
		// bool ShowDemoWindow = true;
		bool ShowPlaybackWindow = true;
		bool ShowParametersWindow = true;
		bool ShowEventsWindow = true;

		u32 ChangedThisFrameFlags = 0;

		ImTextureID (*RetrieveDisplayTextureID)(State*);
		bool (*CheckD3DValidation)(gfx::Context* ctx, std::string& outMessage);
		void (*OnBeforeUnload)(State*);

		gfx::Context* GfxCtx;

		gfx::Texture				RlfDisplayTex;
		gfx::RenderTargetView		RlfDisplayRtv;
		gfx::ShaderResourceView		RlfDisplaySrv;
		gfx::UnorderedAccessView	RlfDisplayUav;
		gfx::Texture				RlfDepthStencilTex;
		gfx::DepthStencilView		RlfDepthStencilView;
	};

	void Initialize(State* s, const char* config_path);
	void Shutdown(State* s);

	bool DoUpdate(State* s);
	void DoRender(State* s);
	void PostFrame(State* s);

}