
cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
	float4x4 Matrix;
	float3 LightPos;
}

struct VSInput {
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct VSOutput {
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
};

VSOutput VSMain(uint id : SV_VertexID, VSInput input)
{
	VSOutput output;
	output.pos = mul(Matrix, float4(input.pos, 1.0));
	output.tex = input.tex;
	return output;
}

Texture2D map_Ka;
SamplerState Sampler;

float4 PSMain(VSOutput input) : SV_Target0
{
	// it seems the UVs for sponza are in opengl texture space (y==0 at the bottom)
	input.tex.y = 1-input.tex.y;

	float alpha = map_Ka.Sample(Sampler, input.tex).a;
	clip( alpha - 0.1 );

	return float4(1,1,1,1);
}
