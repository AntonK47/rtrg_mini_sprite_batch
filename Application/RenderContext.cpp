#include "RenderContext.hpp"
#include "Color.hpp"
#include "ContentManager.hpp"

#include <assert.h>
#include <filesystem>
#include <fstream>
#include <sstream>

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
		case TextureFormat::bc_rgba_unorm:
			return GL_COMPRESSED_RGBA_BPTC_UNORM;
		}
		return 0;
	}

	u32 keyGenerator{ 0 };

	u32 GenerateKey()
	{
		return keyGenerator++;
	};

	TextAsset LoadText(const std::filesystem::path& asset)
	{
		auto fullPath = std::filesystem::path{ "Assets" } / asset;
		auto file = std::ifstream{ fullPath };
		auto stream = std::ostringstream{};
		stream << file.rdbuf();
		return TextAsset{ stream.str() };
	}
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

	fullscreenQuadPipeline = CreateGraphicsPipeline(GraphicsPipelineDescriptor{
		.vertexShaderCode = { LoadText("Shaders/FullscreenBlit.vert"), "Shaders/FullscreenBlit.vert" },
		.fragmentShaderCode = { LoadText("Shaders/FullscreenBlit.frag"), "Shaders/FullscreenBlit.frag" },
		.debugName = "fullscreenQuadPipeline" });

	defaultFramebuffer = CreateFramebuffer(FramebufferDescriptor{
		.colorAttachment = { Texture2DDescriptor{
			.extent = DynamicExtent{}, .format = TextureFormat::rgba8, .debugName = "default_color_render_target" } },
		.depthAttachment = Texture2DDescriptor{ .extent = DynamicExtent{},
												.format = TextureFormat::d32f,
												.debugName = "default_depth_render_target" },
		.debugName = "default_fb" });
}

RenderContext::~RenderContext()
{
	DestroyFramebuffer(defaultFramebuffer);
	DestroyGraphicsPipeline(fullscreenQuadPipeline);
}

