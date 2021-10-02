
cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
	float4x4 Matrix;
}

float4 VSMain(uint id : SV_VertexID, float3 pos : POSITION) : SV_Position
{
	float4 res = mul(Matrix, float4(pos, 1.0));
	return res;
}

StructuredBuffer<float3> VB;
float4 VSMainWithBuffer(uint id : SV_VertexID) : SV_Position
{
	float4 res = mul(Matrix, float4(VB[id], 1.0));
	return res;
}

float4 PSMain(float4 pos : SV_Position) : SV_Target0
{
	return float4(pos.zzz*pos.z, 1);
}

float4 PSMain_Black(float4 pos : SV_Position) : SV_Target0
{
	return float4(1-(pos.zzz*pos.z), 1);
}

float3 LightPos;

RWStructuredBuffer<float3> VBOut;
RWStructuredBuffer<float3> ShadowOut;
[numthreads(4,1,1)]
void CSGenerateBlockerAndShadow(uint3 DTid : SV_DispatchThreadID)
{
	float3 coords;
	coords.y = 0.5;
	coords.x = DTid.x < 2 ? 0.2 : -0.2;
	coords.z = (DTid.x & 1) ? -0.2 : 0.2;
	float x = coords.x;
	float z = coords.z;
	float sint = sin(Time);
	float cost = cos(Time);
	coords.x = cost*x - sint*z;
	coords.z = sint*x + cost*z;
	VBOut[DTid.x] = coords;
	ShadowOut[DTid.x] = coords;
	ShadowOut[4+DTid.x] = coords + 4*(coords-LightPos);
}

[numthreads(1,1,1)]
void CSGenerateShadowVolumes(uint3 DTid : SV_DispatchThreadID)
{
	VBOut[DTid.x] = 1;
}

Texture2D<float> Depth;
Texture2D<uint2> Stencil;
RWTexture2D<float4> OutTexture;
SamplerState Sampler;

[numthreads(8,8,1)]
void CSResolve(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > uint2(TextureSize)))
		return;

	uint mips;
	uint2 dims;
	Depth.GetDimensions(0, dims.x, dims.y, mips);

	float2 loadCoords = DTid.xy / TextureSize * dims;	

	float depth = Depth.Load(float3(loadCoords,0));
	uint stencil = Stencil.Load(float3(loadCoords,0)).g;

	float s = depth < 1 ? 1-depth + 0.3 : 0;
	float inShadow = stencil == 0 ? s : s*0.5;

	OutTexture[DTid.xy] = float4(inShadow.xxx, 1);
}

