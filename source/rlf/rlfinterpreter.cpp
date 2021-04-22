
namespace rlf
{

void InitD3D(
	ID3D11Device* device,
	RenderDescription* rd,
	const char* workingDirectory,
	InitErrorState* errorState)
{
	std::string dirPath = workingDirectory;

	for (rlf::ComputeShader* cs : rd->Shaders)
	{
		std::string shaderPath = dirPath + cs->ShaderPath;

		HANDLE shader = fileio::OpenFileOptional(shaderPath.c_str(), GENERIC_READ);

		if (shader == INVALID_HANDLE_VALUE) // file not found
		{
			errorState->InitSuccess = false;
			errorState->ErrorMessage = std::string("Couldn't find shader file: ") + 
				shaderPath.c_str();
			return;
		}

		u32 shaderSize = fileio::GetFileSize(shader);

		char* shaderBuffer = (char*)malloc(shaderSize);
		Assert(shaderBuffer != nullptr, "failed to alloc");

		fileio::ReadFile(shader, shaderBuffer, shaderSize);

		CloseHandle(shader);

		ID3DBlob* shaderBlob;
		ID3DBlob* errorBlob;
		HRESULT hr = D3DCompile(shaderBuffer, shaderSize, shaderPath.c_str(), NULL,
			NULL, cs->EntryPoint, "cs_5_0", 0, 0, &shaderBlob, &errorBlob);
		Assert(hr == S_OK || hr == E_FAIL, "failed to compile shader hr=%x", hr);

		free(shaderBuffer);

		errorState->InitSuccess = (hr == S_OK);

		if (!errorState->InitSuccess)
		{
			Assert(errorBlob != nullptr, "no error info given");
			char* errorText = (char*)errorBlob->GetBufferPointer();
			errorState->ErrorMessage = std::string("Failed to compile shader:\n") +
				errorText;

			Assert(shaderBlob == nullptr, "leak");

			SafeRelease(errorBlob);

			return;
		}

		hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &cs->ShaderObject);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		ID3D11ShaderReflection* reflector = nullptr; 
		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
            IID_ID3D11ShaderReflection, (void**) &reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		reflector->GetThreadGroupSize(&cs->ThreadGroupSize.x, &cs->ThreadGroupSize.y,
			&cs->ThreadGroupSize.z);

		SafeRelease(reflector);
		SafeRelease(shaderBlob);
		SafeRelease(errorBlob);
	}
}

void Execute(
	ID3D11DeviceContext* ctx,
	RenderDescription* rd,
	float time)
{
	ImGuiIO& io = ImGui::GetIO();

	// D3D11 on PC doesn't like same resource being bound as RT and UAV simultaneously.
	//	Swap to UAV for compute shader. 
	ID3D11RenderTargetView* emptyRT = nullptr;
	ctx->OMSetRenderTargets(1, &emptyRT, nullptr);

	// TODO: duplicated definition
	struct ConstantBuffer
	{
		float DisplaySizeX, DisplaySizeY;
		float Time;
		float Padding;
	};

	// Update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		HRESULT hr = ctx->Map(g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 
			0, &mapped_resource);
		Assert(hr == S_OK, "failed to map CB hr=%x", hr);
		ConstantBuffer* cb = (ConstantBuffer*)mapped_resource.pData;
		cb->DisplaySizeX = io.DisplaySize.x;
		cb->DisplaySizeY = io.DisplaySize.y;
		cb->Time = time;
		ctx->Unmap(g_pConstantBuffer, 0);
	}

	UINT initialCount = (UINT)-1;
	for (rlf::Dispatch* dc : rd->Passes)
	{
		ctx->CSSetShader(dc->Shader->ShaderObject, nullptr, 0);
		ctx->CSSetConstantBuffers(0, 1, &g_pConstantBuffer);
		ctx->CSSetUnorderedAccessViews(0, 1, &g_mainRenderTargetUav, 
			&initialCount);
		uint3 groups = dc->Groups;
		if (dc->ThreadPerPixel)
		{
			uint3 tgs = dc->Shader->ThreadGroupSize;
			groups.x = (u32)((io.DisplaySize.x - 1) / tgs.x) + 1;
			groups.y = (u32)((io.DisplaySize.y - 1) / tgs.y) + 1;
			groups.z = 1;
		}

		ctx->Dispatch(groups.x, groups.y, groups.z);
	}

	// D3D11 on PC doesn't like same resource being bound as RT and UAV simultaneously.
	//	Swap back to RT for drawing. 
	ID3D11UnorderedAccessView* emptyUAV = nullptr;
	ctx->CSSetUnorderedAccessViews(0, 1, &emptyUAV, &initialCount);
	ctx->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
}

} // namespace rlf