#include <cstdlib>
#include <exception>
#include <iostream>

#include <SDL2/SDL.h>

#include "engine/window/Window.h"
#include "engine/graphics/renderer/Renderer.h"

int main(const int argc, char* argv[])
try
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		throw std::runtime_error("Failed to initialise SDL.");
	}

	Window window("Terrain Generator", glm::uvec2{ 1280, 720 });
	Renderer renderer(window);

	bool isRunning = true;
	bool isPaused = false;
	SDL_Event event{ };

	while (isRunning)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_F11:
					window.ToggleFullscreen();

					break;

				case SDLK_ESCAPE:
					isRunning = false;

					break;

				default:
					break;
				}

				break;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					window.ProcessResize(glm::uvec2{ event.window.data1, event.window.data2 });
					
					[[fallthrough]];

				case SDL_WINDOWEVENT_MINIMIZED:
					renderer.ProcessWindowResize();

					break;

				case SDL_WINDOWEVENT_FOCUS_LOST:
					isPaused = true;

					break;

				case SDL_WINDOWEVENT_FOCUS_GAINED:
					isPaused = false;

					break;

				default:
					break;
				}

				break;

			case SDL_QUIT:
				isRunning = false;

				break;

			default:
				break;
			}
		}

		if (!isPaused)
		{
			if (renderer.PrepareRender())
			{
				renderer.BeginRender();
				{

				}
				renderer.EndRender();

				renderer.Present();
			}
		}
	}
	
	renderer.FinaliseRenderOperations();

	window.Destroy();
	SDL_Quit();

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