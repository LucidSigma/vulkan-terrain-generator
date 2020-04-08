#include "VulkanValidationLayers.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <SDL2/SDL.h>

#define getVulkanInstanceProcessAddress(functionName) reinterpret_cast<PFN_ ## functionName>(vkGetInstanceProcAddr(instance, #functionName))

bool VulkanValidationLayers::AreSupported()
{
	std::uint32_t validationLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

	std::vector<VkLayerProperties> availableValidationLayers(validationLayerCount);
	vkEnumerateInstanceLayerProperties(&validationLayerCount, availableValidationLayers.data());

	for (const auto validationLayerName : s_ValidationLayers)
	{
		bool validationLayerFound = false;

		for (const auto& validationLayerProperties : availableValidationLayers)
		{
			if (std::strcmp(validationLayerName, validationLayerProperties.layerName) == 0)
			{
				validationLayerFound = true;

				break;
			}
		}

		if (!validationLayerFound)
		{
			return false;
		}
	}

	return true;
}

VkResult VulkanValidationLayers::CreateDebugUtilsMessenger(const VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfo)
{
	const auto createFunction = getVulkanInstanceProcessAddress(vkCreateDebugUtilsMessengerEXT);

	return createFunction != nullptr ? createFunction(instance, &debugUtilsMessengerCreateInfo, nullptr, &s_debugUtilsMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanValidationLayers::FillInDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfo)
{
	debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugUtilsMessengerCreateInfo.pfnUserCallback = DebugCallback;
	debugUtilsMessengerCreateInfo.pUserData = nullptr;
}

void VulkanValidationLayers::SetupDebugUtilsMessenger(const VkInstance instance)
{
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{ };
	FillInDebugMessengerCreateInfo(debugUtilsMessengerCreateInfo);

	if (CreateDebugUtilsMessenger(instance, debugUtilsMessengerCreateInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to setup debug messenger.");
	}
}

void VulkanValidationLayers::DestroyDebugUtilsMessenger(const VkInstance instance) noexcept
{
	const auto destroyFunction = getVulkanInstanceProcessAddress(vkDestroyDebugUtilsMessengerEXT);

	if (destroyFunction != nullptr && s_debugUtilsMessenger != VK_NULL_HANDLE)
	{
		destroyFunction(instance, s_debugUtilsMessenger, nullptr);
		s_debugUtilsMessenger = VK_NULL_HANDLE;
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanValidationLayers::DebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* const callbackData, void* const userData) noexcept
{
	std::string errorBoxMessage(callbackData->pMessage);
	constexpr std::size_t MaxErrorBoxMessageLineLength = 100u;
	std::size_t currentErrorBoxMessageLineLength = 0;

	for (std::size_t i = 0; i < errorBoxMessage.length(); ++i)
	{
		if (errorBoxMessage[i] == ' ' && currentErrorBoxMessageLineLength > MaxErrorBoxMessageLineLength)
		{
			errorBoxMessage[i] = '\n';
			currentErrorBoxMessageLineLength = 0;
		}
		else
		{
			++currentErrorBoxMessageLineLength;
		}
	}

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vulkan Validation Error", errorBoxMessage.c_str(), nullptr);
	std::cerr << "Validation layer: " << callbackData->pMessage << "\n";

	return VK_FALSE;
}