
RWTexture2D<float4> OutTexture : register(u0);

float2 TextureSize;

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > uint2(TextureSize)))
		return;

	OutTexture[DTid.xy] = float4(1,0,0,1);
}
