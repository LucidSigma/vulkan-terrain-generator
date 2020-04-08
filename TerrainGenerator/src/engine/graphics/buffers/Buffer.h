#pragma once

#include "../../utility/interfaces/INoncopyable.h"
#include "../../utility/interfaces/INonmovable.h"

#include <type_traits>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

class Buffer
	: private INoncopyable, private INonmovable
{
protected:
	enum class Usage
		: std::underlying_type_t<VkBufferUsageFlagBits>
	{
		Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	};

	const class Renderer& m_renderer;

	VkBuffer m_bufferHandle = VK_NULL_HANDLE;
	VmaAllocation m_allocation = VK_NULL_HANDLE;

public:
	Buffer(const class Renderer& renderer);
	~Buffer() noexcept;

	void Destroy() noexcept;

	inline const VkBuffer& GetHandle() const noexcept { return m_bufferHandle; }

protected:
	void Create(const void* bufferData, const VkDeviceSize bufferSize, const Usage usage);
};