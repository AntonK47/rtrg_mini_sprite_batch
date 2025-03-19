#pragma once
#include <array>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Common.hpp"
#include "RenderResources.hpp"



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

	// TODO:
	GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDescriptor& descriptor);
	void DestroyGraphicsPipeline(const GraphicsPipelineHandle graphicsPipeline);

	Framebuffer& Get(FramebufferHandle handle);
	Texture2D& Get(Texture2DHandle handle);
	GraphicsPipeline& Get(GraphicsPipelineHandle handle);

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
};