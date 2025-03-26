#version 460

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 Texcoord;
layout(location = 2) in vec3 Color;

layout(binding = 0) uniform spriteBatchConstants
{
	vec2 viewportSize;
} SpriteBatchConstants;

out gl_PerVertex
{
	vec4 gl_Position;
};

out block
{
		vec2 Texcoord;
		vec3 Color;
} Out;

void main()
{
	float w = SpriteBatchConstants.viewportSize.x;
	float h = SpriteBatchConstants.viewportSize.y;
	vec2 p = Position.xy/vec2(w,h);
	p.y = 1.0-p.y;
	p = p*2.0f - vec2(1.0f, 1.0f);
	Out.Texcoord = Texcoord;
	gl_Position = vec4(p, 0.0, 1.0);
	Out.Texcoord = Texcoord;
	Out.Color = Color.rgb;
}