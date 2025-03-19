#version 460

layout(binding = 0) uniform sampler2D basicTexture;

layout(location = 0, index = 0) out vec4 color;

void main()
{
	vec2 textureSize = vec2(textureSize(basicTexture, 0));

	color = texture(basicTexture, gl_FragCoord.xy / textureSize);
}