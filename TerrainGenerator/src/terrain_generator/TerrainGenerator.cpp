#include "TerrainGenerator.h"

TerrainGenerator::TerrainGenerator()
{
	Initialise();
}

TerrainGenerator::~TerrainGenerator() noexcept
{
	Destroy();
}

void TerrainGenerator::Run()
{
	while (m_isRunning)
	{
		PollEvents();

		if (!m_isPaused)
		{
			ProcessInput();
			Update();
			Render();
		}
	}
}

void TerrainGenerator::Initialise()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		throw std::runtime_error("Failed to initialise SDL.");
	}

	m_window.Create("Terrain Generator", s_InitialWindowSize);
	m_renderer = std::make_unique<Renderer>(m_window);

	if (SDL_CaptureMouse(SDL_TRUE) != 0)
	{
		throw std::runtime_error("Failed to capture mouse cursor.");
	}

	if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0)
	{
		throw std::runtime_error("Failed to enable relative mouse mode.");
	}

	SDL_GetRelativeMouseState(nullptr, nullptr);

	m_world = std::make_unique<World>(*m_renderer, m_window);
}

void TerrainGenerator::Destroy() noexcept
{
	m_renderer->FinaliseRenderOperations();
	
	m_world = nullptr;
	m_renderer = nullptr;

	m_window.Destroy();
	SDL_Quit();
}

void TerrainGenerator::PollEvents()
{
	SDL_Event event{ };

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym)
			{
			case SDLK_F11:
				m_window.ToggleFullscreen();

				break;

			case SDLK_ESCAPE:
				m_isRunning = false;

				break;

			default:
				break;
			}

			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				m_window.ProcessResize(glm::uvec2{ event.window.data1, event.window.data2 });

				[[fallthrough]];

			case SDL_WINDOWEVENT_MINIMIZED:
				m_renderer->ProcessWindowResize();
				m_world->ProcessWindowResize(m_window);

				break;

			case SDL_WINDOWEVENT_FOCUS_LOST:
				m_isPaused = true;

				break;

			case SDL_WINDOWEVENT_FOCUS_GAINED:
				m_isPaused = false;

				break;

			default:
				break;
			}

			break;

		case SDL_QUIT:
			m_isRunning = false;

			break;

		default:
			break;
		}
	}
}

void TerrainGenerator::ProcessInput()
{
	m_world->ProcessInput();
}

void TerrainGenerator::Update()
{
	const float deltaTime = CalculateDeltaTime();

	m_world->Update(deltaTime);
}

void TerrainGenerator::Render()
{
	if (m_renderer->PrepareRender())
	{
		constexpr glm::vec4 SkyClearColour{ 0.1f, 0.5f, 1.0f, 1.0f };

		m_renderer->BeginRender(SkyClearColour);
		{
			m_world->Render();
		}
		m_renderer->EndRender();

		m_renderer->Present();
	}
}

float TerrainGenerator::CalculateDeltaTime()
{
	constexpr float MillisecondsPerSecond = 1000.0f;

	const float deltaTime = static_cast<float>(SDL_GetTicks() - m_ticksCount) / MillisecondsPerSecond;
	m_ticksCount = SDL_GetTicks();

	return deltaTime;
}