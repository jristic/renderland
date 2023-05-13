
namespace gui {


void DisplayShaderConstants(std::vector<rlf::ConstantBuffer>& CBs, ID3D11ShaderReflection* reflector)
{
	D3D11_SHADER_DESC sd;
	reflector->GetDesc(&sd);
	u32 cb_index = 0;
	for (u32 ic = 0 ; ic < sd.ConstantBuffers ; ++ic)
	{
		D3D11_SHADER_BUFFER_DESC bd;
		ID3D11ShaderReflectionConstantBuffer* constBuffer = 
			reflector->GetConstantBufferByIndex(ic);
		constBuffer->GetDesc(&bd);

		if (bd.Type != D3D11_CT_CBUFFER)
			continue;

		const rlf::ConstantBuffer& cb = CBs[cb_index];
		// Buffers were not created for the skipped buffer types,
		//	so we need to advance index used in the rlf data only
		//	for those not skipped. 
		++cb_index; 

		for (u32 j = 0 ; j < bd.Variables ; ++j)
		{
			ID3D11ShaderReflectionVariable* var = constBuffer->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC vd;
			var->GetDesc(&vd);
			ID3D11ShaderReflectionType* ctype = var->GetType();
			D3D11_SHADER_TYPE_DESC td;
			ctype->GetDesc(&td);

			switch (td.Type)
			{
			case D3D_SVT_BOOL: {
				bool val = *(bool*)(cb.BackingMemory+vd.StartOffset);
				ImGui::Text("%s: %s", vd.Name, val ? "true" : "false");
				break;
			}
			case D3D_SVT_INT: {
				i32 val = *(i32*)(cb.BackingMemory+vd.StartOffset);
				ImGui::Text("%s: %d", vd.Name, val);
				break;
			}
			case D3D_SVT_UINT: {
				u32 val = *(u32*)(cb.BackingMemory+vd.StartOffset);
				ImGui::Text("%s: %u", vd.Name, val);
				break;
			}
			case D3D_SVT_FLOAT: {
				float val = *(float*)(cb.BackingMemory+vd.StartOffset);
				ImGui::Text("%s: %f", vd.Name, val);
				break;
			}
			default:
				Unimplemented();
			}
		}
	}
}

void DisplayShaderPasses(rlf::RenderDescription* rd)
{
	std::vector<rlf::Pass> passes = rd->Passes;
	for (int i = 0 ; i < passes.size() ; ++i)
	{
		rlf::Pass& p = passes[i];
		static int selected_index = -1;
		if (ImGui::Selectable(p.Name ? p.Name : "anon", selected_index == i))
			selected_index = i;
		ImGui::Indent();
		switch (p.Type) {
		case rlf::PassType::Dispatch:
			DisplayShaderConstants(p.Dispatch->CBs, p.Dispatch->Shader->Common.Reflector);
			break;
		case rlf::PassType::Draw:
			DisplayShaderConstants(p.Draw->VSCBs, p.Draw->VShader->Common.Reflector);
			DisplayShaderConstants(p.Draw->PSCBs, p.Draw->PShader->Common.Reflector);
			break;
		case rlf::PassType::ClearColor:
		case rlf::PassType::ClearDepth:
		case rlf::PassType::ClearStencil:
		case rlf::PassType::Resolve:
			break;
		default:
			Unimplemented();
		}
		ImGui::Unindent();
	}
}


}