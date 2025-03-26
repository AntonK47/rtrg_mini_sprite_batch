#pragma once

#include <vector>

#include "Color.hpp"
#include "Common.hpp"
#include "RenderResources.hpp"


struct RenderContext;

struct Buffer
{
	GLuint nativeHandle;
	void* mappedPtr{ nullptr };
};

struct SpriteBatchConstants
{
	vec2 viewportSize;
};

enum class FlipSprite
{
	none,
	horizontal,
	vertical,
	horizontalAndVertical
};

struct SpriteBatch
{
	SpriteBatch(RenderContext* context);
	virtual ~SpriteBatch();

	void Begin();
	void End();

	void Draw(const Texture2DHandle texture, const vec2& postion, const Color& color = Colors::White);
	void Draw(const Texture2DHandle texture, const Rectangle& destination, const Color& color = Colors::White);
	void Draw(const Texture2DHandle texture, const Rectangle& source, const Rectangle& destination,
			  const Color& color = Colors::White, const FlipSprite flip = FlipSprite::none,
			  const vec2& origin = vec2{ 0.0f, 0.0f }, float rotation = 0.0f,
			  float layer = 0.0f);

private:
	GraphicsPipelineHandle defaultSpriteBatchPipeline;
	Buffer uniformBuffer;
	u32 uniformConstantsSize{};

public:
	struct SpriteInfo
	{
		Texture2DHandle texture;
		Rectangle source;
		Rectangle destination;
		FlipSprite flip;
		vec2 origin;
		float rotation;
		float layer;
		Color color;
	};
	std::vector<SpriteBatch::SpriteInfo> spriteInfos;

	// openGL specific fields
	GLuint vertexBuffer;
	GLuint vertexArrayObject;
	const u32 defaultBufferSize = 16 * 1024 * 1024;
	RenderContext* renderContext;
};