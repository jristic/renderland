
struct VSOutput {
	float4 pos : SV_Position;
	float3 n : NORMAL0;
};

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 col = float4(0,0,0,1);

	float3 dir = float3(1,0,-1);
	float light = dot(input.n, normalize(dir));
	col.rgb = light;

	return col;
}
