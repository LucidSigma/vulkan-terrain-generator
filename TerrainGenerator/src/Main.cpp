#include <cstdlib>
#include <exception>
#include <iostream>

#include <SDL2/SDL.h>

#include "terrain_generator/TerrainGenerator.h"

int main(const int argc, char* argv[])
try
{
	TerrainGenerator terrainGenerator;
	terrainGenerator.Run();

	return EXIT_SUCCESS;
}
catch (const std::exception& error)
{
	std::cerr << error.what() << "\n";
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error.what(), nullptr);

	std::cin.get();

	return EXIT_FAILURE;
}
catch (...)
{
	std::cerr << "Unknown error.\n";
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Unknown error occurred.", nullptr);

	std::cin.get();

	return EXIT_FAILURE;
}