
RWByteAddressBuffer OutVerts;

float Time;

[numthreads(4,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	OutVerts.Store3(DTid.x*12, asuint(float3(0, sin(Time+DTid.x), DTid.x)));
}