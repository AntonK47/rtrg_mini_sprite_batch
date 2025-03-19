#pragma once

#include <vector>

#include "Common.hpp"

struct Color
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};
namespace Colors
{
	inline constexpr Color White = Color{ 255, 255, 255, 255 };
	inline constexpr Color Black = Color{ 0, 0, 0, 255 };
} // namespace Colors

struct SpriteTexture
{
};

struct RenderContext;

struct SpriteBatch
{
	SpriteBatch(RenderContext* context);
	virtual ~SpriteBatch();

	void Begin();
	void End();

	void Draw(const SpriteTexture texture, const vec2& postion, const Color& color = Colors::White);

public:
	struct SpriteInfo
	{
		SpriteTexture texture;
		vec2 position;
		Color color;
	};
	std::vector<SpriteBatch::SpriteInfo> spriteInfos;

	// openGL specific fields
	GLuint vertexBuffer;
	GLuint vertexArrayObject;
	const u32 defaultBufferSize = 16 * 1024 * 1024;
};