#pragma once

#include "Buffer.h"

#include <cstddef>
#include <vector>

class VertexBuffer
	: public Buffer	
{
private:
	std::uint32_t m_vertexCount = 0;

public:
	VertexBuffer(const class Renderer& renderer);
	~VertexBuffer() noexcept = default;

	template <typename T>
	void Initialise(const std::vector<T>& bufferData)
	{
		m_vertexCount = static_cast<std::uint32_t>(bufferData.size());
		Create(bufferData.data(), sizeof(T) * bufferData.size(), Usage::Vertex);
	}

	inline std::uint32_t GetVertexCount() const noexcept { return m_vertexCount; }
};