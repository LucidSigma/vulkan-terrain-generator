#pragma once

#include "../utility/interfaces/INoncopyable.h"
#include "../utility/interfaces/INonmovable.h"

#include <optional>
#include <string_view>

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

class Window
	: private INoncopyable, private INonmovable
{
private:
	SDL_Window* m_windowHandle = nullptr;

	glm::uvec2 m_size{ 0, 0 };
	std::optional<glm::uvec2> m_sizeBeforeFullscreen = std::nullopt;

	bool m_isFullscreen = false;
	bool m_hasUpdatedFullscreen = false;

public:
	Window() = default;
	Window(const std::string_view& title, const glm::uvec2& size, const bool isFullscreen = false);

	~Window() noexcept;

	void Create(const std::string_view& title, const glm::uvec2& size, const bool isFullscreen = false);
	void Destroy() noexcept;

	void ToggleFullscreen();

	void ChangeSize(const glm::uvec2& newSize);
	void ProcessResize(const glm::uvec2& newSize);

	void SetIcon(const std::string_view& iconFilepath) const;
	void SetBordered(const bool isBordered) const;
	
	[[nodiscard]] VkSurfaceKHR CreateVulkanSurface(const VkInstance& vulkanInstance) const;

	inline SDL_Window* GetHandle() const noexcept { return m_windowHandle; }
	inline const glm::uvec2& GetSize() const noexcept { return m_size; }
	glm::uvec2 GetDrawableSize() const;
	
	inline bool IsMinimised() const noexcept { return SDL_GetWindowFlags(m_windowHandle) & SDL_WINDOW_MINIMIZED; }
};