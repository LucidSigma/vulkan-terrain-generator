#include "VulkanUtility.h"

#include <stdexcept>

namespace vulkan_util
{
	void CreateBuffer(const VulkanContext& vulkanContext, const VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& bufferAllocation)
	{
		VkBufferCreateInfo bufferCreateInfo{ };
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{ };
		allocationCreateInfo.usage = memoryUsage;

		if (vmaCreateBuffer(vulkanContext.GetAllocator(), &bufferCreateInfo, &allocationCreateInfo, &buffer, &bufferAllocation, nullptr) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan buffer.");
		}
	}

	void CopyBuffer(const VulkanContext& vulkanContext, const VkBuffer& sourceBuffer, const VkBuffer& destinationBuffer, const VkDeviceSize size)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{ };
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandPool = vulkanContext.GetCommandPool();
		commandBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		vkAllocateCommandBuffers(vulkanContext.GetLogicalDevice(), &commandBufferAllocateInfo, &commandBuffer);

		VkCommandBufferBeginInfo commandBufferBeginInfo{ };
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
		{
			VkBufferCopy copyRegion{ };
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;

			vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);
		}
		vkEndCommandBuffer(commandBuffer);

		VkFence transferCompleteFence = VK_NULL_HANDLE;
		{
			VkFenceCreateInfo fenceCreateInfo{ };
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

			if (vkCreateFence(vulkanContext.GetLogicalDevice(), &fenceCreateInfo, nullptr, &transferCompleteFence) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create Vulkan fence.");
			}
		}

		VkSubmitInfo submitInfo{ };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(vulkanContext.GetGraphicsQueue(), 1, &submitInfo, transferCompleteFence);
		vkWaitForFences(vulkanContext.GetLogicalDevice(), 1, &transferCompleteFence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());

		vkDestroyFence(vulkanContext.GetLogicalDevice(), transferCompleteFence, nullptr);
		transferCompleteFence = VK_NULL_HANDLE;

		vkFreeCommandBuffers(vulkanContext.GetLogicalDevice(), vulkanContext.GetCommandPool(), 1, &commandBuffer);
		commandBuffer = VK_NULL_HANDLE;
	}

	void CreateImage(const VulkanContext& vulkanContext, const std::uint32_t width, const std::uint32_t height, const VkFormat format, const VkImageTiling imageTiling, const VkImageUsageFlags imageUsage, const VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& imageAlloaction)
	{
		VkImageCreateInfo imageCreateInfo{ };
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = imageTiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = imageUsage;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{ };
		allocationCreateInfo.usage = memoryUsage;

		if (vmaCreateImage(vulkanContext.GetAllocator(), &imageCreateInfo, &allocationCreateInfo, &image, &imageAlloaction, nullptr) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan image.");
		}
	}

	[[nodiscard]] VkImageView CreateImageView(const VulkanContext& vulkanContext, const VkImage image, const VkFormat format, const VkImageAspectFlags imageAspect)
	{
		VkImageViewCreateInfo imageViewCreateInfo{ };
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = imageAspect;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView imageView{ };

		if (vkCreateImageView(vulkanContext.GetLogicalDevice(), &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan image view.");
		}

		return imageView;
	}

	[[nodiscard]] VkFormat FindSupportedFormat(const VulkanContext& vulkanContext, const std::vector<VkFormat>& candidateFormats, const VkImageTiling imageTiling, const VkFormatFeatureFlags features)
	{
		for (const VkFormat format : candidateFormats)
		{
			VkFormatProperties formatProperties{ };
			vkGetPhysicalDeviceFormatProperties(vulkanContext.GetPhysicalDevice(), format, &formatProperties);

			if ((imageTiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features) ||
				(imageTiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features))
			{
				return format;
			}
		}

		throw std::runtime_error("Failed to find a supported Vulkan depth/stencil format.");
	}

	[[nodiscard]] VkCommandBuffer BeginSingleTimeCommands(const VulkanContext& vulkanContext)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{ };
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandPool = vulkanContext.GetCommandPool();
		commandBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		if (vkAllocateCommandBuffers(vulkanContext.GetLogicalDevice(), &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Vulkan single time command buffer.");
		}

		VkCommandBufferBeginInfo commandBufferBeginInfo{ };
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
		{
			vkFreeCommandBuffers(vulkanContext.GetLogicalDevice(), vulkanContext.GetCommandPool(), 1, &commandBuffer);

			throw std::runtime_error("Failed to begin recording Vulkan single time command buffer.");
		}

		return commandBuffer;
	}

	void EndSingleTimeCommands(const VulkanContext& vulkanContext, VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{ };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		if (vkQueueSubmit(vulkanContext.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			vkFreeCommandBuffers(vulkanContext.GetLogicalDevice(), vulkanContext.GetCommandPool(), 1, &commandBuffer);

			throw std::runtime_error("Failed to submit Vulkan single time command buffer to graphics queue.");
		}

		vkQueueWaitIdle(vulkanContext.GetGraphicsQueue());

		vkFreeCommandBuffers(vulkanContext.GetLogicalDevice(), vulkanContext.GetCommandPool(), 1, &commandBuffer);
	}

	void TransitionImageLayout(const VulkanContext& vulkanContext, const VkImage image, const VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout, const bool supportStencil)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(vulkanContext);

		VkImageMemoryBarrier imageMemoryBarrier{ };
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = oldLayout;
		imageMemoryBarrier.newLayout = newLayout;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = image;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (supportStencil)
			{
				imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage{ };
		VkPipelineStageFlags destinationStage{ };

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			throw std::invalid_argument("Unsupported Vulkan image layout transition.");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

		EndSingleTimeCommands(vulkanContext, commandBuffer);
		commandBuffer = VK_NULL_HANDLE;
	}
}