
Texture2D<float4> InTexture : register(t0);
RWTexture2D<float4> OutTexture : register(u0);
SamplerState Sampler : register(s0);

// cbuffer ConstantBuffer
// {
	float2 TextureSize;
	float Time;
// }

float4x4 Matrix;

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > (uint2)TextureSize))
		return;

	float4 val = InTexture.SampleLevel(Sampler, DTid.xy/TextureSize, 0);

	OutTexture[DTid.xy] = val;
}

struct VSInput {
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct VSOutput {
	float4 pos : SV_Position;
	float3 n : NORMAL0;
};

VSOutput VSMain(uint id : SV_VertexID, VSInput input)
{
	VSOutput output;
	output.pos = mul(Matrix, float4(input.pos, 1.0));
	output.n = input.normal;
	return output;
}

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 col = float4(0,0,0,1);
	col.rgb = sqrt(input.pos.z) + 0.1;

	return col;
}
