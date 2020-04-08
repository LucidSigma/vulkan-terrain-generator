#pragma once

#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "../engine/window/Window.h"
#include "../engine/graphics/renderer/Renderer.h"

class TerrainGenerator
{
private:
	static constexpr float s_NearPlane = 0.1f;
	static constexpr float s_FarPlane = 1000.0f;

	static constexpr glm::uvec2 s_InitialWindowSize{ 1280u, 720u };

	Window m_window;
	std::unique_ptr<Renderer> m_renderer = nullptr;

	bool m_isRunning = true;
	bool m_isPaused = false;

	// Terrain graphics pipeline.
	// World and chunks.
	// Perspective matrix.

public:
	TerrainGenerator();
	~TerrainGenerator() noexcept;

	void Run();

private:
	void Initialise();
	void Destroy() noexcept;
};