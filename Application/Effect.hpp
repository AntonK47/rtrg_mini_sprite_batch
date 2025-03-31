#pragma once

#include "Common.hpp"
#include "RenderResources.hpp"

#include <string_view>

/*================================== SOME IDEAS==============================*/
/*=======USAGE========*/
/*

	auto gbuffer = renderContext->CreateFramebuffer(...);
	auto customEffect = CustomEffect(renderContext, "shader.frag", "shader.vert")

	//frame loop
	//Deferred style usage
	customEffect.SetFramebuffer(gbuffer);
	customEffect.SetUniformTexture("texture_name_01", texture_xyz);
	customEffect.SetUniformBlock("light_data", light_data);

	const auto transform = mat3{...};
	spriteBatch.Begin(customEffect, transform);
	spriteBatch.Draw(...);
	spriteBatch.End();

	//or more immediate style like in XNA/MonoGame
	customEffect.Begin(...);
	customEffect.SetFramebuffer(...);
	customEffect.SetUniformTexture(...);
	customEffect.SetUniformBlock(...);
	spriteBatch.Begin();
	spriteBatch.Draw(...);
	spriteBatch.End();
	customEffect.SetUniformTexture(...);
	customEffect.SetUniformBlock(...);
	spriteBatch.Begin();
	spriteBatch.Draw(...);
	spriteBatch.End();
	customEffect.End()
*/
/*=======IDEAS========*/
/*

	For PBR-Like sprites we can bundle them into one SpriteTexture and use slots context dependent

	struct SpriteTexture
	{
		Texture2DHandle slot0;
		Texture2DHandle slot1;
		Texture2DHandle slot2;
		Texture2DHandle slot3;

		//or as array??
	}

*/

struct UniformBlock
{
	u32 offset;
	u32 size;
};

struct DynamicUniformAllocator
{
	template <typename T>
	UniformBlock allocate();
};

struct RenderContext;
struct SpriteBatch;

struct Effect
{
	Effect(RenderContext* context, const std::string_view fragmentShaderAsset,
		   const std::string_view vertexShaderAsset);

public:
	void SetFramebuffer(const FramebufferHandle framebuffer);
	void SetUniformTexture(const std::string_view uniformTextureName, const Texture2DHandle texture);
	void SetUniformBlock(const std::string_view uniformBlockName, UniformBlock uniformBlock);

private:
	friend SpriteBatch;
	FramebufferHandle fbo{};
	GraphicsPipelineHandle pso{};
	DynamicUniformAllocator allocator{};
};

struct DefaultSpriteBatchEffect : Effect
{
	DefaultSpriteBatchEffect(RenderContext* context)
		: Effect{ context, "Shaders/DefaultSpriteBatch.frag", "Shaders/DefaultSpriteBatch.vert" }
	{
	}
};

//struct CustomEffect
//{
//	CustomEffect(RenderContext* context, const std::string_view fragmentShaderAsset,
//				 const std::string_view vertexShaderAsset = "Shaders/DefaultSpriteBatch.vert")
//	{
//	}
//};

template <typename T>
inline UniformBlock DynamicUniformAllocator::allocate()
{
	return UniformBlock();
}
