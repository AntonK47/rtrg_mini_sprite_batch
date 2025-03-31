#include "Effect.hpp"
#include "ContentManager.hpp"
#include "RenderContext.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
	TextAsset LoadText(const std::filesystem::path& asset)
	{
		auto fullPath = std::filesystem::path{ "Assets" } / asset;
		auto file = std::ifstream{ fullPath };
		auto stream = std::ostringstream{};
		stream << file.rdbuf();
		return TextAsset{ stream.str() };
	}
} // namespace

Effect::Effect(RenderContext* context, const std::string_view fragmentShaderAsset,
			   const std::string_view vertexShaderAsset)
{
	pso = context->CreateGraphicsPipeline(GraphicsPipelineDescriptor{
		.vertexShaderCode = { LoadText(vertexShaderAsset), vertexShaderAsset.data() },
		.fragmentShaderCode = { LoadText(fragmentShaderAsset), fragmentShaderAsset.data() },
		.debugName = "DefaultSpriteBatchPipeline" });
}

void Effect::SetFramebuffer(const FramebufferHandle framebuffer)
{
	fbo = framebuffer;
}

void Effect::SetUniformTexture([[maybe_unused]] const std::string_view uniformTextureName,
							   [[maybe_unused]] const Texture2DHandle texture)
{
}

void Effect::SetUniformBlock([[maybe_unused]] const std::string_view uniformBlockName,
							 [[maybe_unused]] UniformBlock uniformBlock)
{
}
