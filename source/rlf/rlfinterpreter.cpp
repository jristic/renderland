
namespace rlf
{

static ID3D11RasterizerState*   DefaultRasterizerState = nullptr;

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
	Assert(m != AddressMode::Invalid, "Invalid");
	static D3D11_TEXTURE_ADDRESS_MODE modes[] = {
		(D3D11_TEXTURE_ADDRESS_MODE)0, //invalid
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_MIRROR,
		D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_BORDER,
	};
	return modes[(u32)m];
}

D3D11_PRIMITIVE_TOPOLOGY RlfToD3d(Topology topo)
{
	Assert(topo != Topology::Invalid, "Invalid");
	static D3D11_PRIMITIVE_TOPOLOGY topos[] = {
		(D3D11_PRIMITIVE_TOPOLOGY)0, //invalid
		D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
		D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	};
	return topos[(u32)topo];
}

D3D11_CULL_MODE RlfToD3d(CullMode cm)
{
	Assert(cm != CullMode::Invalid, "Invalid");
	static D3D11_CULL_MODE cms[] = {
		(D3D11_CULL_MODE)0, //invalid
		D3D11_CULL_NONE,
		D3D11_CULL_FRONT,
		D3D11_CULL_BACK,
	};
	return cms[(u32)cm];
}

u32 RlfToD3d(BufferFlag flags)
{
	u32 f = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	if (flags & BufferFlag_Vertex)
		f |= D3D11_BIND_VERTEX_BUFFER;
	if (flags & BufferFlag_Index)
		f |= D3D11_BIND_INDEX_BUFFER;
	return f;
}

u32 RlfToD3d(TextureFlag flags)
{
	u32 f = 0;
	if (flags & TextureFlag_SRV)
		f |= D3D11_BIND_SHADER_RESOURCE; 
	if (flags & TextureFlag_UAV)
		f |= D3D11_BIND_UNORDERED_ACCESS;
	if (flags & TextureFlag_RTV)
		f |= D3D11_BIND_RENDER_TARGET;
	if (flags & TextureFlag_DSV)
		f |= D3D11_BIND_DEPTH_STENCIL;
	return f;
}

D3D11_COMPARISON_FUNC RlfToD3d(ComparisonFunc cf)
{
	Assert(cf != ComparisonFunc::Invalid, "Invalid");
	static D3D11_COMPARISON_FUNC cfs[] = {
		(D3D11_COMPARISON_FUNC)0, //invalid
		D3D11_COMPARISON_NEVER,
		D3D11_COMPARISON_LESS,
		D3D11_COMPARISON_EQUAL,
		D3D11_COMPARISON_LESS_EQUAL,
		D3D11_COMPARISON_GREATER,
		D3D11_COMPARISON_NOT_EQUAL,
		D3D11_COMPARISON_GREATER_EQUAL,
		D3D11_COMPARISON_ALWAYS,
	};
	return cfs[(u32)cf];
}

D3D11_STENCIL_OP RlfToD3d(StencilOp so)
{
	Assert(so != StencilOp::Invalid, "Invalid");
	static D3D11_STENCIL_OP sos[] = {
		(D3D11_STENCIL_OP)0, //invalid
		D3D11_STENCIL_OP_KEEP,
		D3D11_STENCIL_OP_ZERO,
		D3D11_STENCIL_OP_REPLACE,
		D3D11_STENCIL_OP_INCR_SAT,
		D3D11_STENCIL_OP_DECR_SAT,
		D3D11_STENCIL_OP_INVERT,
		D3D11_STENCIL_OP_INCR,
		D3D11_STENCIL_OP_DECR
	};
	return sos[(u32)so];
}

D3D11_DEPTH_STENCILOP_DESC RlfToD3d(StencilOpDesc sod)
{
	D3D11_DEPTH_STENCILOP_DESC desc = {};
	desc.StencilFailOp = RlfToD3d(sod.StencilFailOp);
	desc.StencilDepthFailOp = RlfToD3d(sod.StencilDepthFailOp);
	desc.StencilPassOp = RlfToD3d(sod.StencilPassOp);
	desc.StencilFunc = RlfToD3d(sod.StencilFunc);
	return desc;
}

