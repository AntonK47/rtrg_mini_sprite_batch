#pragma once

#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <vector>

struct Color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};
namespace Colors
{
	inline constexpr Color White = Color{ 255, 255, 255, 255 };
	inline constexpr Color Black = Color{ 0, 0, 0, 255 };
} // namespace Colors

struct SpriteTexture
{
};

struct SpriteBatch
{
	SpriteBatch();
	virtual ~SpriteBatch();

	void Begin();
	void End();

	void Draw(const SpriteTexture texture, const glm::vec2& postion, const Color& color = Colors::White);

public:
	struct SpriteInfo
	{
		SpriteTexture texture;
		glm::vec2 position;
		Color color;
	};
	std::vector<SpriteBatch::SpriteInfo> spriteInfos;

	// openGL specific fields
	GLuint vertexBuffer;
	GLuint vertexArrayObject;
	const uint32_t defaultBufferSize = 16 * 1024 * 1024;
};