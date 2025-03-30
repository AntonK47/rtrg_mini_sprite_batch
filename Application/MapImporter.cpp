#include "MapImporter.hpp"
#include <fstream>

tiled::TiledMap ImportMap(const std::filesystem::path file)
{
	assert(std::filesystem::exists(file));
	auto f = std::ifstream(file);
	auto data = nlohmann::json::parse(f);
	return data.get<tiled::TiledMap>();
}
