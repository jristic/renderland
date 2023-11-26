
RWTexture2D<float4> OutTexture : register(u0);
RWBuffer<uint> OutArgs : register(u1);

cbuffer ConstantBuffer
{
	float2 TextureSize;
	float Time;
	float Padding;
}

[numthreads(1,1,1)]
void ProducerMain(uint3 DTid : SV_DispatchThreadID)
{
	uint3 args = 1;
	args.x = 20 + 10*sin(Time);
	args.y = 20 + 10*cos(Time);
	OutArgs[0] = args.x;
	OutArgs[1] = args.y;
	OutArgs[2] = args.z;
}

[numthreads(8,8,1)]
void ConsumerMain(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > uint2(TextureSize)))
		return;

	OutTexture[DTid.xy] = float4(1,0,0,1);
}
