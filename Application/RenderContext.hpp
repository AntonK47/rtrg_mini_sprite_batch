#pragma once
#include <array>

#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Common.hpp"

struct RenderContext;

template <typename T>
struct Handle
{
	friend RenderContext;
	friend std::hash<Handle<T>>;

	auto operator<=>(const Handle<T>&) const = default;

private:
	u32 key;
};

template <typename T>
struct std::hash<Handle<T>>
{
	std::size_t operator()(const Handle<T>& s) const noexcept
	{
		return std::hash<unsigned int>()(s.key);
	}
};

struct Texture2D;
struct Framebuffer;
struct GraphicsPipeline;

using Texture2DHandle = Handle<Texture2D>;
using FramebufferHandle = Handle<Framebuffer>;
using GraphicsPipelineHandle = Handle<GraphicsPipeline>;


enum class TextureFormat
{
	unknown,
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
};

struct Framebuffer
{
	GLuint nativeHandle{ 0 };
	std::array<Texture2DHandle, 1> colorAttachment{};
	std::optional<Texture2DHandle> depthAttachment{ std::nullopt };
	bool isSizeDependent{ false };
};

struct FramebufferDescriptor
{
	std::array<Texture2DDescriptor, 1> colorAttachment;
	std::optional<Texture2DDescriptor> depthAttachment;
	const char* debugName = "";
};


struct WindowSizeDependentFramebuffer
{
	Framebuffer framebuffer;
	FramebufferDescriptor descriptor;
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
};

struct GraphicsPipeline
{
};


struct WindowContext
{
	u32 width;
	u32 height;
};

struct RenderContext
{
	void UpdateWindowSize(const u32 width, const u32 height);

	RenderContext(const u32 width, const u32 height);
	virtual ~RenderContext();

	Texture2DHandle CreateTexture2D(const Texture2DDescriptor& descriptor);
	FramebufferHandle CreateFramebuffer(const FramebufferDescriptor& descriptor);

	void DestroyTexture2D(const Texture2DHandle texture);
	void DestroyFramebuffer(const FramebufferHandle framebuffer);

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
	// TODO: Maybe better resource managment, for example by using pools, resource invalidation, decoupled resouce
	// managment class/struct
	std::unordered_map<FramebufferHandle, Framebuffer> framebuffers;
	std::unordered_map<FramebufferHandle, WindowSizeDependentFramebuffer> windowSizeDependentFramebuffers;
	std::unordered_map<Texture2DHandle, Texture2D> textures;
	std::unordered_map<GraphicsPipelineHandle, GraphicsPipeline> pipelines;
};