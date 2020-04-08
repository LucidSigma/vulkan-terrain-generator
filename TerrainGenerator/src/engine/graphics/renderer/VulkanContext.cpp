#include "VulkanContext.h"

#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "VulkanValidationLayers.h"

VulkanContext::VulkanContext(const Window& window)
	: m_window(window)
{
	InitialiseInstance();

	if constexpr (VulkanValidationLayers::AreEnabled())
	{
		VulkanValidationLayers::SetupDebugUtilsMessenger(m_instance);
	}

	InitialiseSurface();
	InitialiseDevice();

	InitialiseCommandPool();
	InitialiseAllocator();
}

VulkanContext::~VulkanContext() noexcept
{
	DestroyAllocator();
	DestroyCommandPool();

	DestroyDevice();
	DestroySurface();

	if constexpr (VulkanValidationLayers::AreEnabled())
	{
		VulkanValidationLayers::DestroyDebugUtilsMessenger(m_instance);
	}

	DestroyInstance();
}

[[nodiscard]] VulkanContext::SurfaceProperties VulkanContext::GetSurfaceProperties(const VkPhysicalDevice physicalDevice) const
{
	SurfaceProperties surfaceProperties{ };
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &surfaceProperties.surfaceCapabilities);

	std::uint32_t surfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &surfaceFormatCount, nullptr);

	if (surfaceFormatCount > 0)
	{
		surfaceProperties.surfaceFormats.resize(surfaceFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &surfaceFormatCount, surfaceProperties.surfaceFormats.data());
	}

	std::uint32_t presentationModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentationModeCount, nullptr);

	if (presentationModeCount > 0)
	{
		surfaceProperties.presentationModes.resize(presentationModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentationModeCount, surfaceProperties.presentationModes.data());
	}

	return surfaceProperties;
}

void VulkanContext::WaitOnLogicalDevice() const
{
	vkDeviceWaitIdle(m_logicalDevice);
}

void VulkanContext::WaitOnGraphicsQueue() const
{
	vkQueueWaitIdle(m_graphicsQueue);
}

void VulkanContext::WaitOnPresentationQueue() const
{
	vkQueueWaitIdle(m_presentationQueue);
}

bool VulkanContext::AreQueueFamilyIndicesComplete(const QueueFamilyIndices& queueFamilyIndices)
{
	return queueFamilyIndices.graphicsFamilyIndex.has_value() && queueFamilyIndices.presentationFamilyIndex.has_value();
}

void VulkanContext::InitialiseInstance()
{
	if constexpr (VulkanValidationLayers::AreEnabled())
	{
		if (!VulkanValidationLayers::AreSupported())
		{
			throw std::runtime_error("Validation layers were requested, but are not available.");
		}
	}

	VkApplicationInfo applicationInfo{ };
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "Vulkan Engine";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	applicationInfo.pEngineName = "Stardust";
	applicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	applicationInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceCreateInfo{ };
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;

	const auto requiredInstanceExtensions = GetRequiredInstanceExtensions();
	instanceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(requiredInstanceExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{ };

	if constexpr (VulkanValidationLayers::AreEnabled())
	{
		VulkanValidationLayers::FillInDebugMessengerCreateInfo(debugUtilsMessengerCreateInfo);

		instanceCreateInfo.enabledLayerCount = static_cast<std::uint32_t>(VulkanValidationLayers::GetValidationLayers().size());
		instanceCreateInfo.ppEnabledLayerNames = VulkanValidationLayers::GetValidationLayers().data();
		instanceCreateInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugUtilsMessengerCreateInfo);
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.ppEnabledLayerNames = nullptr;
		instanceCreateInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan instance.");
	}
}

std::vector<const char*> VulkanContext::GetRequiredInstanceExtensions() const
{
	std::uint32_t requiredSDLInstanceExtensionCount = 0;

	if (!SDL_Vulkan_GetInstanceExtensions(nullptr, &requiredSDLInstanceExtensionCount, nullptr))
	{
		using namespace std::literals::string_literals;

		throw std::runtime_error("Failed to get required SDL Vulkan instance extensions count: "s + SDL_GetError());
	}

	std::vector<const char*> requiredInstanceExtensions(requiredSDLInstanceExtensionCount);

	if (!SDL_Vulkan_GetInstanceExtensions(nullptr, &requiredSDLInstanceExtensionCount, requiredInstanceExtensions.data()))
	{
		using namespace std::literals::string_literals;

		throw std::runtime_error("Failed to get required SDL Vulkan instance extensions' names: "s + SDL_GetError());
	}

	if constexpr (VulkanValidationLayers::AreEnabled())
	{
		requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::uint32_t availableInstanceExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, nullptr);

	std::vector<VkExtensionProperties> availableInstanceExtensions(availableInstanceExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, availableInstanceExtensions.data());

	for (const auto requiredInstanceExtensionName : requiredInstanceExtensions)
	{
		bool extensionFound = false;

		for (const auto& extensionProperties : availableInstanceExtensions)
		{
			if (std::strcmp(requiredInstanceExtensionName, extensionProperties.extensionName) == 0)
			{
				extensionFound = true;

				break;
			}
		}

		if (!extensionFound)
		{
			using namespace std::literals::string_literals;

			throw std::runtime_error("Vulkan extension "s + requiredInstanceExtensionName + " is required but not supported."s);
		}
	}

	return requiredInstanceExtensions;
}

