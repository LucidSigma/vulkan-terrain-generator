#include "Renderer.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>

#include <SDL2/SDL.h>

#include "VulkanUtility.h"

Renderer::Renderer(const Window& window)
	: m_window(window), m_vulkanContext(m_window)
{
	InitialisePipelineCache();
	InitialiseSynchronisationPrimitives();

	InitialiseSwapchain();
	InitialiseRenderPass();
	InitialiseDepthStencilBuffer();
	InitialiseFramebuffers();
	AllocateCommandBuffers();
}

Renderer::~Renderer() noexcept
{
	CleanupPresentationObjects();

	DestroySynchronisationPrimitives();
	DestroyPipelineCache();
}

bool Renderer::PrepareRender()
{
	vkWaitForFences(m_vulkanContext.GetLogicalDevice(), 1, &m_perFrameSynchronisation[m_currentFrameInFlight].inFlightFence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
	vkResetFences(m_vulkanContext.GetLogicalDevice(), 1, &m_perFrameSynchronisation[m_currentFrameInFlight].inFlightFence);

	if (const VkResult imageAcquisitionResult = vkAcquireNextImageKHR(m_vulkanContext.GetLogicalDevice(), m_swapchain, std::numeric_limits<std::uint64_t>::max(), m_perFrameSynchronisation[m_currentFrameInFlight].imageAvailableSemaphore, VK_NULL_HANDLE, &m_nextAcquiredImageIndex);
		imageAcquisitionResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreatePresentationObjects();

		return false;
	}
	else if (imageAcquisitionResult != VK_SUCCESS && imageAcquisitionResult != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire next Vulkan swapchain image.");
	}

	return true;
}

void Renderer::BeginRender(const glm::vec4& clearColour)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{ };
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(m_commandBuffers[m_nextAcquiredImageIndex], &commandBufferBeginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording Vulkan command buffer.");
	}

	VkRenderPassBeginInfo renderPassBeginInfo{ };
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.framebuffer = m_framebuffers[m_nextAcquiredImageIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = m_swapchainExtent;

	std::array<VkClearValue, 2u> clearValues{ };
	{
		clearValues[0].color = VkClearColorValue{ clearColour.r, clearColour.g, clearColour.b, clearColour.a };
		clearValues[1].depthStencil = VkClearDepthStencilValue{ 1.0f, 0 };
	}

	renderPassBeginInfo.clearValueCount = static_cast<std::uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_commandBuffers[m_nextAcquiredImageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::EndRender()
{
	vkCmdEndRenderPass(m_commandBuffers[m_nextAcquiredImageIndex]);

	if (vkEndCommandBuffer(m_commandBuffers[m_nextAcquiredImageIndex]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record Vulkan command buffer.");
	}

	VkSubmitInfo submitInfo{ };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_perFrameSynchronisation[m_currentFrameInFlight].imageAvailableSemaphore;

	const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_nextAcquiredImageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_perFrameSynchronisation[m_currentFrameInFlight].renderFinishedSemaphore;

	if (vkQueueSubmit(m_vulkanContext.GetGraphicsQueue(), 1, &submitInfo, m_perFrameSynchronisation[m_currentFrameInFlight].inFlightFence) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit Vulkan draw command buffer.");
	}
}

void Renderer::Present()
{
	VkPresentInfoKHR presentationInfo{ };
	presentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentationInfo.waitSemaphoreCount = 1;
	presentationInfo.pWaitSemaphores = &m_perFrameSynchronisation[m_currentFrameInFlight].renderFinishedSemaphore;
	presentationInfo.swapchainCount = 1;
	presentationInfo.pSwapchains = &m_swapchain;
	presentationInfo.pImageIndices = &m_nextAcquiredImageIndex;
	presentationInfo.pResults = nullptr;

	if (const VkResult queuePresentResult = vkQueuePresentKHR(m_vulkanContext.GetPresentationQueue(), &presentationInfo);
		queuePresentResult == VK_ERROR_OUT_OF_DATE_KHR || queuePresentResult == VK_SUBOPTIMAL_KHR || m_hasFramebufferResized)
	{
		m_hasFramebufferResized = false;
		RecreatePresentationObjects();
	}
	else if (queuePresentResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present next Vulkan swapchain image.");
	}

	m_currentFrameInFlight = (m_currentFrameInFlight + 1) % s_MaxFramesInFlight;
}

void Renderer::FinaliseRenderOperations() const noexcept
{
	m_vulkanContext.WaitOnLogicalDevice();
}

void Renderer::BindPipeline(const GraphicsPipeline& pipeline)
{
	vkCmdBindPipeline(m_commandBuffers[m_nextAcquiredImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetHandle());

	VkViewport viewport{ };
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchainExtent.width);
	viewport.height = static_cast<float>(m_swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(m_commandBuffers[m_nextAcquiredImageIndex], 0, 1, &viewport);

	VkRect2D scissor{ };
	scissor.offset = VkOffset2D{ 0, 0 };
	scissor.extent = m_swapchainExtent;

	vkCmdSetScissor(m_commandBuffers[m_nextAcquiredImageIndex], 0, 1, &scissor);
}

void Renderer::BindVertexBuffer(const VertexBuffer& vertexBuffer)
{
	const VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(m_commandBuffers[m_nextAcquiredImageIndex], 0, 1, &vertexBuffer.GetHandle(), &offset);
}

void Renderer::BindIndexBuffer(const IndexBuffer& indexBuffer)
{
	vkCmdBindIndexBuffer(m_commandBuffers[m_nextAcquiredImageIndex], indexBuffer.GetHandle(), 0, indexBuffer.GetIndexType());
}

void Renderer::BindDescriptorSet(const GraphicsPipeline& pipeline)
{
	vkCmdBindDescriptorSets(m_commandBuffers[m_nextAcquiredImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(), 0, 1, &pipeline.GetDescriptorSets()[m_nextAcquiredImageIndex], 0, nullptr);
}

void Renderer::Draw(const std::uint32_t vertexCount)
{
	vkCmdDraw(m_commandBuffers[m_nextAcquiredImageIndex], vertexCount, 1, 0, 0);
}

void Renderer::DrawIndexed(const std::uint32_t indexCount)
{
	vkCmdDrawIndexed(m_commandBuffers[m_nextAcquiredImageIndex], indexCount, 1, 0, 0, 0);
}

void Renderer::ProcessWindowResize()
{
	m_hasFramebufferResized = true;
}

void Renderer::InitialisePipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ };
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = nullptr;

	if (vkCreatePipelineCache(m_vulkanContext.GetLogicalDevice(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan pipeline cache.");
	}
}

void Renderer::DestroyPipelineCache() noexcept
{
	if (m_pipelineCache != VK_NULL_HANDLE)
	{
		vkDestroyPipelineCache(m_vulkanContext.GetLogicalDevice(), m_pipelineCache, nullptr);
		m_pipelineCache = VK_NULL_HANDLE;
	}
}

void Renderer::InitialiseSynchronisationPrimitives()
{
	m_perFrameSynchronisation.resize(s_MaxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreCreateInfo{ };
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{ };
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (auto& [imageAvailableSemaphore, renderFinishedSemaphore, inFlightFence] : m_perFrameSynchronisation)
	{
		const std::array<std::reference_wrapper<VkSemaphore>, 2u> semaphores{ std::ref(imageAvailableSemaphore), std::ref(renderFinishedSemaphore) };

		for (auto& semaphore : semaphores)
		{
			if (vkCreateSemaphore(m_vulkanContext.GetLogicalDevice(), &semaphoreCreateInfo, nullptr, &semaphore.get()) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create Vulkan semaphore.");
			}
		}

		if (vkCreateFence(m_vulkanContext.GetLogicalDevice(), &fenceCreateInfo, nullptr, &inFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan fence.");
		}
	}
}

void Renderer::DestroySynchronisationPrimitives() noexcept
{
	if (!m_perFrameSynchronisation.empty())
	{
		for (auto& [imageAvailableSemaphore, renderFinishedSemaphore, inFlightFence] : m_perFrameSynchronisation)
		{
			const std::array<std::reference_wrapper<VkSemaphore>, 2u> semaphores{ std::ref(imageAvailableSemaphore), std::ref(renderFinishedSemaphore) };

			for (auto& semaphore : semaphores)
			{
				vkDestroySemaphore(m_vulkanContext.GetLogicalDevice(), semaphore, nullptr);
				semaphore.get() = VK_NULL_HANDLE;
			}

			vkDestroyFence(m_vulkanContext.GetLogicalDevice(), inFlightFence, nullptr);
			inFlightFence = VK_NULL_HANDLE;
		}

		m_perFrameSynchronisation.clear();
	}
}


void Renderer::InitialiseSwapchain()
{
	const VulkanContext::SurfaceProperties surfaceProperties = m_vulkanContext.GetSurfaceProperties(m_vulkanContext.GetPhysicalDevice());

	m_surfaceFormat = GetBestSurfaceFormat(surfaceProperties.surfaceFormats);
	m_presentationMode = GetBestPresentationMode(surfaceProperties.presentationModes);
	m_swapchainExtent = GetDrawableSurfaceExtent(surfaceProperties.surfaceCapabilities);

	std::uint32_t minSwapchainImageCount = surfaceProperties.surfaceCapabilities.minImageCount + 1;

	if (surfaceProperties.surfaceCapabilities.maxImageCount > 0 && minSwapchainImageCount > surfaceProperties.surfaceCapabilities.maxImageCount)
	{
		minSwapchainImageCount = surfaceProperties.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{ };
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = m_vulkanContext.GetSurface();
	swapchainCreateInfo.minImageCount = minSwapchainImageCount;
	swapchainCreateInfo.imageFormat = m_surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = m_swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const std::array<std::uint32_t, 2u> queueFamilyIndices{ m_vulkanContext.GetQueueFamilyIndices().graphicsFamilyIndex.value(), m_vulkanContext.GetQueueFamilyIndices().presentationFamilyIndex.value() };

	if (m_vulkanContext.GetQueueFamilyIndices().graphicsFamilyIndex == m_vulkanContext.GetQueueFamilyIndices().presentationFamilyIndex)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(queueFamilyIndices.size());
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}

	swapchainCreateInfo.preTransform = surfaceProperties.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = m_presentationMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_vulkanContext.GetLogicalDevice(), &swapchainCreateInfo, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan swapchain.");
	}

	InitialiseSwapchainImages();
}

void Renderer::InitialiseSwapchainImages()
{
	std::uint32_t swapchainImageCount = 0;

	if (vkGetSwapchainImagesKHR(m_vulkanContext.GetLogicalDevice(), m_swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to get Vulkan swapchain image count.");
	}

	m_swapchainImages.resize(swapchainImageCount);

	if (vkGetSwapchainImagesKHR(m_vulkanContext.GetLogicalDevice(), m_swapchain, &swapchainImageCount, m_swapchainImages.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to get Vulkan swapchain images.");
	}

	m_swapchainImageViews.resize(swapchainImageCount);

	for (std::size_t i = 0; i < m_swapchainImages.size(); ++i)
	{
		m_swapchainImageViews[i] = vulkan_util::CreateImageView(m_vulkanContext, m_swapchainImages[i], m_surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

[[nodiscard]] VkSurfaceFormatKHR Renderer::GetBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats) const
{
	for (const auto& availableSurfaceFormat : availableSurfaceFormats)
	{
		if (availableSurfaceFormat.format == VK_FORMAT_UNDEFINED)
		{
			return VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
		else if (availableSurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableSurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableSurfaceFormat;
		}
	}

	return availableSurfaceFormats.front();
}

[[nodiscard]] VkPresentModeKHR Renderer::GetBestPresentationMode(const std::vector<VkPresentModeKHR>& availablePresentationModes) const
{
	bool supportsImmediateMode = false;

	for (const auto& availablePresentationMode : availablePresentationModes)
	{
		if (availablePresentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentationMode;
		}
		else if (availablePresentationMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			supportsImmediateMode = true;
		}
	}

	return supportsImmediateMode ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard]] VkExtent2D Renderer::GetDrawableSurfaceExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const
{
	if (surfaceCapabilities.currentExtent.width < std::numeric_limits<std::uint32_t>::max() || surfaceCapabilities.currentExtent.height < std::numeric_limits<std::uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		const glm::uvec2 drawableWindowSize = m_window.GetDrawableSize();

		VkExtent2D actualSurfaceExtent{ drawableWindowSize.x, drawableWindowSize.y };
		actualSurfaceExtent.width = std::clamp(actualSurfaceExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		actualSurfaceExtent.height = std::clamp(actualSurfaceExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

		return actualSurfaceExtent;
	}
}

void Renderer::DestroySwapchainImages() noexcept
{
	if (!m_swapchainImages.empty())
	{
		for (auto& swapchainImageView : m_swapchainImageViews)
		{
			vkDestroyImageView(m_vulkanContext.GetLogicalDevice(), swapchainImageView, nullptr);
			swapchainImageView = VK_NULL_HANDLE;
		}

		m_swapchainImageViews.clear();
	}
}

void Renderer::DestroySwapchain() noexcept
{
	DestroySwapchainImages();
	
	if (m_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_vulkanContext.GetLogicalDevice(), m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;

		m_swapchainImages.clear();
	}
}

void Renderer::AllocateCommandBuffers()
{
	m_commandBuffers.resize(m_swapchainImages.size());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{ };
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_vulkanContext.GetCommandPool();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = static_cast<std::uint32_t>(m_commandBuffers.size());

	if (vkAllocateCommandBuffers(m_vulkanContext.GetLogicalDevice(), &commandBufferAllocateInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vulkan command buffers.");
	}
}

void Renderer::DeallocateCommandBuffers() noexcept
{
	if (!m_commandBuffers.empty())
	{
		vkFreeCommandBuffers(m_vulkanContext.GetLogicalDevice(), m_vulkanContext.GetCommandPool(), static_cast<std::uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		m_commandBuffers.clear();
	}
}

void Renderer::InitialiseRenderPass()
{
	enum AttachmentType
		: std::size_t
	{
		Colour = 0,
		DepthStencil = 1
	};

	std::array<VkAttachmentDescription, 2u> attachmentDescriptions{ };
	{
		attachmentDescriptions[AttachmentType::Colour].format = m_surfaceFormat.format;
		attachmentDescriptions[AttachmentType::Colour].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[AttachmentType::Colour].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[AttachmentType::Colour].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[AttachmentType::Colour].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[AttachmentType::Colour].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[AttachmentType::Colour].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[AttachmentType::Colour].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		attachmentDescriptions[AttachmentType::DepthStencil].format = FindDepthStencilFormat();
		attachmentDescriptions[AttachmentType::DepthStencil].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[AttachmentType::DepthStencil].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[AttachmentType::DepthStencil].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[AttachmentType::DepthStencil].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[AttachmentType::DepthStencil].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[AttachmentType::DepthStencil].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[AttachmentType::DepthStencil].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	std::array<VkAttachmentReference, 1u> subpassColourAttachmentReferences{ };
	{
		subpassColourAttachmentReferences[0].attachment = AttachmentType::Colour;
		subpassColourAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference depthStencilAttachmentReference{ };
	{
		depthStencilAttachmentReference.attachment = AttachmentType::DepthStencil;
		depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	std::array<VkSubpassDescription, 1u> subpassDescriptions{ };
	{
		subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[0].colorAttachmentCount = static_cast<std::uint32_t>(subpassColourAttachmentReferences.size());
		subpassDescriptions[0].pColorAttachments = subpassColourAttachmentReferences.data();
		subpassDescriptions[0].pDepthStencilAttachment = &depthStencilAttachmentReference;
	}

	std::array<VkSubpassDependency, 2u> subpassDependencies{ };
	{
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}
	
	VkRenderPassCreateInfo renderPassCreateInfo{ };
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<std::uint32_t>(attachmentDescriptions.size());
	renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
	renderPassCreateInfo.subpassCount = static_cast<std::uint32_t>(subpassDescriptions.size());
	renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
	renderPassCreateInfo.dependencyCount = static_cast<std::uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	if (vkCreateRenderPass(m_vulkanContext.GetLogicalDevice(), &renderPassCreateInfo, nullptr, &m_renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan render pass.");
	}
}

void Renderer::DestroyRenderPass() noexcept
{
	if (m_renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(m_vulkanContext.GetLogicalDevice(), m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	}
}

void Renderer::InitialiseFramebuffers()
{
	m_framebuffers.resize(m_swapchainImageViews.size());

	VkFramebufferCreateInfo framebufferCreateInfo{ };
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = m_renderPass;
	framebufferCreateInfo.width = m_swapchainExtent.width;
	framebufferCreateInfo.height = m_swapchainExtent.height;
	framebufferCreateInfo.layers = 1;

	for (std::size_t i = 0; i < m_swapchainImageViews.size(); ++i)
	{
		const std::array<VkImageView, 2u> framebufferAttachments{
			m_swapchainImageViews[i],
			m_depthStencilBuffer.imageView
		};

		framebufferCreateInfo.attachmentCount = static_cast<std::uint32_t>(framebufferAttachments.size());
		framebufferCreateInfo.pAttachments = framebufferAttachments.data();

		if (vkCreateFramebuffer(m_vulkanContext.GetLogicalDevice(), &framebufferCreateInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan framebuffer.");
		}
	}
}

void Renderer::DestroyFramebuffers() noexcept
{
	if (!m_framebuffers.empty())
	{
		for (auto& framebuffer : m_framebuffers)
		{
			vkDestroyFramebuffer(m_vulkanContext.GetLogicalDevice(), framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}

		m_framebuffers.clear();
	}
}

void Renderer::InitialiseDepthStencilBuffer()
{
	const VkFormat depthStencilFormat = FindDepthStencilFormat();
	
	vulkan_util::CreateImage(m_vulkanContext, m_swapchainExtent.width, m_swapchainExtent.height, depthStencilFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, m_depthStencilBuffer.image, m_depthStencilBuffer.allocation);
	m_depthStencilBuffer.imageView = vulkan_util::CreateImageView(m_vulkanContext, m_depthStencilBuffer.image, depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	vulkan_util::TransitionImageLayout(m_vulkanContext, m_depthStencilBuffer.image, depthStencilFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_supportsStencil);
}

[[nodiscard]] VkFormat Renderer::FindDepthStencilFormat()
{
	static const std::vector<VkFormat> candidateDepthStencilFormats{
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	const VkFormat supportedDepthStencilFormat = vulkan_util::FindSupportedFormat(m_vulkanContext, candidateDepthStencilFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	if (supportedDepthStencilFormat != VK_FORMAT_D32_SFLOAT && supportedDepthStencilFormat != VK_FORMAT_D16_UNORM)
	{
		m_supportsStencil = true;
	}

	return supportedDepthStencilFormat;
}

void Renderer::DestroyDepthStencilBuffer() noexcept
{
	if (m_depthStencilBuffer.image != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_vulkanContext.GetLogicalDevice(), m_depthStencilBuffer.imageView, nullptr);
		m_depthStencilBuffer.imageView = VK_NULL_HANDLE;

		vmaDestroyImage(m_vulkanContext.GetAllocator(), m_depthStencilBuffer.image, m_depthStencilBuffer.allocation);
		m_depthStencilBuffer.image = VK_NULL_HANDLE;
		m_depthStencilBuffer.allocation = VK_NULL_HANDLE;
	}
}

void Renderer::CleanupPresentationObjects() noexcept
{
	FinaliseRenderOperations();

	DeallocateCommandBuffers();
	DestroyFramebuffers();
	DestroyDepthStencilBuffer();
	DestroyRenderPass();
	DestroySwapchain();
}

void Renderer::RecreatePresentationObjects()
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities{ };
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vulkanContext.GetPhysicalDevice(), m_vulkanContext.GetSurface(), &surfaceCapabilities);

	glm::uvec2 drawableWindowSize = m_window.GetDrawableSize();

	while (m_window.IsMinimised() || surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0 ||
		   (surfaceCapabilities.currentExtent.width == std::numeric_limits<std::uint32_t>::max() && drawableWindowSize.x == 0) ||
		   (surfaceCapabilities.currentExtent.height == std::numeric_limits<std::uint32_t>::max() && drawableWindowSize.y == 0))
	{
		drawableWindowSize = m_window.GetDrawableSize();
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vulkanContext.GetPhysicalDevice(), m_vulkanContext.GetSurface(), &surfaceCapabilities);

		SDL_WaitEvent(nullptr);
	}

	CleanupPresentationObjects();

	InitialiseSwapchain();
	InitialiseRenderPass();
	InitialiseDepthStencilBuffer();
	InitialiseFramebuffers();
	AllocateCommandBuffers();
}