#version 460

in block
{
	vec2 Texcoord;
	vec3 Color;
} In;

layout(binding = 0) uniform sampler2D basicTexture;
layout(location = 0) out vec4 Color;

void main()
{
	vec4 textureColor = texture(basicTexture, In.Texcoord).rgba;
	textureColor.rgb = textureColor.rgb * In.Color;
	Color = vec4(textureColor);
}