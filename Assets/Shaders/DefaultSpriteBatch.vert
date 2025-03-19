#version 460

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 Texcoord;
layout(location = 2) in vec3 Color;

out gl_PerVertex
{
	vec4 gl_Position;
};

out block
{
		vec2 Texcoord;
} Out;

void main()
{
	Out.Texcoord = Texcoord;
	gl_Position = vec4(Position, 0.0, 1.0);
}