void VulkanContext::DestroyInstance() noexcept
{
	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
		m_physicalDevice = VK_NULL_HANDLE;
	}
}

void VulkanContext::InitialiseSurface()
{
	m_surface = m_window.CreateVulkanSurface(m_instance);
}

void VulkanContext::DestroySurface() noexcept
{
	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}
}

void VulkanContext::SelectPhysicalDevice()
{
	std::uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0)
	{
		throw std::runtime_error("Failed to find physical device with Vulkan support.");
	}

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

	std::multimap<std::uint64_t, VkPhysicalDevice, std::greater<std::uint64_t>> physicalDeviceScores;

	for (const auto& candidatePhysicalDevice : physicalDevices)
	{
		const std::uint64_t score = GetPhysicalDeviceScore(candidatePhysicalDevice);
		physicalDeviceScores.insert({ score, std::cref(candidatePhysicalDevice) });
	}

	if (std::cbegin(physicalDeviceScores)->first > 0)
	{
		m_physicalDevice = std::cbegin(physicalDeviceScores)->second;
		m_queueFamilyIndices = FindQueueFamilyIndices(m_physicalDevice);
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalDeviceMemoryProperties);
	}
	else
	{
		throw std::runtime_error("Failed to find a suitable GPU.");
	}
}

[[nodiscard]] std::uint64_t VulkanContext::GetPhysicalDeviceScore(const VkPhysicalDevice physicalDevice) const
{
	VkPhysicalDeviceFeatures physicalDeviceFeatures{ };
	vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

	const std::vector<VkBool32> requiredPhysicalDeviceFeatures{
		physicalDeviceFeatures.geometryShader,
		physicalDeviceFeatures.tessellationShader,
		physicalDeviceFeatures.fillModeNonSolid,
		physicalDeviceFeatures.depthClamp,
		physicalDeviceFeatures.imageCubeArray,
		physicalDeviceFeatures.shaderStorageImageMultisample,
		physicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing
	};

	for (const auto requiredPhysicalDeviceFeature : requiredPhysicalDeviceFeatures)
	{
		if (!requiredPhysicalDeviceFeature)
		{
			return 0;
		}
	}

	const QueueFamilyIndices queueFamilyIndices = FindQueueFamilyIndices(physicalDevice);
	const bool supportsRequiredDeviceExtensions = SupportsRequiredDeviceExtensions(physicalDevice);

	bool isSurfaceFullySupported = false;

	if (supportsRequiredDeviceExtensions)
	{
		const auto surfaceProperties = GetSurfaceProperties(physicalDevice);
		isSurfaceFullySupported = !surfaceProperties.surfaceFormats.empty() && !surfaceProperties.presentationModes.empty();
	}

	if (!AreQueueFamilyIndicesComplete(queueFamilyIndices) || !supportsRequiredDeviceExtensions || !isSurfaceFullySupported)
	{
		return 0;
	}

	VkPhysicalDeviceProperties physicalDeviceProperties{ };
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	std::uint64_t score = 0;

	switch (physicalDeviceProperties.deviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		score += 10'000;

		break;

	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		score += 1'000;

		break;

	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		score += 100;

		break;

	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		score += 10;

		break;

	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
	default:
		++score;

		break;
	}

	if (queueFamilyIndices.graphicsFamilyIndex == queueFamilyIndices.presentationFamilyIndex)
	{
		score += 1'000;
	}

	score += physicalDeviceProperties.limits.maxImageDimension2D;

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{ };
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	const auto memoryHeapsPointer = physicalDeviceMemoryProperties.memoryHeaps;
	const std::vector<VkMemoryHeap> memoryHeaps(memoryHeapsPointer, memoryHeapsPointer + physicalDeviceMemoryProperties.memoryHeapCount);

	for (const auto& memoryHeap : memoryHeaps)
	{
		if (memoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
		{
			score += memoryHeap.size / 1'000'000;
		}
	}

	return score;
}

[[nodiscard]] VulkanContext::QueueFamilyIndices VulkanContext::FindQueueFamilyIndices(const VkPhysicalDevice physicalDevice) const
{
	QueueFamilyIndices queueFamilyIndices{ };

	std::uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	std::uint32_t currentIndex = 0;

	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT))
		{
			queueFamilyIndices.graphicsFamilyIndex = currentIndex;
		}

		VkBool32 supportsPresentation = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, currentIndex, m_surface, &supportsPresentation);

		if (supportsPresentation)
		{
			queueFamilyIndices.presentationFamilyIndex = currentIndex;
		}

		if (AreQueueFamilyIndicesComplete(queueFamilyIndices))
		{
			break;
		}

		++currentIndex;
	}

	return queueFamilyIndices;
}