ID3DBlob* CommonCompileShader(const char* path, const char* profile, 
	const char * entry, InitErrorState* errorState)
{
	HANDLE shader = fileio::OpenFileOptional(path, GENERIC_READ);

	if (shader == INVALID_HANDLE_VALUE) // file not found
	{
		errorState->InitSuccess = false;
		errorState->ErrorMessage = std::string("Couldn't find shader file: ") + 
			path;
		return nullptr;
	}

	u32 shaderSize = fileio::GetFileSize(shader);

	char* shaderBuffer = (char*)malloc(shaderSize);
	Assert(shaderBuffer != nullptr, "failed to alloc");

	fileio::ReadFile(shader, shaderBuffer, shaderSize);

	CloseHandle(shader);

	ID3DBlob* shaderBlob;
	ID3DBlob* errorBlob;
	HRESULT hr = D3DCompile(shaderBuffer, shaderSize, path, NULL,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, profile, D3DCOMPILE_DEBUG, 
		0, &shaderBlob, &errorBlob);

	free(shaderBuffer);

	errorState->InitSuccess = (hr == S_OK);

	if (!errorState->InitSuccess)
	{
		Assert(errorBlob != nullptr, "No error info given for shader compile fail.");
		char* errorText = (char*)errorBlob->GetBufferPointer();
		errorState->ErrorMessage = std::string("Failed to compile shader:\n") +
			errorText;

		Assert(shaderBlob == nullptr, "leak");

		SafeRelease(errorBlob);

		return nullptr;
	}

	// check for warnings
	if (errorBlob)
	{
		errorState->InitWarning = true;
		char* errorText = (char*)errorBlob->GetBufferPointer();
		errorState->ErrorMessage += errorText;
	}

	SafeRelease(errorBlob);
	return shaderBlob;
}

