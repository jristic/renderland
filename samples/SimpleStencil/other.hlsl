
struct VSOutput {
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
};

VSOutput VSMain(uint id : SV_VertexID)
{
	VSOutput output;
	// generate clip space position
	output.pos.x = (float)(id / 2) * 4.0 - 1.0;
	output.pos.y = (float)(id % 2) * 4.0 - 1.0;
	output.pos.z = 0.0;
	output.pos.w = 1.0;
	// texture coordinates
	output.tex.x = (float)(id / 2) * 2.0;
	output.tex.y = 1.0 - (float)(id % 2) * 2.0;

	return output;
}

float4 PSMain(float4 pos : SV_Position, float2 coords : TEXCOORD0) : SV_Target0
{
	return float4(coords, 1-coords.x, 1);
}

