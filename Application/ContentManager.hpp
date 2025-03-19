#pragma once

#include <memory>
#include <string_view>
#include <string>
#include "RenderResources.hpp"

struct RenderContext;

using TextAsset = std::string;

struct ContentManager
{
public:
	ContentManager(RenderContext* renderContext, const std::string_view assetRootPath);
	virtual ~ContentManager();

	Texture2DHandle LoadTexture(const std::string_view asset);
	TextAsset LoadText(const std::string_view asset);

	std::string assetRootPath{};
	
private:
	struct ContentManagerImpl;
	std::unique_ptr<ContentManagerImpl> impl;
};