void CreateInputLayout(ID3D11Device* device, VertexShader* shader, ID3DBlob* blob)
{
	ID3D11ShaderReflection* reflector = shader->Reflector;

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	reflector->GetDesc( &shaderDesc );

	// Read input layout description from shader info
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for ( u32 i = 0 ; i < shaderDesc.InputParameters ; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		reflector->GetInputParameterDesc(i, &paramDesc );

		if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED)
			continue;

		// fill out input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;   

		// determine DXGI format
		if ( paramDesc.Mask == 1 )
		{
			if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
				elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
				elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
				elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if ( paramDesc.Mask <= 3 )
		{
			if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if ( paramDesc.Mask <= 7 )
		{
			if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if ( paramDesc.Mask <= 15 )
		{
			if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		//save element desc
		inputLayoutDesc.push_back(elementDesc);
	}

	if (inputLayoutDesc.size())
	{
		// Try to create Input Layout
		HRESULT hr = device->CreateInputLayout(
			&inputLayoutDesc[0], (u32)inputLayoutDesc.size(), blob->GetBufferPointer(), 
			blob->GetBufferSize(), &shader->InputLayout);
		Assert(hr == S_OK, "failed to create input layout, hr=%x", hr);
	}
}

void ResolveBind(Bind& bind, ID3D11ShaderReflection* reflector, const char* path,
	InitErrorState* errorState)
{
	D3D11_SHADER_INPUT_BIND_DESC desc;
	HRESULT hr = reflector->GetResourceBindingDescByName(bind.BindTarget, 
		&desc);
	Assert(hr == S_OK || hr == E_INVALIDARG, "unhandled error, hr=%x", hr);
	if (hr == E_INVALIDARG)
	{
		errorState->InitSuccess = false;
		errorState->ErrorMessage = std::string("Could not find resource ") +
			bind.BindTarget + " in shader " + path;
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
	}
}

void PrepareConstants(
	ID3D11ShaderReflection* reflector, std::vector<ConstantBuffer>& buffers, 
	std::vector<SetConstant>& sets, const char* path, InitErrorState* errorState)
{
	D3D11_SHADER_DESC sd;
	reflector->GetDesc(&sd);
	for (u32 i = 0 ; i < sd.ConstantBuffers ; ++i)
	{
		ConstantBuffer cb = {};
		D3D11_SHADER_BUFFER_DESC bd;
		ID3D11ShaderReflectionConstantBuffer* constBuffer = 
			reflector->GetConstantBufferByIndex(i);
		constBuffer->GetDesc(&bd);

		if (bd.Type != D3D11_CT_CBUFFER)
			continue;

		cb.Name = bd.Name;
		cb.Size = bd.Size;
		cb.BackingMemory = (u8*)malloc(bd.Size);

		for (u32 j = 0 ; j < bd.Variables ; ++j)
		{
			ID3D11ShaderReflectionVariable* var = constBuffer->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC vd;
			var->GetDesc(&vd);

			if (vd.DefaultValue)
				memcpy(cb.BackingMemory+vd.StartOffset, vd.DefaultValue, vd.Size);
			else
				ZeroMemory(cb.BackingMemory+vd.StartOffset, vd.Size);
		}

		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = bd.Size;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = cb.BackingMemory;
		HRESULT hr = g_pd3dDevice->CreateBuffer(&desc, &srd, &cb.BufferObject);
		Assert(hr == S_OK, "Failed to create CB hr=%x", hr);

		for (u32 k = 0 ; k < sd.BoundResources ; ++k)
		{
			D3D11_SHADER_INPUT_BIND_DESC id; 
			hr = reflector->GetResourceBindingDesc(k, &id);
			Assert(hr == S_OK, "Failed to get desc, hr=%x", hr);
			if (strcmp(id.Name, bd.Name) == 0)
			{
				cb.Slot = id.BindPoint;
				break;
			}
		}

		buffers.push_back(cb);
	}

	for (SetConstant& set : sets)
	{
		ID3D11ShaderReflectionVariable* cvar = nullptr;
		const char* cbufname = nullptr;
		for (u32 i = 0 ; i < sd.ConstantBuffers ; ++i)
		{
			D3D11_SHADER_BUFFER_DESC bd;
			ID3D11ShaderReflectionConstantBuffer* buffer = 
				reflector->GetConstantBufferByIndex(i);
			buffer->GetDesc(&bd);
			if (bd.Type != D3D11_CT_CBUFFER)
				continue;
			for (u32 j = 0 ; j < bd.Variables ; ++j)
			{
				ID3D11ShaderReflectionVariable* var = buffer->GetVariableByIndex(j);
				D3D11_SHADER_VARIABLE_DESC vd;
				var->GetDesc(&vd);
				if (strcmp(set.VariableName, vd.Name) == 0)
				{
					cvar = var;
					cbufname = bd.Name;
					break;
				}
			}
		}
		if (!cvar)
		{
			errorState->InitSuccess = false;
			errorState->ErrorMessage = std::string("Could not find constant ") +
				set.VariableName + " in shader " + path;
			return;
		} 
		D3D11_SHADER_VARIABLE_DESC vd;
		cvar->GetDesc(&vd);
		set.Offset = vd.StartOffset;
		set.Size = vd.Size;
		bool found = false;
		for (ConstantBuffer& con : buffers)
		{
			if (con.Name == cbufname)
			{
				set.CB = &con;
				found = true;
				break;
			}
		}
		Assert(found, "Failed to find cbuffer resource binding");

		ID3D11ShaderReflectionType* ctype = cvar->GetType();
		D3D11_SHADER_TYPE_DESC td;
		ctype->GetDesc(&td);
		switch (td.Type)
		{
		case D3D_SVT_BOOL:
			Assert(td.Rows == 1, "Unhandled bool vector");
			set.Type.Fmt = VariableFormat::Bool;
			set.Type.Dim = td.Columns;
			break;
		case D3D_SVT_INT:
			Assert(td.Rows == 1, "Unhandled int vector");
			set.Type.Fmt = VariableFormat::Int;
			set.Type.Dim = td.Columns;
			break;
		case D3D_SVT_UINT:
			Assert(td.Rows == 1, "Unhandled uint vector");
			set.Type.Fmt = VariableFormat::Uint;
			set.Type.Dim = td.Columns;
			break;
		case D3D_SVT_FLOAT:
			if (td.Rows == 4 && td.Columns == 4)
			{
				set.Type = Float4x4Type;
			}
			else
			{
				Assert(td.Rows == 1, "Unhandled float vector");
				set.Type.Fmt = VariableFormat::Float;
				set.Type.Dim = td.Columns;
			}
			break;
		default:
			Unimplemented();
		}
	}
}

void InitD3D(
	ID3D11Device* device,
	RenderDescription* rd,
	const char* workingDirectory,
	InitErrorState* errorState)
{
	if (DefaultRasterizerState == nullptr)
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		device->CreateRasterizerState(&desc, &DefaultRasterizerState);
	}
	
	std::string dirPath = workingDirectory;
	errorState->InitSuccess = true;

	for (ComputeShader* cs : rd->CShaders)
	{
		std::string shaderPath = dirPath + cs->ShaderPath;
		ID3DBlob* shaderBlob = CommonCompileShader(shaderPath.c_str(), "cs_5_0", 
			cs->EntryPoint, errorState);

		if (errorState->InitSuccess == false)
		{
			Assert(shaderBlob == nullptr, "leak");
			return;
		}

		HRESULT hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &cs->ShaderObject);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &cs->Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		cs->Reflector->GetThreadGroupSize(&cs->ThreadGroupSize.x, &cs->ThreadGroupSize.y,
			&cs->ThreadGroupSize.z);

		SafeRelease(shaderBlob);
	}

	for (VertexShader* vs : rd->VShaders)
	{
		std::string shaderPath = dirPath + vs->ShaderPath;
		ID3DBlob* shaderBlob = CommonCompileShader(shaderPath.c_str(), "vs_5_0", 
			vs->EntryPoint, errorState);

		if (errorState->InitSuccess == false)
		{
			Assert(shaderBlob == nullptr, "leak");
			return;
		}

		HRESULT hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &vs->ShaderObject);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &vs->Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		CreateInputLayout(device, vs, shaderBlob);

		SafeRelease(shaderBlob);
	}

	for (PixelShader* ps : rd->PShaders)
	{
		std::string shaderPath = dirPath + ps->ShaderPath;
		ID3DBlob* shaderBlob = CommonCompileShader(shaderPath.c_str(), "ps_5_0", 
			ps->EntryPoint, errorState);

		if (errorState->InitSuccess == false)
		{
			Assert(shaderBlob == nullptr, "leak");
			return;
		}

		HRESULT hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &ps->ShaderObject);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &ps->Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		SafeRelease(shaderBlob);
	}


	for (Dispatch* dc : rd->Dispatches)
	{
		ID3D11ShaderReflection* reflector = dc->Shader->Reflector;
		for (Bind& bind : dc->Binds)
		{
			ResolveBind(bind, reflector, dc->Shader->ShaderPath, errorState);
			if (errorState->InitSuccess == false)
				return;
		}
		PrepareConstants(reflector, dc->CBs, dc->Constants, dc->Shader->ShaderPath, 
			errorState);
		if (errorState->InitSuccess == false)
			return;
	}

	for (Draw* draw : rd->Draws)
	{
		if (!draw->VShader)
		{
			errorState->InitSuccess = false;
			errorState->ErrorMessage = "Null vertex shader on draw not permitted.";
			return;
		}
		ID3D11ShaderReflection* reflector = draw->VShader->Reflector;
		for (Bind& bind : draw->VSBinds)
		{
			ResolveBind(bind, reflector, draw->VShader->ShaderPath, errorState);
			if (errorState->InitSuccess == false)
				return;
		}
		PrepareConstants(reflector, draw->VSCBs, draw->VSConstants,
			draw->VShader->ShaderPath, errorState);
		if (errorState->InitSuccess == false)
			return;
		if (draw->PShader)
		{
			reflector = draw->PShader->Reflector;
			for (Bind& bind : draw->PSBinds)
			{
				ResolveBind(bind, reflector, draw->PShader->ShaderPath, errorState);
				if (errorState->InitSuccess == false)
					return;
			}
			PrepareConstants(reflector, draw->PSCBs, draw->PSConstants,
				draw->PShader->ShaderPath, errorState);
			if (errorState->InitSuccess == false)
				return;
		}
	}

	for (Buffer* buf : rd->Buffers)
	{
		u32 bufSize = buf->ElementSize * buf->ElementCount;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = buf->InitData;

		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = bufSize;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = RlfToD3d(buf->Flags);
		desc.CPUAccessFlags = 0;
		desc.StructureByteStride = buf->ElementSize;

		const u32 vif = BufferFlag_Vertex | BufferFlag_Index;
		desc.MiscFlags = buf->Flags ? 0 : D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.MiscFlags |= (buf->Flags & BufferFlag_IndirectArgs) ? 
			D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS : 0;

		HRESULT hr = device->CreateBuffer(&desc, initData.pSysMem ? &initData : nullptr,
			&buf->BufferObject);
		Assert(hr == S_OK, "failed to create buffer, hr=%x", hr);

		if (buf->Flags == 0)
		{
			hr = device->CreateShaderResourceView(buf->BufferObject, nullptr, &buf->SRV);
			Assert(hr == S_OK, "failed to create SRV, hr=%x", hr);

			hr = device->CreateUnorderedAccessView(buf->BufferObject, nullptr, &buf->UAV);
			Assert(hr == S_OK, "failed to create UAV, hr=%x", hr);
		}
	}

	for (Texture* tex : rd->Textures)
	{
		if (tex->DDSPath)
		{
			std::string ddsPath = dirPath + tex->DDSPath;
			HANDLE dds = fileio::OpenFileOptional(ddsPath.c_str(), GENERIC_READ);

			if (dds == INVALID_HANDLE_VALUE) // file not found
			{
				errorState->InitSuccess = false;
				errorState->ErrorMessage = std::string("Couldn't find dds file: ") + 
					ddsPath;
				return;
			}

			u32 ddsSize = fileio::GetFileSize(dds);
			char* ddsBuffer = (char*)malloc(ddsSize);
			Assert(ddsBuffer != nullptr, "failed to alloc");

			fileio::ReadFile(dds, ddsBuffer, ddsSize);

			CloseHandle(dds);

			DirectX::TexMetadata meta;
			DirectX::ScratchImage scratch;
			DirectX::LoadFromDDSMemory(ddsBuffer, ddsSize, DirectX::DDS_FLAGS_NONE, 
				&meta, scratch);
			HRESULT hr = DirectX::CreateShaderResourceViewEx(device, scratch.GetImages(),
				scratch.GetImageCount(), meta, D3D11_USAGE_IMMUTABLE, 
				D3D11_BIND_SHADER_RESOURCE, 0, 0, false, &tex->SRV);
			Assert(hr == S_OK, "Failed to create SRV, hr=%x", hr);
		}
		else
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
			desc.BindFlags = RlfToD3d(tex->Flags);
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			HRESULT hr = device->CreateTexture2D(&desc, nullptr, &tex->TextureObject);
			Assert(hr == S_OK, "failed to create texture, hr=%x", hr);

			if (tex->Flags & TextureFlag_SRV)
			{
				hr = device->CreateShaderResourceView(tex->TextureObject, nullptr, &tex->SRV);
				Assert(hr == S_OK, "failed to create SRV, hr=%x", hr);
			}
			if (tex->Flags & TextureFlag_UAV)
			{
				hr = device->CreateUnorderedAccessView(tex->TextureObject, nullptr, &tex->UAV);
				Assert(hr == S_OK, "failed to create UAV, hr=%x", hr);
			}
			if (tex->Flags & TextureFlag_RTV)
			{
				hr = device->CreateRenderTargetView(tex->TextureObject, nullptr, &tex->RTV);
				Assert(hr == S_OK, "failed to create RTV, hr=%x", hr);
			}
			if (tex->Flags & TextureFlag_DSV)
			{
				hr = device->CreateDepthStencilView(tex->TextureObject, nullptr, &tex->DSV);
				Assert(hr == S_OK, "failed to create DSV, hr=%x", hr);
			}
		}
	}

	for (Sampler* s : rd->Samplers)
	{
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter = RlfToD3d(s->Filter);
		desc.AddressU = RlfToD3d(s->AddressMode.U);
		desc.AddressV = RlfToD3d(s->AddressMode.V);
		desc.AddressW = RlfToD3d(s->AddressMode.W);
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

	for (RasterizerState* rs : rd->RasterizerStates)
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = rs->Fill ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
		desc.CullMode = RlfToD3d(rs->CullMode);
		desc.FrontCounterClockwise = rs->FrontCCW;
		desc.DepthBias = rs->DepthBias;
		desc.SlopeScaledDepthBias = rs->SlopeScaledDepthBias;
		desc.DepthBiasClamp = rs->DepthBiasClamp;
		desc.DepthClipEnable = rs->DepthClipEnable;
		desc.ScissorEnable = rs->ScissorEnable;
		device->CreateRasterizerState(&desc, &rs->RSObject);
	}

	for (DepthStencilState* dss : rd->DepthStencilStates)
	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = dss->DepthEnable;
		desc.DepthWriteMask = dss->DepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : 
			D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = RlfToD3d(dss->DepthFunc);
		desc.StencilEnable = dss->StencilEnable;
		desc.StencilReadMask = dss->StencilReadMask;
		desc.StencilWriteMask = dss->StencilWriteMask;
		desc.FrontFace = RlfToD3d(dss->FrontFace);
		desc.BackFace = RlfToD3d(dss->BackFace);
		device->CreateDepthStencilState(&desc, &dss->DSSObject);
	}
}

