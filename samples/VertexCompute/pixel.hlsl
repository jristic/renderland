
struct VSOutput {
	float4 pos : SV_Position;
	float3 wpos : TEXCOORD0;
};

float4 PSMain(VSOutput input) : SV_Target0
{
	float3 lightPos = float3(1.5,1,1);

	float contribution = 1/distance(input.wpos, lightPos);

	float4 res = 1;
	res.rgb = contribution;
	return res;
}
