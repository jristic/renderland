
struct VSOutput {
	float4 pos : SV_Position;
	float3 n : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct PSOut
{
	float4 out0 : SV_Target0;
	float out1 : SV_Target1;
	float2 out2 : SV_Target2;
};

PSOut PSMain(VSOutput input)
{
	PSOut output;
	output.out0 = float4(input.pos.xyz, 1.0);
	output.out2 = input.tex;

	float3 dir = float3(1,0,-1);
	float light = dot(input.n, normalize(dir));
	output.out1 = light;

	return output;
}