void ReleaseD3D(
	RenderDescription* rd)
{
	for (ComputeShader* cs : rd->CShaders)
	{
		SafeRelease(cs->ShaderObject);
		SafeRelease(cs->Reflector);
	}
	for (VertexShader* vs : rd->VShaders)
	{
		SafeRelease(vs->ShaderObject);
		SafeRelease(vs->Reflector);
		SafeRelease(vs->InputLayout);
	}
	for (PixelShader* ps : rd->PShaders)
	{
		SafeRelease(ps->ShaderObject);
		SafeRelease(ps->Reflector);
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
		SafeRelease(tex->RTV);
		SafeRelease(tex->DSV);
		SafeRelease(tex->TextureObject);
	}

	for (Sampler* s : rd->Samplers)
	{
		SafeRelease(s->SamplerObject);
	}

	for (RasterizerState* rs : rd->RasterizerStates)
	{
		SafeRelease(rs->RSObject);
	}
	for (DepthStencilState* dss : rd->DepthStencilStates)
	{
		SafeRelease(dss->DSSObject);
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		for (ConstantBuffer& cb : dc->CBs)
		{
			SafeRelease(cb.BufferObject);
			free(cb.BackingMemory);
		}
	}
}


// -----------------------------------------------------------------------------
// ------------------------------ EXECUTION ------------------------------------
// -----------------------------------------------------------------------------
struct ExecuteException
{
	ErrorInfo Info;
};

