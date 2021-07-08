
RWTexture2D<float4> OutTexture : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	float2 TextureSize;
	float Time;
	bool Clockwise;
	int Speed;
	float Wavelength;
	uint CheckerSize;
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

	if (!Clockwise)
		theta *= -1;

	float intensity = (sin(dist/Wavelength + theta*3.14*2 - Time*Speed) + 1) /2;
	if ((DTid.x/CheckerSize + DTid.y/CheckerSize)%2 == 0)
		intensity = pow(intensity,2);

	OutTexture[DTid.xy] = float4(intensity, intensity, intensity, 1);
}
