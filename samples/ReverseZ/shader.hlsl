
float4x4 Matrix;

struct VSInput {
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct VSOutput {
	float4 pos : SV_Position;
	float3 n : NORMAL0;
};

VSOutput VSMain(uint id : SV_VertexID, VSInput input)
{
	VSOutput output;
	output.pos = mul(Matrix, float4(input.pos, 1.0));
	output.n = input.normal;
	return output;
}

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 col = float4(0,0,0,1);
	col.rgb = sqrt(input.pos.z) + 0.1;

	return col;
}
