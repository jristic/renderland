
RWByteAddressBuffer OutVerts;

float Time;
uint VertCount;

[numthreads(64,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= VertCount)
		return;
	uint byteOffset = DTid.x * 4 * 8;
	float3 pos = asfloat(OutVerts.Load3(byteOffset));

	pos = normalize(pos);
	float amt = (pos.x*573.9 + pos.y*337.3 + pos.z*97.3 + Time*0.1) % 0.1;
	pos = (1 + amt) * pos;

	OutVerts.Store3(byteOffset, asuint(pos));
}