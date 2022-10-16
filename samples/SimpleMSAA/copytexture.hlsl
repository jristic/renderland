
Texture2D<float4> InTexture : register(t0);
RWTexture2D<float4> OutTexture : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
}

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > (uint2)TextureSize))
		return;

	float4 val = InTexture.Load(int3(DTid.xy,0));

	OutTexture[DTid.xy] = val;
}

uint NumSamples;
Texture2DMS<float4> InTexture_MS;

[numthreads(8,8,1)]
void CSAlt(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > (uint2)TextureSize))
		return;

	float4 val = 0;
	for (uint sample = 0 ; sample < NumSamples ; ++sample)
		val += InTexture_MS.Load(DTid.xy, sample);
	val /= NumSamples;
	val.a = 1;

	OutTexture[DTid.xy] = val;
}
