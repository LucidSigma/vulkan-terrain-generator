#pragma once

#include "../../utility/interfaces/INoncopyable.h"
#include "../../utility/interfaces/INonmovable.h"

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <spirv-cross/spirv_glsl.hpp>
#include <vulkan/vulkan.h>

class ShaderModule
	: private INoncopyable, private INonmovable
{
public:
	enum class Stage
		: std::underlying_type_t<VkShaderStageFlagBits>
	{
		None = 0,
		Vertex = VK_SHADER_STAGE_VERTEX_BIT,
		TessellationControl = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		TessellationEvaluation = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
		Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
		Compute = VK_SHADER_STAGE_COMPUTE_BIT
	};

private:
	VkDevice m_vulkanDevice = VK_NULL_HANDLE;

	VkShaderModule m_moduleHandle = VK_NULL_HANDLE;
	Stage m_stage = Stage::None;

	std::unique_ptr<spirv_cross::CompilerGLSL> m_dataReflector = nullptr;

public:
	ShaderModule(const VkDevice vulkanDevice, const std::string& shaderFilepath, const Stage stage);
	~ShaderModule() noexcept;

	[[nodiscard]] VkPipelineShaderStageCreateInfo GetCreateInfo() const;	
	
	inline Stage GetStage() const noexcept { return m_stage; }
	inline const std::unique_ptr<spirv_cross::CompilerGLSL>& GetDataReflector() const noexcept { return m_dataReflector; }

private:
	[[nodiscard]] std::vector<std::uint32_t> ReadSpirVFile(const std::string& filepath);

	void Destroy() noexcept;
};