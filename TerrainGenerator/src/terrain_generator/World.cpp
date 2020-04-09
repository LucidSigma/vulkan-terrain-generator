#include "World.h"

#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

World::World(Renderer& renderer, const Window& window)
	: m_renderer(renderer)
{
	Initialise(window);

	for (int i = -s_RenderDistance; i <= s_RenderDistance; ++i)
	{
		for (int j = -s_RenderDistance; j <= s_RenderDistance; ++j)
		{
			m_chunks.emplace_back(std::make_unique<Chunk>(m_renderer, glm::ivec2{ i, j }));
		}
	}
}

World::~World() noexcept
{
	m_chunks.clear();
	m_terrainPipeline->Destroy();
}

void World::ProcessInput()
{
	m_camera.ProcessInput();
}

void World::Update(const float deltaTime)
{
	m_camera.Update(deltaTime);

	static glm::ivec2 previousChunk{ 0, 0 };
	const glm::ivec2 currentChunk = glm::ivec2{ glm::round(m_camera.GetPosition().x / Chunk::GetChunkLength()), glm::round(m_camera.GetPosition().z / Chunk::GetChunkWidth()) };

	if (currentChunk != previousChunk)
	{
		for (auto& chunk : m_chunks)
		{
			const float xDistanceFromCamera = chunk->GetPosition().x - (m_camera.GetPosition().x / Chunk::GetChunkLength());
			const float zDistanceFromCamera = chunk->GetPosition().y - (m_camera.GetPosition().z / Chunk::GetChunkWidth());

			if (xDistanceFromCamera > s_RenderDistance)
			{
				chunk = std::make_unique<Chunk>(m_renderer, glm::ivec2{ chunk->GetPosition().x - (s_RenderDistance * 2.0f), chunk->GetPosition().y });
			}
			else if (xDistanceFromCamera < -s_RenderDistance)
			{
				chunk = std::make_unique<Chunk>(m_renderer, glm::ivec2{ chunk->GetPosition().x + (s_RenderDistance * 2.0f), chunk->GetPosition().y });
			}

			else if (zDistanceFromCamera > s_RenderDistance)
			{
				chunk = std::make_unique<Chunk>(m_renderer, glm::ivec2{ chunk->GetPosition().x, chunk->GetPosition().y - (s_RenderDistance * 2.0f) });
			}
			else if (zDistanceFromCamera < -s_RenderDistance)
			{
				chunk = std::make_unique<Chunk>(m_renderer, glm::ivec2{ chunk->GetPosition().x, chunk->GetPosition().y + (s_RenderDistance * 2.0f) });
			}
		}
	}

	previousChunk = currentChunk;
}

void World::Render()
{
	m_renderer.BindPipeline(*m_terrainPipeline);

	const std::array<glm::mat4, 2> viewProjection{ m_camera.GetViewMatrix(), m_projection };
	m_terrainPipeline->SetUniform(0, viewProjection);
	m_renderer.BindDescriptorSet(*m_terrainPipeline);

	for (const auto& chunk : m_chunks)
	{
		chunk->Render(m_renderer, *m_terrainPipeline);
	}
}

void World::ProcessWindowResize(const Window& window)
{
	m_terrainPipeline->RefreshUniformBuffers();

	m_projection = glm::perspectiveLH(glm::radians(60.0f), static_cast<float>(window.GetDrawableSize().x) / static_cast<float>(window.GetDrawableSize().y), 0.1f, 2500.0f);
	m_projection[1][1] *= -1.0f;
}

void World::Initialise(const Window& window)
{
	const GraphicsPipeline::Config terrainPipelineConfig{
		.shaderInfo{
			{ "assets/shaders/terrain.vert.spv", ShaderModule::Stage::Vertex },
			{ "assets/shaders/terrain.frag.spv", ShaderModule::Stage::Fragment }
		},

		.enableDepthTest = true,
		.drawWireframe = false,
		.enableCullFace = true,
		.enableBlending = true
	};

	m_terrainPipeline = std::make_unique<GraphicsPipeline>(m_renderer, terrainPipelineConfig);

	m_projection = glm::perspectiveLH(glm::radians(60.0f), static_cast<float>(window.GetDrawableSize().x) / static_cast<float>(window.GetDrawableSize().y), 0.1f, 2500.0f);
	m_projection[1][1] *= -1.0f;
}