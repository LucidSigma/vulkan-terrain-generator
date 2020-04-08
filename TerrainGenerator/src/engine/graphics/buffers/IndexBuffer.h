#pragma once

#include "Buffer.h"

#include <cstdint>
#include <type_traits>
#include <vector>

class IndexBuffer
	: public Buffer
{
private:
	std::uint32_t m_indexCount = 0;
	VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;

public:
	IndexBuffer(const class Renderer& renderer);
	~IndexBuffer() noexcept = default;
	
	template <typename T>
	void Initialise(const std::vector<T>& bufferData)
	{
		m_indexCount = static_cast<std::uint32_t>(bufferData.size());

		if constexpr (std::is_same_v<T, std::uint16_t>)
		{
			m_indexType = VK_INDEX_TYPE_UINT16;
		}
		else if constexpr (std::is_same_v<T, std::uint32_t>)
		{
			m_indexType = VK_INDEX_TYPE_UINT32;
		}
		else
		{
			static_assert(false, "Invalid index buffer type.");
		}

		Create(bufferData.data(), sizeof(T) * bufferData.size(), Usage::Index);
	}

	inline std::uint32_t GetIndexCount() const noexcept { return m_indexCount; }
	inline VkIndexType GetIndexType() const noexcept { return m_indexType; }
};