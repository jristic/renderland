
namespace rlf
{

D3D11_FILTER RlfToD3d(FilterMode fm)
{
	if (fm.Min == Filter::Aniso || fm.Mag == Filter::Aniso ||
		fm.Mip == Filter::Aniso)
	{
		return D3D11_FILTER_ANISOTROPIC;
	}

	static D3D11_FILTER filters[] = {
		D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
        D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
        D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
        D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
    };

	u32 i = 0;
	if (fm.Min == Filter::Linear)
		i += 4;
	if (fm.Mag == Filter::Linear)
		i += 2;
	if (fm.Mip == Filter::Linear)
		i += 1;
	
	return filters[i];
}

D3D11_TEXTURE_ADDRESS_MODE RlfToD3d(AddressMode m)
{
	static D3D11_TEXTURE_ADDRESS_MODE modes[] = {
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_MIRROR,
		D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_BORDER,
	};
	return modes[(u32)m];
}

void InitD3D(
	ID3D11Device* device,
	RenderDescription* rd,
	const char* workingDirectory,
	InitErrorState* errorState)
{
	std::string dirPath = workingDirectory;

	for (ComputeShader* cs : rd->Shaders)
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
			D3D_COMPILE_STANDARD_FILE_INCLUDE, cs->EntryPoint, "cs_5_0", D3DCOMPILE_DEBUG, 
			0, &shaderBlob, &errorBlob);
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

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
            IID_ID3D11ShaderReflection, (void**) &cs->Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		cs->Reflector->GetThreadGroupSize(&cs->ThreadGroupSize.x, &cs->ThreadGroupSize.y,
			&cs->ThreadGroupSize.z);

		SafeRelease(shaderBlob);
		SafeRelease(errorBlob);
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		ID3D11ShaderReflection* reflector = dc->Shader->Reflector;
		for (Bind& bind : dc->Binds)
		{
			D3D11_SHADER_INPUT_BIND_DESC desc;
			HRESULT hr = reflector->GetResourceBindingDescByName(bind.BindTarget, 
				&desc);
			Assert(hr == S_OK || hr == E_INVALIDARG, "unhandled error, hr=%x", hr);
			if (hr == E_INVALIDARG)
			{
				errorState->InitSuccess = false;
				errorState->ErrorMessage = std::string("Could not find resource ") +
					bind.BindTarget + " in shader " + dc->Shader->ShaderPath;
				return;
			}
			bind.BindIndex = desc.BindPoint;
			switch (desc.Type)
			{
			case D3D_SIT_TEXTURE:
			case D3D_SIT_SAMPLER:
			case D3D_SIT_STRUCTURED:
				bind.IsOutput = false;
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
				bind.IsOutput = true;
				break;
			case D3D_SIT_BYTEADDRESS:
			case D3D_SIT_UAV_RWBYTEADDRESS:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_CONSUME_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER:
			default:
				Assert(false, "Unhandled type %d", desc.Type);
				break;
			}
		}
	}

	for (Buffer* buf : rd->Buffers)
	{
		u32 bufSize = buf->ElementSize * buf->ElementCount;

		D3D11_SUBRESOURCE_DATA initData = {};
		void* zeroMem = nullptr;
		if (buf->InitToZero)
		{
			initData.pSysMem = zeroMem = malloc(bufSize);
			ZeroMemory(zeroMem, bufSize);
		}

		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = bufSize;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = buf->ElementSize;

		HRESULT hr = device->CreateBuffer(&desc, buf->InitToZero ? &initData : 0, 
			&buf->BufferObject);
		Assert(hr == S_OK, "failed to create buffer, hr=%x", hr);

		hr = device->CreateShaderResourceView(buf->BufferObject, nullptr, &buf->SRV);
		Assert(hr == S_OK, "failed to create SRV, hr=%x", hr);

		hr = device->CreateUnorderedAccessView(buf->BufferObject, nullptr, &buf->UAV);
		Assert(hr == S_OK, "failed to create UAV, hr=%x", hr);

		if (buf->InitToZero)
			free(zeroMem);
	}

	for (Texture* tex : rd->Textures)
	{
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = tex->Size.x;
		desc.Height = tex->Size.y;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = D3DTextureFormat[(u32)tex->Format];
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		HRESULT hr = device->CreateTexture2D(&desc, nullptr, &tex->TextureObject);
		Assert(hr == S_OK, "failed to create texture, hr=%x", hr);

		hr = device->CreateShaderResourceView(tex->TextureObject, nullptr, &tex->SRV);
		Assert(hr == S_OK, "failed to create SRV, hr=%x", hr);

		hr = device->CreateUnorderedAccessView(tex->TextureObject, nullptr, &tex->UAV);
		Assert(hr == S_OK, "failed to create UAV, hr=%x", hr);
	}

	for (Sampler* s : rd->Samplers)
	{
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter = RlfToD3d(s->Filter);
		desc.AddressU = RlfToD3d(s->Address.U);
		desc.AddressV = RlfToD3d(s->Address.V);
		desc.AddressW = RlfToD3d(s->Address.W);
		desc.MipLODBias = s->MipLODBias;
		desc.MaxAnisotropy = s->MaxAnisotropy;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.BorderColor[0] = s->BorderColor.x;
		desc.BorderColor[1] = s->BorderColor.y;
		desc.BorderColor[2] = s->BorderColor.z;
		desc.BorderColor[3] = s->BorderColor.w;
		desc.MinLOD = s->MinLOD;
		desc.MaxLOD = s->MaxLOD;

		HRESULT hr = device->CreateSamplerState(&desc, &s->SamplerObject);
		Assert(hr == S_OK, "failed to create sampler, hr=%x", hr);
	}
}

