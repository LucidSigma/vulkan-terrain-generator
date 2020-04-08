#include "ShaderModule.h"

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

ShaderModule::ShaderModule(const VkDevice vulkanDevice, const std::string& shaderFilepath, const Stage stage)
	: m_vulkanDevice(vulkanDevice), m_stage(stage)
{
	std::vector<std::uint32_t> shaderData = ReadSpirVFile(shaderFilepath);

	VkShaderModuleCreateInfo shaderModuleCreateInfo{ };
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = shaderData.size() * sizeof(std::uint32_t);
	shaderModuleCreateInfo.pCode = reinterpret_cast<const std::uint32_t*>(shaderData.data());

	if (vkCreateShaderModule(m_vulkanDevice, &shaderModuleCreateInfo, nullptr, &m_moduleHandle) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan shader module.");
	}

	m_dataReflector = std::make_unique<spirv_cross::CompilerGLSL>(std::move(shaderData));
}

ShaderModule::~ShaderModule() noexcept
{
	Destroy();
}

[[nodiscard]] VkPipelineShaderStageCreateInfo ShaderModule::GetCreateInfo() const
{
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo{ };
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = static_cast<VkShaderStageFlagBits>(m_stage);
	shaderStageCreateInfo.module = m_moduleHandle;
	shaderStageCreateInfo.pName = "main";
	shaderStageCreateInfo.pSpecializationInfo = nullptr;

	return shaderStageCreateInfo;
}

[[nodiscard]] std::vector<std::uint32_t> ShaderModule::ReadSpirVFile(const std::string& filepath)
{
	std::ifstream shaderFile(filepath, std::ios_base::in | std::ios_base::ate | std::ios_base::binary);

	if (!shaderFile.is_open())
	{
		throw std::runtime_error("Failed to open shader file " + filepath + ".");
	}

	const std::size_t fileSize = static_cast<std::size_t>(shaderFile.tellg());
	std::vector<std::uint32_t> shaderData(fileSize / sizeof(std::uint32_t));

	shaderFile.seekg(0);
	shaderFile.read(reinterpret_cast<char*>(shaderData.data()), fileSize);
	shaderFile.close();

	return shaderData;
}

void ShaderModule::Destroy() noexcept
{
	if (m_moduleHandle != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_vulkanDevice, m_moduleHandle, nullptr);
		m_moduleHandle = VK_NULL_HANDLE;
	}

	m_dataReflector = nullptr;
}