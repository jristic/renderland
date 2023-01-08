
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
	output.pos = float4(input.pos, 1.0);
	output.n = input.normal;
	return output;
}