bool VulkanContext::SupportsRequiredDeviceExtensions(const VkPhysicalDevice physicalDevice) const
{
	std::uint32_t availableDeviceExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableDeviceExtensionCount, nullptr);

	std::vector<VkExtensionProperties> availableDeviceExtensions(availableDeviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableDeviceExtensionCount, availableDeviceExtensions.data());

	std::unordered_set<std::string> requiredDeviceExtensions(std::cbegin(s_RequiredDeviceExtensions), std::cend(s_RequiredDeviceExtensions));

	for (const auto& availableDeviceExtension : availableDeviceExtensions)
	{
		requiredDeviceExtensions.erase(availableDeviceExtension.extensionName);

		if (requiredDeviceExtensions.empty())
		{
			return true;
		}
	}

	return false;
}

void VulkanContext::InitialiseDevice()
{
	SelectPhysicalDevice();

	const std::array<float, 1u> queuePriorities{ 1.0f };
	const std::unordered_set<std::uint32_t> uniqueQueueFamilyIndices{ m_queueFamilyIndices.graphicsFamilyIndex.value(), m_queueFamilyIndices.presentationFamilyIndex.value() };
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	deviceQueueCreateInfos.reserve(uniqueQueueFamilyIndices.size());

	for (const auto queueFamilyIndex : uniqueQueueFamilyIndices)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo{ };
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.queueCount = 1;
		deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		deviceQueueCreateInfo.pQueuePriorities = queuePriorities.data();

		deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
	}

	VkPhysicalDeviceFeatures physicalDeviceFeatures{ };
	physicalDeviceFeatures.geometryShader = VK_TRUE;
	physicalDeviceFeatures.tessellationShader = VK_TRUE;
	physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;
	physicalDeviceFeatures.depthClamp = VK_TRUE;
	physicalDeviceFeatures.imageCubeArray = VK_TRUE;
	physicalDeviceFeatures.shaderStorageImageMultisample = VK_TRUE;
	physicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo{ };
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<std::uint32_t>(deviceQueueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(s_RequiredDeviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = s_RequiredDeviceExtensions.data();

	if constexpr (VulkanValidationLayers::AreEnabled())
	{
		deviceCreateInfo.enabledLayerCount = static_cast<std::uint32_t>(VulkanValidationLayers::GetValidationLayers().size());
		deviceCreateInfo.ppEnabledLayerNames = VulkanValidationLayers::GetValidationLayers().data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
	}

	if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_logicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical Vulkan device.");
	}

	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.graphicsFamilyIndex.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.presentationFamilyIndex.value(), 0, &m_presentationQueue);
}

void VulkanContext::DestroyDevice()
{
	if (m_logicalDevice != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_logicalDevice, nullptr);
		m_logicalDevice = VK_NULL_HANDLE;

		m_graphicsQueue = VK_NULL_HANDLE;
		m_presentationQueue = VK_NULL_HANDLE;
	}
}

void VulkanContext::InitialiseCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo{ };
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamilyIndex.value();
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_logicalDevice, &commandPoolCreateInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan command pool.");
	}
}

void VulkanContext::DestroyCommandPool() noexcept
{
	if (m_commandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
		m_commandPool = VK_NULL_HANDLE;
	}
}

void VulkanContext::InitialiseAllocator()
{
	VmaAllocatorCreateInfo allocatorCreateInfo{ };
	allocatorCreateInfo.physicalDevice = m_physicalDevice;
	allocatorCreateInfo.device = m_logicalDevice;

	if (vmaCreateAllocator(&allocatorCreateInfo, &m_allocator) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan allocator.");
	}
}

void VulkanContext::DestroyAllocator() noexcept
{
	if (m_allocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(m_allocator);
		m_allocator = VK_NULL_HANDLE;
	}
}

std::uint32_t VulkanContext::GetMemoryTypeIndex(const std::uint32_t memoryTypeFilter, const VkMemoryPropertyFlags& memoryPropertyFlags) const
{
	for (std::uint32_t i = 0; i < m_physicalDeviceMemoryProperties.memoryTypeCount; ++i)
	{
		if ((memoryTypeFilter & (1 << i)) && (m_physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type.");
}