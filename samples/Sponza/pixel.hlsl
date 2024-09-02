
struct VSOutput {
	float4 pos : SV_Position;
	float3 n : NORMAL0;
	float2 tex : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
};

Texture2D map_Ka;
SamplerState Sampler;

float4 PSMain(VSOutput input) : SV_Target0
{
	// it seems the UVs for sponza are in opengl texture space (y==0 at the bottom)
	input.tex.y = 1-input.tex.y;

	float4 albedo = map_Ka.Sample(Sampler, input.tex);
	if (albedo.a < 0.1)
		discard;

	float3 lightPos = float3(0, 200, 0);
	float3 dir = lightPos - input.worldPos;
	float distFactor = saturate( 1 - length(dir) / 1000 );
	float light = dot(input.n, normalize(dir)) * distFactor;
	float3 col = (saturate(light) + 0.5) * albedo.rgb;

	return float4(col, 1);
}
