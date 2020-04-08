#pragma once

#include <memory>
#include <vector>

#include "../engine/graphics/pipeline/GraphicsPipeline.h"
#include "../engine/graphics/renderer/Renderer.h"
#include "../engine/window/Window.h"
#include "Camera3D.h"
#include "Chunk.h"

class World
{
private:
	Renderer& m_renderer;
	std::unique_ptr<GraphicsPipeline> m_terrainPipeline = nullptr;

	Camera3D m_camera{ glm::vec3{ 0.0f, 64.0f, 0.0f } };
	std::vector<std::unique_ptr<Chunk>> m_chunks;

	glm::mat4 m_projection{ 1.0f };

public:
	World(class Renderer& renderer, const Window& window);
	~World() noexcept;

	void ProcessInput();
	void Update(const float deltaTime);
	void Render();

	void ProcessWindowResize(const Window& window);

private:
	void Initialise(const Window& window);
};