#pragma once
#include <array>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Color.hpp"
#include "Common.hpp"
#include "RenderResources.hpp"

/*================================== SOME IDEAS==============================*/
#include <string_view>

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

protected:
	void SetFramebuffer(const FramebufferHandle frambuffer);
	void SetUniformTexture(const std::string_view uniformTextureName, const Texture2DHandle texture);
	void SetUniformBlock(const std::string_view uniformBlockName, UniformBlock uniformBlock);

private:
	friend SpriteBatch;
	FramebufferHandle framebuffer;
	GraphicsPipelineHandle pso;
	DynamicUniformAllocator allocator;
};

struct DefaultSpriteBatchEffect : Effect
{
	DefaultSpriteBatchEffect(RenderContext* context)
		: Effect{ context, "Shaders/DefaultSpriteBatch.vert", "Shaders/DefaultSpriteBatch.frag" }
	{
	}
};

struct CustomEffect
{
	CustomEffect(RenderContext* context, const std::string_view fragmentShaderAsset,
				 const std::string_view vertexShaderAsset = "Shaders/DefaultSpriteBatch.vert")
	{
	}
};

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


struct RenderContext
{
	void UpdateWindowSize(const u32 width, const u32 height);

	RenderContext(const u32 width, const u32 height);
	virtual ~RenderContext();

	Texture2DHandle CreateTexture2D(const Texture2DDescriptor& descriptor);
	FramebufferHandle CreateFramebuffer(const FramebufferDescriptor& descriptor);

	void DestroyTexture2D(const Texture2DHandle texture);
	void DestroyFramebuffer(const FramebufferHandle framebuffer);

	void UploadTextureData(const Texture2DHandle texture, const u8 level, void* data, size_t size);

	GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDescriptor& descriptor);
	void DestroyGraphicsPipeline(const GraphicsPipelineHandle graphicsPipeline);

	Framebuffer& Get(FramebufferHandle handle);
	Texture2D& Get(Texture2DHandle handle);
	GraphicsPipeline& Get(GraphicsPipelineHandle handle);

	WindowContext GetWindowsContext() const
	{
		return windowContext;
	}

	void Clear(const Color& color);
	void Blit();
	FramebufferHandle GetDefaultFramebuffer() const
	{
		return defaultFramebuffer;
	}

private:
	void RecreateWindowSizeDependentResources();

	Framebuffer CreateOpenGlFramebuffer(const FramebufferDescriptor& descriptor);
	void DestroyOpenGlFramebuffer(const Framebuffer& framebuffer);

	WindowContext windowContext;
	// TODO: Maybe better resource management, for example by using pools, resource invalidation, decoupled resource
	// management class/struct
	std::unordered_map<FramebufferHandle, Framebuffer> framebuffers;
	std::unordered_map<FramebufferHandle, WindowSizeDependentFramebuffer> windowSizeDependentFramebuffers;
	std::unordered_map<Texture2DHandle, Texture2D> textures;
	std::unordered_map<GraphicsPipelineHandle, GraphicsPipeline> pipelines;


	GraphicsPipelineHandle fullscreenQuadPipeline;
	FramebufferHandle defaultFramebuffer;
};