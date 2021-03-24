
RWTexture2D<uint4> OutBuffer : register(u0);

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	OutBuffer[DTid.xy] = uint4(0,1,1,1);
}
