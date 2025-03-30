#include "ContentManager.hpp"

#pragma warning(push)
#pragma warning(disable: 26495)
#include <gli/gli.hpp>
#pragma warning(pop)
#include <filesystem>
#include <fstream>
#include <string>
#include "RenderContext.hpp"

#include <memory>

#include <vfspp/NativeFileSystem.hpp>
#include <vfspp/VirtualFileSystem.hpp>
#include <vfspp/ZipFileSystem.hpp>

#include <curl/curl.h>

namespace
{
	size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata)
	{
		size_t totalSize = size * nitems;
		std::string header(buffer, totalSize);

		if (header.find("Content-Length:") != std::string::npos)
		{
			long* fileSize = static_cast<long*>(userdata);
			*fileSize = std::stol(header.substr(header.find(":") + 1));
		}
		return totalSize;
	}
} // namespace

struct ContentManager::ContentManagerImpl
{
	ContentManagerImpl(RenderContext& context, const std::string_view assetRootPath) : renderContext{ context }
	{
#ifdef ENABLE_ASSETS_DOWNLOAD
		if (EnableResourceFileDownload)
		{
			using namespace std::string_literals;
			const auto url = "https://drive.google.com/uc?id=1WWYTPwFHTFxR5M0_a58ysKh9He98wcE8&export=download"s;
			const auto targetFile = std::filesystem::path{ assetRootPath } / "package_00";

			auto curl = curl_easy_init();
			if (curl)
			{
				auto skipDownload = false;

				if (std::filesystem::exists(std::filesystem::path{ assetRootPath } / "package_00"))
				{
					long fileSize = -1;

					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
					curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
					curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
					curl_easy_setopt(curl, CURLOPT_HEADERDATA, &fileSize);
					curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

					[[maybe_unused]] const auto result = curl_easy_perform(curl);
					assert(result == CURLE_OK);

					auto in = std::ifstream(targetFile, std::ifstream::ate | std::ifstream::binary);
					const auto localFileSize = in.tellg();

					skipDownload = localFileSize == fileSize;
				}

				if (not skipDownload)
				{
					FILE* file{ nullptr };
					fopen_s(&file, targetFile.generic_string().c_str(), "wb");
					if (file)
					{
						curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
						curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
						curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
						curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
						[[maybe_unused]] const auto result = curl_easy_perform(curl);
						assert(result == CURLE_OK);

						fclose(file);
					}
				}
				curl_easy_cleanup(curl);
			}
		}
#endif
		auto packaged = std::make_unique<vfspp::ZipFileSystem>(std::string{ assetRootPath } + "/package_00");
		auto unpackaged = std::make_unique<vfspp::NativeFileSystem>(std::string{ assetRootPath });

		packaged->Initialize();
		unpackaged->Initialize();

		vfs = std::make_unique<vfspp::VirtualFileSystem>();
		vfs->AddFileSystem(std::string{ assetRootPath }, std::move(packaged));
		vfs->AddFileSystem(std::string{ assetRootPath }, std::move(unpackaged));
	}

	Texture2DHandle LoadTexture(const std::filesystem::path& asset)
	{
		auto mapGliToTextureFormat = [](gli::gl::format format)
		{
			if (format.Internal == gli::gl::INTERNAL_RGB_BP_UNORM)
			{
				return TextureFormat::bc_rgba_unorm;
			}
			assert(false);
			return TextureFormat::unknown;
		};

		auto textureFile = vfs->OpenFile(vfspp::FileInfo(asset.generic_string()), vfspp::IFile::FileMode::Read);

		assert(textureFile);
		const auto binarySize = textureFile->Size();
		auto binaryData = std::vector<char>{};
		binaryData.resize(binarySize);

		textureFile->Read((uint8_t*)binaryData.data(), binarySize);
		textureFile->Close();

		const auto textureData = gli::texture2d(gli::load(binaryData.data(), binarySize));
		const auto gl = gli::gl(gli::gl::PROFILE_GL33);
		const auto format = gl.translate(textureData.format(), textureData.swizzles());
		const auto mips = textureData.levels();
		const auto width = textureData.extent().x;
		const auto height = textureData.extent().y;

		auto descriptor = Texture2DDescriptor{};
		descriptor.extent = StaticExtent{ .width = (u32)width, .height = (u32)height };
		descriptor.format = mapGliToTextureFormat(format);
		descriptor.levels = static_cast<u8>(mips);
		const auto assetString = asset.generic_string();
		descriptor.debugName = assetString.c_str();
		auto textureHandle = renderContext.CreateTexture2D(descriptor);

		for (auto i = 0; i < mips; i++)
		{
			renderContext.UploadTextureData(textureHandle, (u8)i, textureData[i].data(), textureData[i].size());
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
	std::unique_ptr<vfspp::VirtualFileSystem> vfs{};
};


ContentManager::ContentManager(RenderContext* renderContext, const std::string_view assetRootPath)
	: assetRootPath{ assetRootPath }
{
	impl = std::make_unique<ContentManager::ContentManagerImpl>(*renderContext, assetRootPath);
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
