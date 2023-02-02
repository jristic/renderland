
StructuredBuffer<float4> Points : register(t0);
RWTexture2D<float4> OutTexture : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	uint BallCount;
}

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > TextureSize))
		return;

	float hit = 0;

	for (uint i = 0 ; i < BallCount ; ++i)
	{
		float4 pt = Points[i];
		if (distance(DTid.xy, pt.xy) < 10)
		{
			hit = 1;
			break;
		}
	}

	OutTexture[DTid.xy] = float4(hit, hit, hit, 1);
}
