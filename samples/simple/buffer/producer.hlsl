
RWStructuredBuffer<float4> Points : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	uint BallCount;
}

[numthreads(32,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= BallCount)
		return;
	float4 pt = Points[DTid.x];
	if (all(pt == 0))
	{
		// setup random velocity
		uint random = DTid.x * 3973217.0;
		pt.z = (random % 256.0) / 255.0;
		pt.w = ((random / 256.0) % 256.0) / 255.0;
		pt.zw *= (random / 65536.0) % 5.0;
	}

	pt.xy += pt.zw;

	// check collision
	if (pt.x < 0)
	{
		pt.x = abs(pt.x);
		pt.z = -pt.z;
	}
	else if (pt.x > TextureSize.x)
	{
		pt.x = 2*TextureSize.x - pt.x;
		pt.z = -pt.z;
	}

	if (pt.y < 0)
	{
		pt.y = abs(pt.y);
		pt.w = -pt.w;
	}
	else if (pt.y > TextureSize.y)
	{
		pt.y = 2*TextureSize.y - pt.y;
		pt.w = -pt.w;
	}

	// output updated result
	Points[DTid.x] = pt;
}
