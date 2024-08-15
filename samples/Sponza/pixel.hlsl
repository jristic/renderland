
struct VSOutput {
	float4 pos : SV_Position;
	float3 n : NORMAL0;
	float2 tex : TEXCOORD0;
};

Texture2D map_Ka;
SamplerState Sampler;

float4 PSMain(VSOutput input) : SV_Target0
{
	// float4 col = float4(0,0,0,1);

	// float3 dir = float3(1,0,-1);
	// float light = dot(input.n, normalize(dir));
	// col.rgb = abs(light) + 0.1;

	// return col;

	// it seems the UVs for sponza are in opengl texture space (y==0 at the bottom)
	input.tex.y = 1-input.tex.y;

	float4 albedo = map_Ka.Sample(Sampler, input.tex);
	if (albedo.a < 0.1)
		discard;

	return albedo;
}
