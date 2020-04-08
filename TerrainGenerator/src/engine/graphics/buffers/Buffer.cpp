#include "Buffer.h"

#include <cstddef>
#include <cstring>

#include "../renderer/Renderer.h"
#include "../renderer/VulkanUtility.h"

Buffer::Buffer(const Renderer& renderer)
	: m_renderer(renderer)
{ }

Buffer::~Buffer() noexcept
{
	Destroy();
}

void Buffer::Destroy() noexcept
{
	if (m_bufferHandle != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(m_renderer.GetVulkanContext().GetAllocator(), m_bufferHandle, m_allocation);
		m_bufferHandle = VK_NULL_HANDLE;
		m_allocation = VK_NULL_HANDLE;
	}
}

void Buffer::Create(const void* bufferData, const VkDeviceSize bufferSize, const Usage usage)
{
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VmaAllocation stagingAllocation = VK_NULL_HANDLE;
	vulkan_util::CreateBuffer(m_renderer.GetVulkanContext(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAllocation);

	void* vertexData = nullptr;

	vmaMapMemory(m_renderer.GetVulkanContext().GetAllocator(), stagingAllocation, &vertexData);
	{
		std::memcpy(vertexData, bufferData, static_cast<std::size_t>(bufferSize));
	}
	vmaUnmapMemory(m_renderer.GetVulkanContext().GetAllocator(), stagingAllocation);

	vulkan_util::CreateBuffer(m_renderer.GetVulkanContext(), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | static_cast<VkBufferUsageFlagBits>(usage), VMA_MEMORY_USAGE_GPU_ONLY, m_bufferHandle, m_allocation);
	vulkan_util::CopyBuffer(m_renderer.GetVulkanContext(), stagingBuffer, m_bufferHandle, bufferSize);

	vmaDestroyBuffer(m_renderer.GetVulkanContext().GetAllocator(), stagingBuffer, stagingAllocation);
	stagingBuffer = VK_NULL_HANDLE;
	stagingAllocation = VK_NULL_HANDLE;
}