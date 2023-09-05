
cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
	float4x4 Matrix;
}


struct LightData {
	float3 Position;
};
RWStructuredBuffer<LightData> LightBufferWrite;
uint NumLights = 0;

[numthreads(64,1,1)]
void CSUpdateLights(uint3 DTid : SV_DispatchThreadID) {
	if (DTid.x >= NumLights)
		return;
	LightData light;
	light.Position = float3(2*sin(Time), 1, 2*cos(Time));
	// light.Position = float3(0,1,1);
	LightBufferWrite[DTid.x] = light;
}

struct VSInput {
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct VSOutput {
	float4 pos : SV_Position;
	float3 WorldPos : POSITION;
	float3 n : NORMAL0;
};

VSOutput VSMain(uint id : SV_VertexID, VSInput input)
{
	VSOutput output;
	output.pos = mul(Matrix, float4(input.pos, 1.0));
	output.n = input.normal;
	output.WorldPos = input.pos;
	return output;
}

StructuredBuffer<LightData> LightBufferRead;

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 col = float4(0,0,0,1);
	float3 worldCoords = input.WorldPos;

	float3 LightPos = LightBufferRead[0].Position;
	// float3 LightPos = float3(0,1,1);
	float3 dir = LightPos - worldCoords;
	float light = saturate(dot(input.n, normalize(dir)));
	col.rgb = light + 0.05;

	// col.xy = 1;
	// if (lndc.x > 1 || lndc.y > 1) col = 1;
	// if (lndc.x < 0 || lndc.y < 0) col = 1;

	return col;
}


Texture2D<float4> InTexture : register(t0);
RWTexture2D<float4> OutTexture : register(u0);
SamplerState Sampler : register(s0);

[numthreads(8,8,1)]
void CSCopy(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > (uint2)TextureSize))
		return;

	float4 val = InTexture.SampleLevel(Sampler, DTid.xy/TextureSize, 0);

	OutTexture[DTid.xy] = val;
}
