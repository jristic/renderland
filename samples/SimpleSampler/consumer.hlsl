
Texture2D<float4> InTexture : register(t0);
RWTexture2D<float4> OutTexture : register(u0);
SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);

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

	float2 coords = DTid.xy / TextureSize;
	float top = 0.1;
	float bottom = 0.9;

	if (coords.y >= top && coords.y <= bottom)
	{
		coords.y = (coords.y - top) / (bottom - top);

		float left = 0.3 + 0.2*sin(Time + coords.y*3.14);
		float right = 0.7 - 0.2*sin(Time + coords.y*3.14);

		if (coords.x >= left && coords.x <= right)
		{
			coords.x = (coords.x - left) / (right - left);
			float4 val = 0;
			if (coords.x < 0.5)
			{
				val = InTexture.SampleLevel(PointSampler, coords,0);
			}
			else
			{
				val = InTexture.SampleLevel(LinearSampler, coords,0);
			}

			OutTexture[DTid.xy] = val;
		}
	}

}
