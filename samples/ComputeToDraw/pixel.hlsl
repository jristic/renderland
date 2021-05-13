
Texture2D<float4> InTexture : register(t0);
SamplerState Sampler : register(s0);

float4 PSMain(float4 pos : SV_Position, float2 coords : TEXCOORD0) : SV_Target0
{
	return InTexture.SampleLevel(Sampler, coords, 0);
}
