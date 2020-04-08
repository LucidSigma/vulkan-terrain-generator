#pragma once

#include "../../utility/interfaces/INoncopyable.h"
#include "../../utility/interfaces/INonmovable.h"

#include <array>

#include <vulkan/vulkan.h>

class VulkanValidationLayers
	: private INoncopyable, private INonmovable
{
private:
#if defined(NDEBUG) || defined(__APPLE__)
	static constexpr bool s_EnableValidationLayers = false;
#else
	static constexpr bool s_EnableValidationLayers = true;
#endif

	static constexpr std::array<const char*, 1u> s_ValidationLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	inline static VkDebugUtilsMessengerEXT s_debugUtilsMessenger = VK_NULL_HANDLE;

public:
	VulkanValidationLayers() = delete;
	~VulkanValidationLayers() noexcept = delete;

	static constexpr bool AreEnabled() noexcept { return s_EnableValidationLayers; }
	static constexpr const decltype(s_ValidationLayers)& GetValidationLayers() noexcept { return s_ValidationLayers; }

	static bool AreSupported();

	static VkResult CreateDebugUtilsMessenger(const VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfo);
	static void FillInDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfo);
	static void SetupDebugUtilsMessenger(const VkInstance instance);
	static void DestroyDebugUtilsMessenger(const VkInstance instance) noexcept;

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* const callbackData, void* const userData) noexcept;
};