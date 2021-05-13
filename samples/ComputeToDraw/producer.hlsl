
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
	float2 coords = DTid.xy / 127.0;
	OutTexture[DTid.xy] = float4(coords.xy, 1-coords.x, 1);
}
