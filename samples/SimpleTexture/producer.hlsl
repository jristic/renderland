
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
	int2 coords = (DTid.xy + Time*50) % 512;
	if (coords.x > 255)
		coords.x = 511 - coords.x;
	if (coords.y > 255)
		coords.y = 511 - coords.y;
	float3 color = float3(coords / 255.0, (256 - coords.x) /255.0);
	OutTexture[DTid.xy] = float4(color, 1);
}
