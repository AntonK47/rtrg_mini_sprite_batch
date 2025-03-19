#include "ContentManager.hpp"
#include <gli/gli.hpp>

#include <filesystem>
#include <fstream>
#include "RenderContext.hpp"

struct ContentManager::ContentManagerImpl
{
	ContentManagerImpl(RenderContext& context) : renderContext{ context }
	{
	}

	Texture2DHandle LoadTexture(const std::filesystem::path& asset)
	{
		auto mapGliToTextureFormat =
			[](gli::gl::format format)
		{
			if (format.Internal == gli::gl::INTERNAL_RGB_BP_UNORM)
			{
				return TextureFormat::bc_rgba_unorm;
			}
			assert(false);
			return TextureFormat::unknown;
		};

		const auto textureData = gli::texture2d(gli::load(asset.generic_string()));
		const auto gl = gli::gl(gli::gl::PROFILE_GL33);
		const auto format = gl.translate(textureData.format(), textureData.swizzles());
		const auto mips = textureData.levels();
		const auto width = textureData.extent().x;
		const auto height = textureData.extent().y;

		auto textureHandle = renderContext.CreateTexture2D(
			Texture2DDescriptor{ .extent = StaticExtent{ .width = (u32)width, .height = (u32)height },
								 .format = mapGliToTextureFormat(format),
								 .levels = (u8)mips,
								 .debugName = asset.generic_string().c_str() });

		for (auto i = 0; i < mips; i++)
		{
			renderContext.UploadTextureData(textureHandle, i, textureData[i].data(), textureData[i].size());
		}
		return textureHandle;
	}

	TextAsset LoadText(const std::filesystem::path& asset)
	{
		auto file = std::ifstream{ asset };
		auto stream = std::ostringstream{};
		stream << file.rdbuf();
		return TextAsset{ stream.str() };
	}

	RenderContext& renderContext;
};


ContentManager::ContentManager(RenderContext* renderContext, const std::string_view assetRootPath)
	: assetRootPath{ assetRootPath }
{
	impl = std::make_unique<ContentManager::ContentManagerImpl>(*renderContext);
}

ContentManager::~ContentManager()
{
}

TextAsset ContentManager::LoadText(const std::string_view asset)
{
	return impl->LoadText(std::filesystem::path{ assetRootPath } / asset);
}

Texture2DHandle ContentManager::LoadTexture(const std::string_view asset)
{
	return impl->LoadTexture(std::filesystem::path{ assetRootPath } / asset);
}