void ExecuteError(const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);

	ExecuteException ee;
	ee.Info.Location = nullptr;
	ee.Info.Message = buf;
	throw ee;
}

#define ExecuteAssert(expression, message, ...) \
do {											\
	if (!(expression)) {						\
		ExecuteError(message, ##__VA_ARGS__);	\
	}											\
} while (0);									\

void EvaluateConstants(ExecuteContext* ec, std::vector<Constant*>& cnsts)
{
	ast::EvaluationContext evCtx;
	evCtx.DisplaySize = ec->DisplaySize;
	evCtx.Time = ec->Time;
	for (Constant* cnst : cnsts)
	{
		ast::Result res;
		ast::EvaluateErrorState es;
		ast::Evaluate(evCtx, cnst->Expr, res, es);
		if (!es.EvaluateSuccess)
		{
			ExecuteException ee;
			ee.Info.Location = es.Info.Location;
			ee.Info.Message = "AST evaluation error: " + es.Info.Message;
			throw ee;
		}
		ExecuteAssert( (cnst->Type.Fmt != VariableFormat::Float4x4 && 
			res.Type.Fmt != VariableFormat::Float4x4) || cnst->Type.Fmt == res.Type.Fmt,
			"Constant %s expected type (%s) is not compatible with actual type (%s)",
			cnst->Name, TypeFmtToString(cnst->Type.Fmt), TypeFmtToString(res.Type.Fmt));
		ExecuteAssert(cnst->Type.Dim == res.Type.Dim,
			"Constant %s size (%u) doe not match actual size (%u)",
			cnst->Name, cnst->Type.Dim, res.Type.Dim);
		Convert(res, cnst->Type.Fmt);
		cnst->Value = res.Value;
	}
}

void ExecuteSetConstants(ExecuteContext* ec, std::vector<SetConstant>& sets, 
	std::vector<ConstantBuffer>& buffers)
{
	ID3D11DeviceContext* ctx = ec->D3dCtx;
	ast::EvaluationContext evCtx;
	evCtx.DisplaySize = ec->DisplaySize;
	evCtx.Time = ec->Time;
	for (const SetConstant& set : sets)
	{
		ast::Result res;
		ast::EvaluateErrorState es;
		ast::Evaluate(evCtx, set.Value, res, es);
		if (!es.EvaluateSuccess)
		{
			ExecuteException ee;
			ee.Info.Location = es.Info.Location;
			ee.Info.Message = "AST evaluation error: " + es.Info.Message;
			throw ee;
		}
		u32 typeSize = res.Type.Dim * 4;
		ExecuteAssert(set.Size == typeSize, 
			"SetConstant %s does not match size, expected=%u got=%u",
			set.VariableName, set.Size, typeSize);
		if (set.Type != res.Type)
			Convert(res, set.Type.Fmt);
		// ExecuteAssert(set.Type == res.Type, 
		// 	"SetConstant %s does not match type, expected=%s got=%s",
		// 	set.VariableName, TypeToString(set.Type), TypeToString(res.Type));
		if (res.Type.Fmt == VariableFormat::Bool)
			for (u32 i = 0 ; i < res.Type.Dim ; ++i)
				*(((u32*)(set.CB->BackingMemory+set.Offset)) + i) = 
					res.Value.Bool4Val.m[i] ? 1 : 0;
		else
			memcpy(set.CB->BackingMemory+set.Offset, &res.Value, typeSize);
	}
	for (const ConstantBuffer& buf : buffers)
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		HRESULT hr = ctx->Map(buf.BufferObject, 0, D3D11_MAP_WRITE_DISCARD, 
			0, &mapped_resource);
		Assert(hr == S_OK, "failed to map CB hr=%x", hr);
		memcpy(mapped_resource.pData, buf.BackingMemory, buf.Size);
		ctx->Unmap(buf.BufferObject, 0);
	}
}

