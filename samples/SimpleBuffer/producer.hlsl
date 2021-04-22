
RWStructuredBuffer<float4> Points : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
}

[numthreads(32,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	float4 point = Points[DTid.x];
	if (all(point == 0))
	{
		// setup random velocity
		uint random = DTid.x * 3973217;
		point.z = (random % 256) / 255.0;
		point.w = ((random / 256) % 256) / 255.0;
		point.zw *= (random / 65536) % 5;
	}

	point.xy += point.zw;

	// check collision
	if (point.x < 0)
	{
		point.x = abs(point.x);
		point.z = -point.z;
	}
	else if (point.x > TextureSize.x)
	{
		point.x = 2*TextureSize.x - point.x;
		point.z = -point.z;
	}

	if (point.y < 0)
	{
		point.y = abs(point.y);
		point.w = -point.w;
	}
	else if (point.y > TextureSize.y)
	{
		point.y = 2*TextureSize.y - point.y;
		point.w = -point.w;
	}

	// output updated result
	Points[DTid.x] = point;
}
