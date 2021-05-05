
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
	int2 coords = DTid.xy;
	float3 color = float3(coords / 127.0, (128 - coords.x) /127.0);
	color = ((DTid.x + DTid.y) % 2) == 0 ? 1 : 0;
	OutTexture[DTid.xy] = float4(color, 1);
}
