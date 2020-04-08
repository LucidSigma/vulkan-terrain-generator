#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera3D
{
private:
	static constexpr float s_DefaultYaw = 90.0f;
	static constexpr float s_DefaultPitch = 0.0f;
	static constexpr float s_DefaultSpeed = 50.0f;
	static constexpr float s_DefaultSensitivity = 0.1f;
	static constexpr float s_DefaultZoom = 45.0f;

	glm::vec3 m_position{ 0.0f, 0.0f, 0.0f };
	glm::vec3 m_front{ 0.0f, 0.0f, 0.0f };
	glm::vec3 m_up{ 0.0f, 1.0f, 0.0f };
	glm::vec3 m_right{ 0.0f, 0.0f, 0.0f };
	glm::vec3 m_worldUp{ 0.0f, 1.0f, 0.0f };

	glm::vec3 m_velocity{ 0.0f, 0.0f, 0.0f };

	float m_yaw;
	float m_pitch;

	float m_movementSpeed = s_DefaultSpeed;
	float m_mouseSensitivity = s_DefaultSensitivity;
	float m_zoom = s_DefaultZoom;

public:
	Camera3D(const glm::vec3& position = glm::vec3{ 0.0f, 0.0f, 0.0f }, const glm::vec3& up = glm::vec3{ 0.0f, 1.0f, 0.0f }, const float yaw = s_DefaultYaw, const float pitch = s_DefaultPitch);
	~Camera3D() noexcept = default;

	void ProcessInput();
	void Update(const float deltaTime);

	inline float GetZoom() const noexcept { return m_zoom; }
	inline const glm::vec3& GetPosition() const noexcept { return m_position; }
	inline const glm::vec3& GetFront() const noexcept { return m_front; }
	inline glm::mat4 GetViewMatrix() const noexcept { return glm::lookAtLH(m_position, m_position + m_front, m_up); }

private:
	void UpdateVectors();
};