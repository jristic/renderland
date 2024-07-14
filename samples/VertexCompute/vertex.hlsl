
cbuffer ConstantBuffer : register(b0)
{
	float4x4 Matrix;
}

struct VSInput {
	float3 pos : POSITION;
};

struct VSOutput {
	float4 pos : SV_Position;
	float3 wpos : TEXCOORD0;
};

VSOutput VSMain(uint id : SV_VertexID, VSInput input)
{
	VSOutput output;
	output.pos = mul(Matrix, float4(input.pos, 1.0));
	output.wpos = input.pos;
	return output;
}
