#pragma once

#include "../../utility/interfaces/INoncopyable.h"
#include "../../utility/interfaces/INonmovable.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "../../window/Window.h"
#include "../buffers/IndexBuffer.h"
#include "../buffers/VertexBuffer.h"
#include "../pipeline/GraphicsPipeline.h"
#include "VulkanContext.h"

class Renderer
	: private INoncopyable, private INonmovable
{
private:
	struct FrameSynchronisationPrimitives
	{
		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

		VkFence inFlightFence = VK_NULL_HANDLE;
	};

	struct DepthStencilBuffer
	{
		VkImage image = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;

		VmaAllocation allocation = VK_NULL_HANDLE;
	};

	static constexpr std::size_t s_MaxFramesInFlight = 2u;
	
	const Window& m_window;
	VulkanContext m_vulkanContext;

	std::vector<VkCommandBuffer> m_commandBuffers{ };
	std::uint32_t m_nextAcquiredImageIndex = 0;

	std::vector<FrameSynchronisationPrimitives> m_perFrameSynchronisation;
	std::size_t m_currentFrameInFlight = 0;

	VkSurfaceFormatKHR m_surfaceFormat{ };
	VkPresentModeKHR m_presentationMode = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D m_swapchainExtent{ };

	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;

	VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_framebuffers;

	DepthStencilBuffer m_depthStencilBuffer{ };
	bool m_supportsStencil = false;

	bool m_hasFramebufferResized = false;

public:
	Renderer(const Window& window);
	~Renderer() noexcept;

	bool PrepareRender();
	void BeginRender(const glm::vec4& clearColour = glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f });
	void EndRender();
	void Present();
	void FinaliseRenderOperations() const noexcept;

	void BindPipeline(const GraphicsPipeline& pipeline);

	void BindVertexBuffer(const VertexBuffer& vertexBuffer);
	void BindIndexBuffer(const IndexBuffer& indexBuffer);

	template <typename T>
	void PushConstants(const GraphicsPipeline& pipeline, const T& data)
	{
		vkCmdPushConstants(m_commandBuffers[m_nextAcquiredImageIndex], pipeline.GetLayout(), pipeline.GetPushConstantStageFlags(), 0, sizeof(T), &data);
	}

	void BindDescriptorSet(const GraphicsPipeline& pipeline);

	void Draw(const std::uint32_t vertexCount);
	void DrawIndexed(const std::uint32_t indexCount);

	void ProcessWindowResize();

	inline const VulkanContext& GetVulkanContext() const noexcept { return m_vulkanContext; }

	inline std::uint32_t GetNextAcquiredImageIndex() const noexcept { return m_nextAcquiredImageIndex; }

	inline const VkExtent2D& GetSwapchainExtent() const noexcept { return m_swapchainExtent; }
	inline std::uint32_t GetSwapchainImageCount() const noexcept { return static_cast<std::uint32_t>(m_swapchainImages.size()); }
	inline VkRenderPass GetRenderPass() const noexcept { return m_renderPass; }

	inline bool SupportsStencilOperations() const noexcept { return m_supportsStencil; }

private:
	void InitialisePipelineCache();
	void DestroyPipelineCache() noexcept;

	void InitialiseSynchronisationPrimitives();
	void DestroySynchronisationPrimitives() noexcept;

	void InitialiseSwapchain();
	void InitialiseSwapchainImages();
	[[nodiscard]] VkSurfaceFormatKHR GetBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats) const;
	[[nodiscard]] VkPresentModeKHR GetBestPresentationMode(const std::vector<VkPresentModeKHR>& availablePresentationModes) const;
	[[nodiscard]] VkExtent2D GetDrawableSurfaceExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const;
	void DestroySwapchainImages() noexcept;
	void DestroySwapchain() noexcept;

	void AllocateCommandBuffers();
	void DeallocateCommandBuffers() noexcept;

	void InitialiseRenderPass();
	void DestroyRenderPass() noexcept;
	void InitialiseFramebuffers();
	void DestroyFramebuffers() noexcept;

	void InitialiseDepthStencilBuffer();
	[[nodiscard]] VkFormat FindDepthStencilFormat();
	void DestroyDepthStencilBuffer() noexcept;

	void CleanupPresentationObjects() noexcept;
	void RecreatePresentationObjects();
};