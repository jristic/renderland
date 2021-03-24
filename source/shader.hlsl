
RWTexture2D<float4> OutBuffer : register(u0);

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	OutBuffer[DTid.xy] = float4(1,0,0,1);
}
