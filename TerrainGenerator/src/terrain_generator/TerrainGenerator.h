#pragma once

#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "../engine/window/Window.h"
#include "../engine/graphics/renderer/Renderer.h"
#include "World.h"

class TerrainGenerator
{
private:
	static constexpr glm::uvec2 s_InitialWindowSize{ 1280u, 720u };

	Window m_window;
	std::unique_ptr<Renderer> m_renderer = nullptr;

	bool m_isRunning = true;
	bool m_isPaused = false;

	std::unique_ptr<World> m_world = nullptr;

public:
	TerrainGenerator();
	~TerrainGenerator() noexcept;

	void Run();

private:
	void Initialise();
	void Destroy() noexcept;
};