void ExecuteDispatch(
	Dispatch* dc,
	ExecuteContext* ec)
{
	ID3D11DeviceContext* ctx = ec->D3dCtx;
	UINT initialCount = (UINT)-1;
	ctx->CSSetShader(dc->Shader->ShaderObject, nullptr, 0);
	ExecuteSetConstants(ec, dc->Constants, dc->CBs);
	for (const ConstantBuffer& buf : dc->CBs)
	{
		ctx->CSSetConstantBuffers(buf.Slot, 1, &buf.BufferObject);
	}
	for (Bind& bind : dc->Binds)
	{
		switch (bind.Type)
		{
		case BindType::SystemValue:
			if (bind.SystemBind == SystemValue::BackBuffer)
				ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &ec->MainRtUav, 
					&initialCount);
			else
				Assert(false, "unhandled");
			break;
		case BindType::Buffer:
			if (bind.IsOutput)
				ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &bind.BufferBind->UAV, 
					&initialCount);
			else
				ctx->CSSetShaderResources(bind.BindIndex, 1, &bind.BufferBind->SRV);
			break;
		case BindType::Texture:
			if (bind.IsOutput)
				ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &bind.TextureBind->UAV, 
					&initialCount);
			else
				ctx->CSSetShaderResources(bind.BindIndex, 1, &bind.TextureBind->SRV);
			break;
		case BindType::Sampler:
			ctx->CSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->SamplerObject);
			break;
		default:
			Assert(false, "invalid type %d", bind.Type);
		}
	}
	if (dc->Indirect)
	{
		ctx->DispatchIndirect(dc->IndirectArgs->BufferObject, dc->IndirectArgsOffset);
	}
	else
	{
		uint3 groups = dc->Groups;
		if (dc->ThreadPerPixel && ec->DisplaySize.x != 0 && ec->DisplaySize.y != 0)
		{
			uint3 tgs = dc->Shader->ThreadGroupSize;
			groups.x = (u32)((ec->DisplaySize.x - 1) / tgs.x) + 1;
			groups.y = (u32)((ec->DisplaySize.y - 1) / tgs.y) + 1;
			groups.z = 1;
		}

		ctx->Dispatch(groups.x, groups.y, groups.z);
	}
}

