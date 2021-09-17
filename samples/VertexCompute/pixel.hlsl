
struct VSOutput {
	float4 pos : SV_Position;
};

float4 PSMain(VSOutput input) : SV_Target0
{
	return float4(1,1,1,1);
}
