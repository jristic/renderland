
cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
	float4x4 Matrix;
}

struct VSInput {
	float3 pos : POSITION;
};

struct VSOutput {
	float4 pos : SV_Position;
};

VSOutput VSMain(uint id : SV_VertexID, VSInput input)
{
	VSOutput output;
	output.pos = mul(Matrix, float4(input.pos, 1.0));
	return output;
}