void ExecuteDraw(
	Draw* draw,
	ExecuteContext* ec)
{
	ID3D11DeviceContext* ctx = ec->D3dCtx;
	ctx->VSSetShader(draw->VShader->ShaderObject, nullptr, 0);
	ctx->IASetInputLayout(draw->VShader->InputLayout);
	ctx->PSSetShader(draw->PShader ? draw->PShader->ShaderObject : nullptr, nullptr, 0);
	ExecuteSetConstants(ec, draw->VSConstants, draw->VSCBs);
	ExecuteSetConstants(ec, draw->PSConstants, draw->PSCBs);
	for (const ConstantBuffer& buf : draw->VSCBs)
	{
		ctx->VSSetConstantBuffers(buf.Slot, 1, &buf.BufferObject);
	}
	for (const ConstantBuffer& buf : draw->PSCBs)
	{
		ctx->PSSetConstantBuffers(buf.Slot, 1, &buf.BufferObject);
	}
	for (Bind& bind : draw->VSBinds)
	{
		switch (bind.Type)
		{
		case BindType::Buffer:
			ctx->VSSetShaderResources(bind.BindIndex, 1, &bind.BufferBind->SRV);
			break;
		case BindType::Texture:
			ctx->VSSetShaderResources(bind.BindIndex, 1, &bind.TextureBind->SRV);
			break;
		case BindType::Sampler:
			ctx->VSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->SamplerObject);
			break;
		case BindType::SystemValue:
		default:
			Assert(false, "unhandled type %d", bind.Type);
		}
	}
	for (Bind& bind : draw->PSBinds)
	{
		switch (bind.Type)
		{
		case BindType::Buffer:
			ctx->PSSetShaderResources(bind.BindIndex, 1, &bind.BufferBind->SRV);
			break;
		case BindType::Texture:
			ctx->PSSetShaderResources(bind.BindIndex, 1, &bind.TextureBind->SRV);
			break;
		case BindType::Sampler:
			ctx->PSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->SamplerObject);
			break;
		case BindType::SystemValue:
		default:
			Assert(false, "unhandled type %d", bind.Type);
		}
	}
	ID3D11RenderTargetView* rtViews[8] = {};
	D3D11_VIEWPORT vp[8] = {};
	u32 rtCount = 0;
	for (TextureTarget target : draw->RenderTarget)
	{
		if (target.Type == BindType::SystemValue)
		{
			if (target.System == SystemValue::BackBuffer)
			{
				rtViews[rtCount] = ec->MainRtv;
				vp[rtCount].Width = (float)ec->DisplaySize.x;
				vp[rtCount].Height = (float)ec->DisplaySize.y;
			}
			else 
				Unimplemented();
		}
		else
		{
			vp[rtCount].Width = (float)target.Texture->Size.x;
			vp[rtCount].Height = (float)target.Texture->Size.y;
			rtViews[rtCount] = target.Texture->RTV;
		}
		vp[rtCount].MinDepth = 0.0f;
		vp[rtCount].MaxDepth = 1.0f;
		vp[rtCount].TopLeftX = vp[rtCount].TopLeftY = 0;
		++rtCount;
	}
	ID3D11DepthStencilView* dsView = nullptr;
	for (TextureTarget target : draw->DepthStencil)
	{
		if (target.Type == BindType::SystemValue)
		{
			if (target.System == SystemValue::DefaultDepth)
			{
				dsView = ec->DefaultDepthView;
				vp[rtCount].Width = (float)ec->DisplaySize.x;
				vp[rtCount].Height = (float)ec->DisplaySize.y;
			}
			else
				Unimplemented();
		}
		else
		{
			dsView = target.Texture->DSV;
			vp[0].Width = (float)target.Texture->Size.x;
			vp[0].Height = (float)target.Texture->Size.y;
		}
		vp[0].MinDepth = 0.0f;
		vp[0].MaxDepth = 1.0f;
	}
	ctx->OMSetRenderTargets(8, rtViews, dsView);
	ctx->RSSetViewports(8, vp);
	ctx->IASetPrimitiveTopology(RlfToD3d(draw->Topology));
	ctx->RSSetState(draw->RState ? draw->RState->RSObject : DefaultRasterizerState);
	ctx->OMSetDepthStencilState(draw->DSState ? draw->DSState->DSSObject : nullptr,
		draw->StencilRef);
	if (draw->Type == DrawType::Draw)
	{
		ctx->Draw(draw->VertexCount, 0);
	}
	else if (draw->Type == DrawType::DrawIndexed)
	{
		Buffer* ib = draw->IndexBuffer;
		Buffer* vb = draw->VertexBuffer;
		ExecuteAssert(ib, "DrawIndexed must have an index buffer");
		ctx->IASetIndexBuffer(ib->BufferObject, ib->ElementSize == 2 ? 
			DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
		u32 offset = 0;
		if (vb)
			ctx->IASetVertexBuffers(0, 1, &vb->BufferObject, &vb->ElementSize, 
				&offset);
		ctx->DrawIndexed(ib->ElementCount, 0, 0);
	}
	else
	{
		Unimplemented();
	}
}

