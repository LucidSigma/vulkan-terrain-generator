#include "Chunk.h"

#include <cstdint>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>

#include "../engine/graphics/renderer/Renderer.h"
#include "../engine/graphics/Vertex.h"

Chunk::Chunk(const Renderer& renderer, const glm::ivec2& position)
	: m_vertexBuffer(renderer), m_indexBuffer(renderer), m_position(position)
{
	m_model = glm::translate(glm::mat4{ 1.0f }, glm::vec3{ m_position.x * static_cast<int>(s_ChunkLength), 0.0f, m_position.y * static_cast<int>(s_ChunkWidth) });
	InitialiseVertices();
}

Chunk::~Chunk() noexcept
{ }

void Chunk::Render(class Renderer& renderer, const GraphicsPipeline& pipeline)
{
	renderer.PushConstants(pipeline, m_model);

	renderer.BindVertexBuffer(m_vertexBuffer);
	renderer.BindIndexBuffer(m_indexBuffer);

	renderer.DrawIndexed(m_indexBuffer.GetIndexCount());
}

std::vector<std::vector<float>> Chunk::CreateNoiseMap(const glm::ivec2& position)
{
	std::vector<std::vector<float>> noiseMap(s_ChunkLength + 1, std::vector<float>(s_ChunkWidth + 1));

	for (int x = 0; x < noiseMap.size(); ++x)
	{
		for (int z = 0; z < noiseMap.front().size(); ++z)
		{
			constexpr auto NormalisedSimplex = [](const glm::vec2& values) -> float
			{
				return (glm::simplex(glm::vec2{ values.x, values.y }) + 1.0f) / 2.0f;
			};

			const float normalisedX = ((position.x * static_cast<int>(s_ChunkLength)) + x) / (16.0f * s_ChunkLength) - 0.5f;
			const float normalisedZ = ((position.y * static_cast<int>(s_ChunkWidth)) + z) / (16.0f * s_ChunkWidth) - 0.5f;

			noiseMap[x][z] = NormalisedSimplex(1.0f * glm::vec2{ normalisedX, normalisedZ });
			noiseMap[x][z] += 0.5f * NormalisedSimplex(2.0f * glm::vec2{ normalisedX, normalisedZ });
			noiseMap[x][z] += 0.25f * NormalisedSimplex(4.0f * glm::vec2{ normalisedX, normalisedZ });
			noiseMap[x][z] += 0.125f * NormalisedSimplex(8.0f * glm::vec2{ normalisedX, normalisedZ });
			noiseMap[x][z] += 0.0625f * NormalisedSimplex(16.0f * glm::vec2{ normalisedX, normalisedZ });
			noiseMap[x][z] = glm::pow(noiseMap[x][z], 2);

			noiseMap[x][z] *= 48.0f;
		}
	}

	return noiseMap;
}

void Chunk::InitialiseVertices()
{
	const auto noiseMap = CreateNoiseMap(m_position);

	std::vector<VertexP3C3> chunkVertices;
	chunkVertices.reserve(s_ChunkLength * s_ChunkWidth * 4);

	std::vector<std::uint16_t> chunkIndices;
	chunkIndices.reserve(s_ChunkLength * s_ChunkWidth * 6);

	unsigned int indexCount = 0;

	for (std::size_t x = 0; x < s_ChunkLength; ++x)
	{
		for (std::size_t z = 0; z < s_ChunkWidth; ++z)
		{
			chunkVertices.push_back({
				glm::vec3{ x, noiseMap[x][z], z },
				glm::vec3{ 0.0f, 1.0f, 0.0f }
			});

			chunkVertices.push_back({
				glm::vec3{ 1.0f + x, noiseMap[x + 1][z], z },
				glm::vec3{ 0.0f, 1.0f, 0.0f }
			});

			chunkVertices.push_back({
				glm::vec3{ x, noiseMap[x][z + 1], 1.0f + z },
				glm::vec3{ 0.0f, 1.0f, 0.0f }
			});

			chunkVertices.push_back({
				glm::vec3{ 1.0f + x, noiseMap[x + 1][z + 1], 1.0f + z },
				glm::vec3{ 0.0f, 1.0f, 0.0f }
			});

			chunkIndices.push_back(indexCount + 0);
			chunkIndices.push_back(indexCount + 1);
			chunkIndices.push_back(indexCount + 2);
			chunkIndices.push_back(indexCount + 2);
			chunkIndices.push_back(indexCount + 1);
			chunkIndices.push_back(indexCount + 3);

			indexCount += 4;
		}
	}

	m_vertexBuffer.Initialise(chunkVertices);
	m_indexBuffer.Initialise(chunkIndices);
}