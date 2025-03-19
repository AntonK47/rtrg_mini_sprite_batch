#version 460

in block
{
	vec2 Texcoord;
} In;

uniform sampler2D texture;
layout(location = 0) out vec4 Color;

void main()
{
	Color = vec4(In.Texcoord,0.0f,1.0f);
}