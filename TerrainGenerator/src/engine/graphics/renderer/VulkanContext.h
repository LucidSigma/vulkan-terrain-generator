#pragma once

#include "../../utility/interfaces/INoncopyable.h"
#include "../../utility/interfaces/INonmovable.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "../../window/Window.h"

class VulkanContext
	: private INoncopyable, private INonmovable
{
public:
	struct SurfaceProperties
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities{ };

		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentationModes;
	};

	struct QueueFamilyIndices
	{
		std::optional<std::uint32_t> graphicsFamilyIndex = std::nullopt;
		std::optional<std::uint32_t> presentationFamilyIndex = std::nullopt;
	};

private:
	static constexpr std::array<const char*, 1u> s_RequiredDeviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkInstance m_instance = VK_NULL_HANDLE;

	const Window& m_window;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties{ };
	VkDevice m_logicalDevice = VK_NULL_HANDLE;

	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	VmaAllocator m_allocator = VK_NULL_HANDLE;

	QueueFamilyIndices m_queueFamilyIndices{ };
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentationQueue = VK_NULL_HANDLE;

public:
	VulkanContext(const Window& window);
	~VulkanContext() noexcept;

	[[nodiscard]] SurfaceProperties GetSurfaceProperties(const VkPhysicalDevice physicalDevice) const;

	void WaitOnLogicalDevice() const;
	void WaitOnGraphicsQueue() const;
	void WaitOnPresentationQueue() const;

	inline VkSurfaceKHR GetSurface() const noexcept { return m_surface; }

	inline VkPhysicalDevice GetPhysicalDevice() const noexcept { return m_physicalDevice; }
	inline VkDevice GetLogicalDevice() const noexcept { return m_logicalDevice; }

	inline VkCommandPool GetCommandPool() const noexcept { return m_commandPool; }
	inline VmaAllocator GetAllocator() const noexcept { return m_allocator; }

	inline const QueueFamilyIndices& GetQueueFamilyIndices() const noexcept { return m_queueFamilyIndices; }
	inline VkQueue GetGraphicsQueue() const noexcept { return m_graphicsQueue; }
	inline VkQueue GetPresentationQueue() const noexcept { return m_presentationQueue; }

private:
	static bool AreQueueFamilyIndicesComplete(const QueueFamilyIndices& queueFamilyIndices);

	void InitialiseInstance();
	[[nodiscard]] std::vector<const char*> GetRequiredInstanceExtensions() const;
	void DestroyInstance() noexcept;

	void InitialiseSurface();
	void DestroySurface() noexcept;

	void SelectPhysicalDevice();
	[[nodiscard]] std::uint64_t GetPhysicalDeviceScore(const VkPhysicalDevice physicalDevice) const;
	[[nodiscard]] QueueFamilyIndices FindQueueFamilyIndices(const VkPhysicalDevice physicalDevice) const;
	bool SupportsRequiredDeviceExtensions(const VkPhysicalDevice physicalDevice) const;

	void InitialiseDevice();
	void DestroyDevice();

	void InitialiseCommandPool();
	void DestroyCommandPool() noexcept;
	void InitialiseAllocator();
	void DestroyAllocator() noexcept;

	std::uint32_t GetMemoryTypeIndex(const std::uint32_t memoryTypeFilter, const VkMemoryPropertyFlags& memoryPropertyFlags) const;
};