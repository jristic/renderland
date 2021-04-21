
RWTexture2D<float4> OutTexture : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	float Padding;
}

[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy > TextureSize))
		return;

	float2 delta = DTid.xy - TextureSize/2;
	float2 deltaSqr = delta*delta;
	float dist = sqrt(deltaSqr.x + deltaSqr.y);

	float theta;
	if (delta.y > 0)
		theta = 0.5 * atan2(delta.y,delta.x) /3.14;
	else
		theta = (0.5*atan2(-delta.y,-delta.x) )/3.14 + 0.5;

	float intensity = (sin(dist/30 + theta*3.14*2 - Time*10) + 1) /2;
	if ((DTid.x/4 + DTid.y/4)%2 == 0)
		intensity = pow(intensity,2);

	OutTexture[DTid.xy] = float4(intensity, intensity, intensity, 1);
}
