
cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
	float4x4 Matrix;
	float3 LightPos;
}

struct VSInput {
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct VSOutput {
	float4 pos : SV_Position;
	float3 WorldPos : POSITION;
	float3 n : NORMAL0;
};

VSOutput VSMain(uint id : SV_VertexID, VSInput input)
{
	VSOutput output;
	output.pos = mul(Matrix, float4(input.pos, 1.0));
	output.n = input.normal;
	output.WorldPos = input.pos;
	return output;
}

float4x4 LightView;
float4x4 LightProjection;

Texture2D<float> ShadowDepth;
SamplerState ShadowSampler;

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 col = float4(0,0,0,1);

	// Compute NDC / homogenous coords
	float4 hc = input.pos;
	hc.xy = (hc.xy / TextureSize) * 2 - 1;
	hc.y *= -1;
	hc.xyz *= hc.w;
	float4 worldCoords = mul(Matrix, hc);

	float3 dir = LightPos - worldCoords.xyz;
	float light = saturate(dot(input.n, normalize(dir)));
	col.rgb = light + 0.05;

	// Compute coords in light view space
	float4 lvc = mul(LightView, worldCoords);
	float4 lndc = mul(LightProjection, lvc);

	// if (lvc.z > 3) col.rgb = 1;
	if (sqrt(lvc.x*lvc.x + lvc.y*lvc.y) > lvc.z) col = 0.05;

	lndc.xyz /= lndc.w;
	lndc.y *= -1;
	lndc.xy = lndc.xy * 0.5 + 0.5;

	float depth = ShadowDepth.Sample(ShadowSampler, lndc.xy);
	if (depth + 0.0001 < lndc.z) col = 0;

	// col.xy = 1;
	// if (lndc.x > 1 || lndc.y > 1) col = 1;
	// if (lndc.x < 0 || lndc.y < 0) col = 1;

	return col;
}
