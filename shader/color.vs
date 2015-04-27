#version 450

in vec3 gInputPosition;
in vec3 gInputColor;

layout(location = 0) out vec3 gOutputColor;

uniform mat4 gProjViewWorldMatrix;

void main()
{
	gl_Position = gProjViewWorldMatrix * vec4(gInputPosition,1.0);
	gOutputColor = gInputColor;
}
