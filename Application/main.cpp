#include "SampleGame.hpp"

#include "MapImporter.hpp"

int main(int argc, char* argv[])
{
	ImportMap();

	auto game = SampleGame{};
	game.Run(argc, argv);

	return 0;
}