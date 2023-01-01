
float Alpha = 1;

float4 PSMain(float4 pos : SV_Position, float2 coords : TEXCOORD0) : SV_Target0
{
	return float4(coords, 1-coords.x, Alpha);
}