void RenderContext::Blit()
{
	const auto& framebuffer = Get(defaultFramebuffer);
	const auto& colorTexture = Get(framebuffer.colorAttachment[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, windowContext.width, windowContext.height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindProgramPipeline(Get(fullscreenQuadPipeline).nativeHandle);
	glBindTextureUnit(0, colorTexture.nativeHandle);
	glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
	glDisable(GL_BLEND);
}

void RenderContext::Blit(const FramebufferHandle from, const u32 index)
{
	assert(index == 0);
	const auto& framebuffer = Get(from);
	const auto& colorTexture = Get(framebuffer.colorAttachment[index]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, windowContext.width, windowContext.height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindProgramPipeline(Get(fullscreenQuadPipeline).nativeHandle);
	glBindTextureUnit(0, colorTexture.nativeHandle);
	glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
	glDisable(GL_BLEND);
}

void RenderContext::Clear(const Color& color, const FramebufferHandle framebuffer)
{
	auto framebufferHandle = FramebufferHandle{};
	if (framebuffer == framebufferHandle)
	{
		framebufferHandle = defaultFramebuffer;
	}
	else
	{
		framebufferHandle = framebuffer;
	}
	const auto& fbo = Get(framebufferHandle);
	const auto colorClearValue = std::array{ 0.0f, 0.0f, 0.0f, 0.0f };
	glClearNamedFramebufferfv(fbo.nativeHandle, GL_COLOR, 0, colorClearValue.data());
	glClearNamedFramebufferfi(fbo.nativeHandle, GL_DEPTH_STENCIL, 0, 0.0f, 0);
	//TODO: clear dependent on framebuffer images

	{
		const auto clearColor = vec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.0f);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w,
					 clearColor.w);
		glClear(GL_COLOR_BUFFER_BIT);
	}
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
	auto width = u32{};
	auto height = u32{};
	const auto isDynamicExtent = std::holds_alternative<DynamicExtent>(descriptor.extent);
	if (isDynamicExtent)
	{
		const auto& extentScales = std::get<DynamicExtent>(descriptor.extent);
		width = static_cast<u32>(windowContext.width * extentScales.scaleWidth);
		height = static_cast<u32>(windowContext.height * extentScales.scaleHeight);
	}
	else
	{
		const auto& extent = std::get<StaticExtent>(descriptor.extent);
		width = extent.width;
		height = extent.height;
	}

	auto texture = Texture2D{};
	glCreateTextures(GL_TEXTURE_2D, 1, &texture.nativeHandle);
	glTextureParameteri(texture.nativeHandle, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(texture.nativeHandle, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureStorage2D(texture.nativeHandle, descriptor.levels, mapToGlFormat(descriptor.format),
					   static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	glObjectLabel(GL_TEXTURE, texture.nativeHandle, glLabel(descriptor.debugName));

	texture.width = width;
	texture.height = height;
	texture.levels = descriptor.levels;
	texture.format = descriptor.format;

	auto texture2DHandle = Texture2DHandle{};
	texture2DHandle.key = GenerateKey();
	textures[texture2DHandle] = texture;
	return texture2DHandle;
}

FramebufferHandle RenderContext::CreateFramebuffer(const FramebufferDescriptor& descriptor)
{
	const auto framebuffer = CreateOpenGlFramebuffer(descriptor);

	auto handle = FramebufferHandle{};
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

void RenderContext::UploadTextureData(const Texture2DHandle texture, const u8 level, void* data, size_t size)
{
	auto& nativeTexture = Get(texture);
	auto extent = glm::uvec2{ nativeTexture.width, nativeTexture.height };
	auto mipExtent = glm::uvec2(extent >> glm::uvec2{ static_cast<u32>(level) });
	mipExtent = glm::max(mipExtent, glm::uvec2(static_cast<u32>(1u)));

	glCompressedTextureSubImage2D(nativeTexture.nativeHandle, static_cast<GLint>(level), 0, 0,
								  static_cast<GLsizei>(mipExtent.x), static_cast<GLsizei>(mipExtent.y),
								  mapToGlFormat(nativeTexture.format), static_cast<GLsizei>(size), data);
}

GraphicsPipelineHandle RenderContext::CreateGraphicsPipeline(const GraphicsPipelineDescriptor& descriptor)
{
	const auto sourcesVertexShader = std::array{ descriptor.vertexShaderCode.code.c_str() };
	const auto sourcesFragmentShader = std::array{ descriptor.fragmentShaderCode.code.c_str() };

	const auto vertexProgram = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, sourcesVertexShader.data());
	glObjectLabel(GL_PROGRAM, vertexProgram, glLabel(descriptor.vertexShaderCode.debugName));
	const auto fragmentProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, sourcesFragmentShader.data());
	glObjectLabel(GL_PROGRAM, fragmentProgram, glLabel(descriptor.fragmentShaderCode.debugName));

	auto pipeline = GraphicsPipeline{};

	glCreateProgramPipelines(1, &pipeline.nativeHandle);

	glObjectLabel(GL_PROGRAM_PIPELINE, pipeline.nativeHandle, glLabel(descriptor.debugName));
	glUseProgramStages(pipeline.nativeHandle, GL_VERTEX_SHADER_BIT, vertexProgram);
	glUseProgramStages(pipeline.nativeHandle, GL_FRAGMENT_SHADER_BIT, fragmentProgram);

	glDeleteProgram(vertexProgram);
	glDeleteProgram(fragmentProgram);
#if 0
		auto validateProgram = [](GLuint program)
			{
				int success;
				glGetProgramiv(program, GL_LINK_STATUS, &success);
				if (!success)
				{
					char infoLog[512];
					glGetProgramInfoLog(program, 512, NULL, infoLog);
					std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
				}
			};

		validateProgram(fooVertProgram1);
		validateProgram(fooFragProgram1);
#endif

	auto pipelineHandle = GraphicsPipelineHandle{};
	pipelineHandle.key = GenerateKey();
	pipelines[pipelineHandle] = pipeline;
	return pipelineHandle;
}

void RenderContext::DestroyGraphicsPipeline(const GraphicsPipelineHandle graphicsPipeline)
{
	const auto& pipeline = Get(graphicsPipeline);

	glDeleteProgramPipelines(1, &pipeline.nativeHandle);

	pipelines.erase(graphicsPipeline);
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
