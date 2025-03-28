#pragma once

#include "Common.hpp"
#include <optional>
#include <array>
#include <string>
#include <variant>

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
	d32f,
	bc_rgba_unorm
};

struct StaticExtent
{
	u32 width;
	u32 height;
};

struct DynamicExtent
{
	f32 scaleWidth{ 1.0f };
	f32 scaleHeight{ 1.0f };
};

using Extent = std::variant<StaticExtent, DynamicExtent>;

struct Texture2DDescriptor
{
	Extent extent{};
	TextureFormat format{TextureFormat::unknown};
	u8 levels = 1;
	const char* debugName = "";
};

struct Texture2D
{
	GLuint nativeHandle;
	u32 width;
	u32 height;
	TextureFormat format;
	u8 levels;
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
	std::string code;
	const char* debugName = "";
};

struct GraphicsPipelineDescriptor
{
	ShaderCode vertexShaderCode;
	ShaderCode fragmentShaderCode;
	const char* debugName = "";
};

struct GraphicsPipeline
{
	GLuint nativeHandle;
};


struct WindowContext
{
	u32 width;
	u32 height;
};