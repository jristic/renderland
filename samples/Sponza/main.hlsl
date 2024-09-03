
cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
	float4x4 Matrix;
}

struct VSInput {
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct VSOutput {
	float4 pos : SV_Position;
	float3 n : NORMAL0;
	float2 tex : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
};

VSOutput VSMain(uint id : SV_VertexID, VSInput input)
{
	VSOutput output;
	output.pos = mul(Matrix, float4(input.pos, 1.0));
	output.n = input.normal;
	output.tex = input.tex;
	output.worldPos = input.pos;
	return output;
}

float4x4 LightView;
float4x4 LightProjection;
float3 LightPos;

Texture2D<float> ShadowDepth;
SamplerState ShadowSampler;

Texture2D map_Ka;
SamplerState Sampler;

float GetShadowAmount(float4 svpos)
{
	float shadow = 1;

	// Compute NDC / homogenous coords
	float4 hc = svpos;
	hc.xy = (hc.xy / TextureSize) * 2 - 1;
	hc.y *= -1;
	hc.xyz *= hc.w;
	float4 worldCoords = mul(Matrix, hc);

	// Compute coords in light view space
	float4 lvc = mul(LightView, worldCoords);
	float4 lndc = mul(LightProjection, lvc);

	if (sqrt(lvc.x*lvc.x + lvc.y*lvc.y) > lvc.z) shadow = 0;

	lndc.xyz /= lndc.w;
	lndc.y *= -1;
	lndc.xy = lndc.xy * 0.5 + 0.5;

	float depth = ShadowDepth.Sample(ShadowSampler, lndc.xy);
	if (depth < lndc.z) shadow = 0;

	return shadow;

}

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
	float3 shadowLightDir = normalize(LightPos - input.worldPos);
	float shadowLight = dot(input.n, shadowLightDir) * GetShadowAmount(input.pos);
	float3 col = (saturate(light)*0.5 + saturate(shadowLight)*0.5 + 0.5) * albedo.rgb;

	return float4(col, 1);
}

