#pragma once

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "VulkanContext.h"

namespace vulkan_util
{
	extern void CreateBuffer(const VulkanContext& vulkanContext, const VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& bufferAllocation);
	extern void CopyBuffer(const VulkanContext& vulkanContext, const VkBuffer& sourceBuffer, const VkBuffer& destinationBuffer, const VkDeviceSize size);

	extern void CreateImage(const VulkanContext& vulkanContext, const std::uint32_t width, const std::uint32_t height, const VkFormat format, const VkImageTiling imageTiling, const VkImageUsageFlags imageUsage, const VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& imageAlloaction);
	[[nodiscard]] extern VkImageView CreateImageView(const VulkanContext& vulkanContext, const VkImage image, const VkFormat format, const VkImageAspectFlags imageAspect);
	[[nodiscard]] extern VkFormat FindSupportedFormat(const VulkanContext& vulkanContext, const std::vector<VkFormat>& candidateFormats, const VkImageTiling imageTiling, const VkFormatFeatureFlags features);

	[[nodiscard]] extern VkCommandBuffer BeginSingleTimeCommands(const VulkanContext& vulkanContext);
	extern void EndSingleTimeCommands(const VulkanContext& vulkanContext, VkCommandBuffer commandBuffer);

	extern void TransitionImageLayout(const VulkanContext& vulkanContext, const VkImage image, const VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout, const bool supportStencil);
}