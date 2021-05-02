
Texture2D<float4> InTexture : register(t0);
RWTexture2D<float4> OutTexture : register(u0);
SamplerState Sampler : register(s0);

cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
}

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > TextureSize))
		return;

	int2 coords = (DTid.xy + Time*30) % 512;
	if (coords.x > 255)
		coords.x = 511 - coords.x;
	if (coords.y > 255)
		coords.y = 511 - coords.y;

	float4 val = InTexture.SampleLevel(Sampler, coords/255.0,0);

	OutTexture[DTid.xy] = val;
}
