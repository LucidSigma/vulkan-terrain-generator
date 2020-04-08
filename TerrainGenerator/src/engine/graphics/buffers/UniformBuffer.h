#pragma once

#include "../../utility/interfaces/INoncopyable.h"
#include "../../utility/interfaces/INonmovable.h"

#include <cstddef>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

class UniformBuffer
	: private INoncopyable, private INonmovable
{
private:
	const class Renderer& m_renderer;

	VkBuffer m_bufferHandle = VK_NULL_HANDLE;
	VmaAllocation m_allocation = VK_NULL_HANDLE;

	VkDeviceSize m_bufferSize = 0;

public:
	UniformBuffer(const class Renderer& renderer);
	~UniformBuffer() noexcept;

	void Initialise(const VkDeviceSize size);
	void Destroy() noexcept;

	void SetBufferData(const void* bufferData, const std::size_t size, const VkDeviceSize offset);

	inline VkBuffer GetHandle() const noexcept { return m_bufferHandle; }
	inline VkDeviceSize GetSize() const noexcept { return m_bufferSize; }

private:
	void Create();
};