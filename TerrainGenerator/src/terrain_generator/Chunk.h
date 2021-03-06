#pragma once

#include <cstddef>

#include <glm/glm.hpp>

#include "../engine/graphics/buffers/IndexBuffer.h"
#include "../engine/graphics/buffers/VertexBuffer.h"
#include "../engine/graphics/pipeline/GraphicsPipeline.h"

class Chunk
{
private:
	static constexpr std::size_t s_ChunkLength = 32u;
	static constexpr std::size_t s_ChunkWidth = 32u;

	VertexBuffer m_vertexBuffer;
	IndexBuffer m_indexBuffer;

	glm::ivec2 m_position;
	glm::mat4 m_model{ 1.0f };

public:
	static constexpr std::size_t GetChunkLength() noexcept { return s_ChunkLength; }
	static constexpr std::size_t GetChunkWidth() noexcept { return s_ChunkWidth; }
	
	Chunk(const class Renderer& renderer, const glm::ivec2& position);
	~Chunk() noexcept;

	void Render(class Renderer& renderer, const GraphicsPipeline& pipeline);

	inline const glm::ivec2& GetPosition() const noexcept { return m_position; }

private:
	static std::vector<std::vector<float>> CreateNoiseMap(const glm::ivec2& position);

	void InitialiseVertices();
	glm::vec3 GetBiomeColour(const float height) const;
};