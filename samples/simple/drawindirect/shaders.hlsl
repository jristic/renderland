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

float4 PSMain(float4 pos : SV_Position, float2 coords : TEXCOORD0) : SV_Target0
{
	return float4(coords, 1-coords.x, 1);
}

RWByteAddressBuffer OutArgs : register(u1);
[numthreads(1,1,1)]
void CSProducer(uint3 DTid : SV_DispatchThreadID)
{
	uint vert_count = 3;
	uint instance_count = (uint)(Time % 4) + 1;
	uint start_vert = 0;
	uint start_instance = 0;
	uint4 args = uint4(vert_count, instance_count, start_vert, start_instance);
	OutArgs.Store4(0, args);
}
