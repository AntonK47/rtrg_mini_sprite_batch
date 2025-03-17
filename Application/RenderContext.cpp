#include "RenderContext.hpp"
#include <assert.h>

namespace
{
	GLenum mapToGlFormat(const TextureFormat& format)
	{
		assert(format != TextureFormat::unknown);
		switch (format)
		{
		case TextureFormat::rgba8:
			return GL_RGBA8;
		case TextureFormat::d32f:
			return GL_DEPTH_COMPONENT32F;
		}
		return 0;
	}

	u32 keyGenerator{ 0 };

	u32 GenerateKey()
	{
		return keyGenerator++;
	};


} // namespace

Framebuffer RenderContext::CreateOpenGlFramebuffer(const FramebufferDescriptor& descriptor)
{
	auto isWindowsSizeDependent = false;
	Framebuffer framebuffer;

	for (auto i = 0; i < descriptor.colorAttachment.size(); i++)
	{
		auto& colorAttachmentDescriptor = descriptor.colorAttachment[i];
		isWindowsSizeDependent |= std::holds_alternative<DynamicExtent>(colorAttachmentDescriptor.extent);
		auto colorAttachment = CreateTexture2D(colorAttachmentDescriptor);
		framebuffer.colorAttachment[i] = colorAttachment;
	}

	if (descriptor.depthAttachment.has_value())
	{
		auto& depthAttachmentDescriptor = descriptor.depthAttachment.value();
		isWindowsSizeDependent |= std::holds_alternative<DynamicExtent>(depthAttachmentDescriptor.extent);
		auto depthAttachment = CreateTexture2D(depthAttachmentDescriptor);
		framebuffer.depthAttachment = depthAttachment;
	}

	framebuffer.isSizeDependent = isWindowsSizeDependent;

	glCreateFramebuffers(1, &framebuffer.nativeHandle);
	glObjectLabel(GL_FRAMEBUFFER, framebuffer.nativeHandle, glLabel(descriptor.debugName));
	for (auto i = 0; i < descriptor.colorAttachment.size(); i++)
	{
		const auto& texture = Get(framebuffer.colorAttachment[i]);
		glNamedFramebufferTexture(framebuffer.nativeHandle, GL_COLOR_ATTACHMENT0 + i, texture.nativeHandle, 0);
	}

	if (descriptor.depthAttachment.has_value())
	{
		const auto& texture = Get(framebuffer.depthAttachment.value());
		glNamedFramebufferTexture(framebuffer.nativeHandle, GL_DEPTH_ATTACHMENT, texture.nativeHandle, 0);
	}
	return framebuffer;
}


void RenderContext::UpdateWindowSize(const u32 width, const u32 height)
{
	assert(width > 0 && height > 0);
	auto sizeHasChanged = windowContext.width != width || windowContext.height != height;
	if (sizeHasChanged)
	{
		windowContext.height = height;
		windowContext.width = width;
		RecreateWindowSizeDependentResources();
	}
}

RenderContext::RenderContext(const u32 width, const u32 height)
{
	windowContext = { .width = width, .height = height };
}

RenderContext::~RenderContext()
{
}

void RenderContext::RecreateWindowSizeDependentResources()
{
	for (auto& [key, sizeDependentFramebuffer] : windowSizeDependentFramebuffers)
	{
		DestroyOpenGlFramebuffer(sizeDependentFramebuffer.framebuffer);
		auto framebuffer = CreateOpenGlFramebuffer(sizeDependentFramebuffer.descriptor);
		windowSizeDependentFramebuffers.at(key).framebuffer = framebuffer;
	}
}

Texture2DHandle RenderContext::CreateTexture2D(const Texture2DDescriptor& descriptor)
{
	u32 width;
	u32 height;
	const auto isDynamicExtent = std::holds_alternative<DynamicExtent>(descriptor.extent);
	if (isDynamicExtent)
	{
		const auto extentScales = std::get<DynamicExtent>(descriptor.extent);
		width = static_cast<u32>(windowContext.width * extentScales.scaleWidth);
		height = static_cast<u32>(windowContext.height * extentScales.scaleHeight);
	}
	else
	{
		const auto extent = std::get<StaticExtent>(descriptor.extent);
		width = extent.width;
		height = extent.height;
	}

	Texture2D texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture.nativeHandle);
	glTextureParameteri(texture.nativeHandle, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(texture.nativeHandle, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureStorage2D(texture.nativeHandle, 1, mapToGlFormat(descriptor.format), static_cast<GLsizei>(width),
					   static_cast<GLsizei>(height));
	glObjectLabel(GL_TEXTURE, texture.nativeHandle, glLabel(descriptor.debugName));

	Texture2DHandle texture2DHandle;
	texture2DHandle.key = GenerateKey();
	textures[texture2DHandle] = texture;
	return texture2DHandle;
}

FramebufferHandle RenderContext::CreateFramebuffer(const FramebufferDescriptor& descriptor)
{
	Framebuffer framebuffer = CreateOpenGlFramebuffer(descriptor);


	FramebufferHandle handle;
	handle.key = GenerateKey();

	if (framebuffer.isSizeDependent)
	{
		windowSizeDependentFramebuffers[handle] = { framebuffer, descriptor };
	}
	else
	{
		framebuffers[handle] = framebuffer;
	}
	return handle;
}

void RenderContext::DestroyTexture2D(const Texture2DHandle texture)
{
	const auto& textureObject = Get(texture);
	glDeleteTextures(1, &textureObject.nativeHandle);
	textures.erase(texture);
}

void RenderContext::DestroyOpenGlFramebuffer(const Framebuffer& framebuffer)
{
	for (auto i = 0; i < framebuffer.colorAttachment.size(); i++)
	{
		DestroyTexture2D(framebuffer.colorAttachment[i]);
	}

	if (framebuffer.depthAttachment.has_value())
	{
		DestroyTexture2D(framebuffer.depthAttachment.value());
	}

	glDeleteFramebuffers(1, &framebuffer.nativeHandle);
}

void RenderContext::DestroyFramebuffer(const FramebufferHandle framebuffer)
{
	const auto& framebufferObject = Get(framebuffer);

	DestroyOpenGlFramebuffer(framebufferObject);

	framebufferObject.isSizeDependent ? windowSizeDependentFramebuffers.erase(framebuffer) :
										framebuffers.erase(framebuffer);
}

GraphicsPipelineHandle RenderContext::CreateGraphicsPipeline(const GraphicsPipelineDescriptor& descriptor)
{
	// TODO
	return GraphicsPipelineHandle();
}

void RenderContext::DestroyGraphicsPipeline(const GraphicsPipelineHandle graphicsPipeline)
{
	// TODO
}

Framebuffer& RenderContext::Get(FramebufferHandle handle)
{
	return windowSizeDependentFramebuffers[handle].framebuffer;
}

Texture2D& RenderContext::Get(Texture2DHandle handle)
{
	return textures[handle];
}

GraphicsPipeline& RenderContext::Get(GraphicsPipelineHandle handle)
{
	return pipelines[handle];
}
