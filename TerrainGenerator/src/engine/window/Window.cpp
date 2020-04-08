#include "Window.h"

#include <cstdint>
#include <stdexcept>
#include <string>

#include <stb_image/stb_image.h>

Window::Window(const std::string_view& title, const glm::uvec2& size, const bool isFullscreen)
{
	Create(title, size, isFullscreen);
}

Window::~Window() noexcept
{
	Destroy();
}

void Window::Create(const std::string_view& title, const glm::uvec2& size, const bool isFullscreen)
{
	m_windowHandle = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, size.x, size.y, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);

	if (m_windowHandle == nullptr)
	{
		using namespace std::literals::string_literals;

		throw std::runtime_error("Failed to create SDL window: "s + SDL_GetError());
	}

	m_size = size;

	if (isFullscreen)
	{
		ToggleFullscreen();
	}
}

void Window::Destroy() noexcept
{
	if (m_windowHandle != nullptr)
	{
		SDL_DestroyWindow(m_windowHandle);
		m_windowHandle = nullptr;

		m_size = glm::vec2{ 0, 0 };
		m_sizeBeforeFullscreen = std::nullopt;
		m_isFullscreen = false;
		m_hasUpdatedFullscreen = false;
	}
}

void Window::ToggleFullscreen()
{
	if (m_isFullscreen)
	{
		SDL_SetWindowSize(m_windowHandle, m_sizeBeforeFullscreen->x, m_sizeBeforeFullscreen->y);
		SDL_SetWindowFullscreen(m_windowHandle, 0);
		SetBordered(true);

		m_size = m_sizeBeforeFullscreen.value();
		m_sizeBeforeFullscreen = std::nullopt;
		m_hasUpdatedFullscreen = true;
	}
	else
	{
		SDL_DisplayMode displayMode{ };
		SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(m_windowHandle), &displayMode);

		m_sizeBeforeFullscreen = m_size;
		m_size = glm::uvec2{ displayMode.w, displayMode.h };

		SDL_SetWindowSize(m_windowHandle, m_size.x, m_size.y);
		SDL_SetWindowFullscreen(m_windowHandle, SDL_WINDOW_FULLSCREEN);
		SetBordered(false);

		m_hasUpdatedFullscreen = true;
	}

	m_isFullscreen = !m_isFullscreen;
}

void Window::ChangeSize(const glm::uvec2& newSize)
{
	SDL_SetWindowSize(m_windowHandle, newSize.x, newSize.y);
	ProcessResize(newSize);
}

void Window::ProcessResize(const glm::uvec2& newSize)
{
	if (!m_hasUpdatedFullscreen)
	{
		m_size = newSize;
	}
	else
	{
		m_hasUpdatedFullscreen = false;
	}
}

void Window::SetIcon(const std::string_view& iconFilepath) const
{
	int iconWidth = 0;
	int iconHeight = 0;
	stbi_uc* iconData = stbi_load(iconFilepath.data(), &iconWidth, &iconHeight, nullptr, STBI_rgb_alpha);

	if (iconData == nullptr)
	{
		throw std::runtime_error("Failed to load icon file: " + std::string(iconFilepath));
	}

	const std::uint32_t redMask = SDL_BYTEORDER == SDL_LIL_ENDIAN ? 0x00'00'00'FF : 0xFF'00'00'00;
	const std::uint32_t greenMask = SDL_BYTEORDER == SDL_LIL_ENDIAN ? 0x00'00'FF'00 : 0x00'FF'00'00;
	const std::uint32_t blueMask = SDL_BYTEORDER == SDL_LIL_ENDIAN ? 0x00'FF'00'00 : 0x00'00'FF'00;
	const std::uint32_t alphaMask = SDL_BYTEORDER == SDL_LIL_ENDIAN ? 0xFF'00'00'00 : 0x00'00'00'FF;

	SDL_Surface* iconSurface = SDL_CreateRGBSurfaceFrom(iconData, iconWidth, iconHeight, sizeof(std::uint32_t) * 8, iconWidth * 4, redMask, greenMask, blueMask, alphaMask);
	SDL_SetWindowIcon(m_windowHandle, iconSurface);

	SDL_FreeSurface(iconSurface);
	iconSurface = nullptr;

	stbi_image_free(iconData);
	iconData = nullptr;
}

void Window::SetBordered(const bool isBordered) const
{
	SDL_SetWindowBordered(m_windowHandle, isBordered ? SDL_TRUE : SDL_FALSE);
}

VkSurfaceKHR Window::CreateVulkanSurface(const VkInstance& vulkanInstance) const
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	if (!SDL_Vulkan_CreateSurface(m_windowHandle, vulkanInstance, &surface))
	{
		using namespace std::literals::string_literals;

		throw std::runtime_error("Failed to create Vulkan surface from SDL window: "s + SDL_GetError());
	}

	return surface;
}

glm::uvec2 Window::GetDrawableSize() const
{
	int drawableWidth = 0;
	int drawableHeight = 0;
	SDL_Vulkan_GetDrawableSize(m_windowHandle, &drawableWidth, &drawableHeight);

	return glm::uvec2{ drawableWidth, drawableHeight };
}