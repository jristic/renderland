
Texture2D<float4> InTex0;
Texture2D<float> InTex1;
Texture2D<float2> InTex2;
Texture2D<float> InDS;
RWTexture2D<float4> OutTexture : register(u0);

uint2 TextureSize;

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > TextureSize))
		return;

	float4 val = 0;
	if (all(DTid.xy <= TextureSize/2))
		val = InTex0.Load(int3(DTid.xy,0));
	else if (DTid.x > TextureSize.x/2 && DTid.y <= TextureSize.y/2)
		val = float4(InTex1.Load(int3(DTid.xy,0)).rrr, 1);
	else if (DTid.x <= TextureSize.x/2 && DTid.y > TextureSize.y/2)
		val = float4(InTex2.Load(int3(DTid.xy,0)).rg, 0, 1);
	else if (all(DTid.xy > TextureSize/2))
		val = float4(InDS.Load(int3(DTid.xy,0)).rrr, 1);

	OutTexture[DTid.xy] = val;
}
