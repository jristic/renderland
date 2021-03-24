
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
	if (any(DTid.xy > TextureSize))
		return;

	float2 delta = DTid.xy - TextureSize/2;
	float2 deltaSqr = delta*delta;
	float dist = sqrt(deltaSqr.x + deltaSqr.y);
	float intensity = (sin(dist/30 - Time*10) + 1) /2;
	OutTexture[DTid.xy] = float4(intensity, intensity, intensity, 1);
}