void _Execute(
	ExecuteContext* ec,
	RenderDescription* rd)
{
	ID3D11DeviceContext* ctx = ec->D3dCtx;

	EvaluateConstants(ec, rd->Constants);

	// Clear state so we aren't polluted by previous program drawing or previous 
	//	execution. 
	ctx->ClearState();

	for (Pass pass : rd->Passes)
	{
		if (pass.Type == PassType::Dispatch)
		{
			ExecuteDispatch(pass.Dispatch, ec);
		}
		else if (pass.Type == PassType::Draw)
		{
			ExecuteDraw(pass.Draw, ec);
		}
		else if (pass.Type == PassType::ClearColor)
		{
			float4& color = pass.ClearColor->Color;
			const float clear_color[4] =
			{
				color.x, color.y, color.z, color.w
			};
			ctx->ClearRenderTargetView(pass.ClearColor->Target->RTV, clear_color);
		}
		else if (pass.Type == PassType::ClearDepth)
		{
			ctx->ClearDepthStencilView(pass.ClearDepth->Target->DSV, D3D11_CLEAR_DEPTH,
				pass.ClearDepth->Depth, 0);
		}
		else if (pass.Type == PassType::ClearStencil)
		{
			ctx->ClearDepthStencilView(pass.ClearStencil->Target->DSV, D3D11_CLEAR_STENCIL,
				0.f, pass.ClearStencil->Stencil);
		}
		else
		{
			Unimplemented();
		}

		// Clear state after execution so we don't pollute the rest of program drawing. 
		ctx->ClearState();
	}

}

void Execute(
	ExecuteContext* ec,
	RenderDescription* rd,
	ExecuteErrorState* es)
{
	es->ExecuteSuccess = true;
	try {
		_Execute(ec, rd);
	}
	catch (ExecuteException ee)
	{
		es->ExecuteSuccess = false;
		es->Info = ee.Info;
	}
}

#undef ExeucteAssert


} // namespace rlf
