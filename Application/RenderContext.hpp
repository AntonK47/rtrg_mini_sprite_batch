#pragma once
#include <array>

#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Common.hpp"

struct Texture2DHandle
{
	u32 key;
	auto operator<=>(const Texture2DHandle&) const = default;
};

template <>
struct std::hash<Texture2DHandle>
{
	std::size_t operator()(const Texture2DHandle& s) const noexcept
	{
		return std::hash<int>()(s.key);
	}
};

struct FramebufferHandle
{
	u32 key;
	auto operator<=>(const FramebufferHandle&) const = default;
};

template <>
struct std::hash<FramebufferHandle>
{
	std::size_t operator()(const FramebufferHandle& s) const noexcept
	{
		return std::hash<int>()(s.key);
	}
};


enum class TextureFormat
{
	rgba8,
	d32f
};


struct StaticExtent
{
	u32 width;
	u32 height;
};

struct DynamicExtent
{
	float scaleWidth{ 1.0f };
	float scaleHeight{ 1.0f };
};

using Extent = std::variant<StaticExtent, DynamicExtent>;

struct Texture2DDescriptor
{
	Extent extent;
	TextureFormat format;
	const char* debugName = "";
};

struct Texture2D
{
	GLuint nativeHandle;

	auto operator<=>(const Texture2D&) const = default;
};

struct Framebuffer
{
	GLuint nativeHandle;
	std::array<Texture2DHandle, 1> colorAttachment;
	std::optional<Texture2DHandle> depthAttachment;
	bool isSizeDependent{ false };

	auto operator<=>(const Framebuffer&) const = default;
};

struct FramebufferDescriptor
{
	std::array<Texture2DDescriptor, 1> colorAttachment;
	std::optional<Texture2DDescriptor> depthAttachment;
	const char* debugName = "";

	auto operator<=>(const FramebufferDescriptor&) const = default;
};


struct WindowSizeDependentFramebuffer
{
	Framebuffer framebuffer;
	FramebufferDescriptor descriptor;

	auto operator<=>(const WindowSizeDependentFramebuffer&) const = default;
};

struct ShaderCode
{
	const char* code;
	const char* debugName = "";
};

struct GraphicsPipelineDescriptor
{
	ShaderCode vertexShaderCode;
	ShaderCode fragmentSchaderCode;
	const char* debugName = "";

	auto operator<=>(const GraphicsPipelineDescriptor&) const = default;
};

struct GraphicsPipeline
{
	auto operator<=>(const GraphicsPipeline&) const = default;
};

struct GraphicsPipelineHandle
{
	u32 key;

	auto operator<=>(const GraphicsPipelineHandle&) const = default;
};


template <>
struct std::hash<GraphicsPipelineHandle>
{
	std::size_t operator()(const GraphicsPipelineHandle& s) const noexcept
	{
		return std::hash<int>()(s.key);
	}
};


struct RenderContext
{
	struct WindowContext
	{
		u32 width;
		u32 height;
		RenderContext* renderContext;

		void UpdateSize(const u32 width, const u32 height);
	};

	WindowContext windowContext;

	RenderContext(const u32 width, const u32 height);
	virtual ~RenderContext();

	void RecreateWindowSizeDependentResources();
	Texture2DHandle CreateTexture2D(const Texture2DDescriptor& descriptor);
	FramebufferHandle CreateFramebuffer(const FramebufferDescriptor& descriptor);

	void DestroyTexture2D(const Texture2DHandle texture);
	void DestroyFramebuffer(const FramebufferHandle framebuffer);

	// TODO:
	GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDescriptor& descriptor);

	Framebuffer& Get(FramebufferHandle handle);
	Texture2D& Get(Texture2DHandle handle);
	GraphicsPipeline& Get(GraphicsPipelineHandle handle);

private:
	Framebuffer CreateOpenGlFramebuffer(const FramebufferDescriptor& descriptor);
	void DestroyOpenGlFramebuffer(const Framebuffer& framebuffer);

	std::unordered_map<FramebufferHandle, Framebuffer> framebuffers;

	std::unordered_map<FramebufferHandle, WindowSizeDependentFramebuffer> windowSizeDependentFramebuffers;

	std::unordered_map<Texture2DHandle, Texture2D> textures;

	std::unordered_map<GraphicsPipelineHandle, GraphicsPipeline> pipelines;
};