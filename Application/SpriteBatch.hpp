#pragma once

#include <vector>

#include "Color.hpp"
#include "Common.hpp"
#include "RenderResources.hpp"

struct RenderContext;

struct Buffer
{
	GLuint nativeHandle{};
	void* mappedPtr{ nullptr };
};

struct SpriteBatchConstants
{
	vec2 viewportSize;
	vec2 pad;
	mat4 transform;
};

enum class FlipSprite
{
	none,
	horizontal,
	vertical,
	horizontalAndVertical
};

struct Effect;

struct SpriteBatch
{
	SpriteBatch(RenderContext* context);
	virtual ~SpriteBatch();

	void Begin(const mat3& transform = mat3{ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f }, Effect* effect = nullptr);
	void End();

	void Draw(const Texture2DHandle texture, const vec2& postion, const Color& color = Colors::White);
	void Draw(const Texture2DHandle texture, const Rectangle& destination, const Color& color = Colors::White);
	void Draw(const Texture2DHandle texture, const Rectangle& source, const Rectangle& destination,
			  const Color& color = Colors::White, const FlipSprite flip = FlipSprite::none,
			  const vec2& origin = vec2{ 0.0f, 0.0f }, float rotation = 0.0f, float layer = 0.0f);

private:
	GraphicsPipelineHandle defaultSpriteBatchPipeline;
	Buffer uniformBuffer;
	u32 uniformConstantsSize{};
	
	struct SpriteQuadVertex
	{
		vec2 position;
		vec2 uv;
		vec4 color;
	};
	std::vector<SpriteQuadVertex> generatedVertices;
	struct Batch
	{
		Texture2DHandle texture;
		u32 vertexOffset;
		u32 vertexCount;
	};
	std::vector<Batch> batches;

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