void ReleaseD3D(
	RenderDescription* rd)
{
	for (ComputeShader* cs : rd->Shaders)
	{
		SafeRelease(cs->ShaderObject);
		SafeRelease(cs->Reflector);
	}

	for (Buffer* buf : rd->Buffers)
	{
		SafeRelease(buf->UAV);
		SafeRelease(buf->SRV);
		SafeRelease(buf->BufferObject);
	}

	for (Texture* tex : rd->Textures)
	{
		SafeRelease(tex->UAV);
		SafeRelease(tex->SRV);
		SafeRelease(tex->TextureObject);
	}

	for (Sampler* s : rd->Samplers)
	{
		SafeRelease(s->SamplerObject);
	}
}

void Execute(
	ExecuteContext* context,
	RenderDescription* rd)
{
	ID3D11DeviceContext* ctx = context->D3dCtx;
	float time = context->Time;

	ImGuiIO& io = ImGui::GetIO();

	// Clear state so we aren't polluted by previous program drawing or previous 
	//	execution. 
	ctx->ClearState();

	// TODO: duplicated definition
	struct ConstantBuffer
	{
		float DisplaySizeX, DisplaySizeY;
		float Time;
		float Padding;
	};

	// Update constant buffer
	ID3D11Buffer* constBuffer = context->GlobalConstantBuffer;
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		HRESULT hr = ctx->Map(constBuffer, 0, D3D11_MAP_WRITE_DISCARD, 
			0, &mapped_resource);
		Assert(hr == S_OK, "failed to map CB hr=%x", hr);
		ConstantBuffer* cb = (ConstantBuffer*)mapped_resource.pData;
		cb->DisplaySizeX = io.DisplaySize.x;
		cb->DisplaySizeY = io.DisplaySize.y;
		cb->Time = time;
		ctx->Unmap(constBuffer, 0);
	}

	UINT initialCount = (UINT)-1;
	for (Dispatch* dc : rd->Passes)
	{
		ctx->CSSetShader(dc->Shader->ShaderObject, nullptr, 0);
		ctx->CSSetConstantBuffers(0, 1, &constBuffer);
		for (Bind& bind : dc->Binds)
		{
			switch (bind.Type)
			{
			case BindType::SystemValue:
				if (bind.SystemBind == SystemValue::BackBuffer)
					ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &context->MainRtUav, 
						&initialCount);
				else
					Assert(false, "unhandled");
				break;
			case BindType::Buffer:
				if (bind.IsOutput)
				{
					ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &bind.BufferBind->UAV, 
						&initialCount);
				}
				else
				{
					ctx->CSSetShaderResources(bind.BindIndex, 1, &bind.BufferBind->SRV);
				}
				break;
			case BindType::Texture:
				if (bind.IsOutput)
				{
					ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &bind.TextureBind->UAV, 
						&initialCount);
				}
				else
				{
					ctx->CSSetShaderResources(bind.BindIndex, 1, &bind.TextureBind->SRV);
				}
				break;
			case BindType::Sampler:
				ctx->CSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->SamplerObject);
				break;
			default:
				Assert(false, "invalid type %d", bind.Type);
			}
		}
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

	// Clear state after execution so we don't pollute the rest of program drawing. 
	ctx->ClearState();
}

} // namespace rlf
