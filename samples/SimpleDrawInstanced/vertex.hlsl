
float Time;

struct VSOutput {
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
};

VSOutput VSMain(uint id : SV_VertexID, uint iid : SV_InstanceID)
{
	float Rads = Time + iid * 3.14 / 2;
	float2 InstancePos = float2(sin(Rads), cos(Rads)) * 0.5;

	float2 VertexPos = float2(
		(id / 2) * 4.0 - 1.0, 
		(id % 2) * 4.0 - 1.0);

	// generate clip space position
	VSOutput output;
	output.pos.xy = InstancePos + VertexPos*0.05;
	output.pos.z = 0.0;
	output.pos.w = 1.0;
	// texture coordinates
	output.tex.x = (float)(id / 2) * 2.0;
	output.tex.y = 1.0 - (float)(id % 2) * 2.0;

	return output;
}
