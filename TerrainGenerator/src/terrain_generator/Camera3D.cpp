#include "Camera3D.h"

#include <algorithm>
#include <limits>

#include <SDL2/SDL.h>

Camera3D::Camera3D(const glm::vec3& position, const glm::vec3& up, const float yaw, const float pitch)
	: m_position(position), m_worldUp(up), m_yaw(yaw), m_pitch(pitch)
{
	UpdateVectors();
}

void Camera3D::ProcessInput()
{
	const Uint8* keyStates = SDL_GetKeyboardState(nullptr);

	m_velocity = glm::vec3{ 0.0f, 0.0f, 0.0f };
	m_movementSpeed = keyStates[SDL_SCANCODE_LSHIFT] ? s_DefaultSpeed * 4.0f : s_DefaultSpeed;

	if (keyStates[SDL_SCANCODE_W])
	{
		m_velocity += m_front * m_movementSpeed;
	}

	if (keyStates[SDL_SCANCODE_S])
	{
		m_velocity -= m_front * m_movementSpeed;
	}

	if (keyStates[SDL_SCANCODE_A])
	{
		m_velocity += m_right * m_movementSpeed;
	}

	if (keyStates[SDL_SCANCODE_D])
	{
		m_velocity -= m_right * m_movementSpeed;
	}

	int relativeMouseX = 0;
	int relativeMouseY = 0;
	SDL_GetRelativeMouseState(&relativeMouseX, &relativeMouseY);
	
	const float xOffset = relativeMouseX * m_mouseSensitivity;
	const float yOffset = relativeMouseY * m_mouseSensitivity;
	m_yaw -= xOffset;
	m_pitch -= yOffset;

	constexpr float MaxPitch = 89.5f;
	constexpr float MinPitch = -89.5f;

	m_pitch = std::clamp(m_pitch, MinPitch, MaxPitch);

	UpdateVectors();
}

void Camera3D::Update(const float deltaTime)
{
	m_position += m_velocity * deltaTime;
}

void Camera3D::UpdateVectors()
{
	glm::vec3 updatedFront{
		glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch)),
		glm::sin(glm::radians(m_pitch)),
		glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch))
	};

	m_front = glm::normalize(updatedFront);
	m_right = glm::normalize(glm::cross(m_front, m_worldUp));
	m_up = glm::normalize(glm::cross(m_right, m_front));
}