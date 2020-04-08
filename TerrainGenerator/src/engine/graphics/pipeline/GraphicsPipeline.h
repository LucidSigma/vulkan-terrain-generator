#pragma once

#include "../../utility/interfaces/INoncopyable.h"
#include "../../utility/interfaces/INonmovable.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <spirv-cross/spirv_cross.hpp>
#include <vulkan/vulkan.h>

#include "../buffers/UniformBuffer.h"
#include "ShaderModule.h"

class GraphicsPipeline
	: private INoncopyable, private INonmovable
{
public:
	struct Config
	{
		std::vector<std::pair<std::string, ShaderModule::Stage>> shaderInfo;

		bool enableDepthTest = true;
		bool drawWireframe = false;
		bool enableCullFace = true;
		bool enableBlending = true;
	};

private:
	struct VertexInputData
	{
		std::uint32_t location;

		std::uint32_t size;
		VkFormat format;

		inline bool operator <(const VertexInputData& other) const noexcept { return this->location < other.location; }
	};

	struct DescriptorSetBindingData
	{
		std::optional<std::uint32_t> size;
		VkDescriptorType type;
		VkShaderStageFlags shaderStages;
	};

	inline static VkDeviceSize s_minOffsetAlignment = std::numeric_limits<VkDeviceSize>::max();
	inline static std::uint32_t s_maxPushConstantBufferSize = 0;

	const class Renderer& m_renderer;

	VkPipeline m_pipelineHandle = VK_NULL_HANDLE;

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkPushConstantRange m_pushConstantRange{ };
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

	std::vector<std::unique_ptr<UniformBuffer>> m_uniformBuffers;
	std::vector<VkDescriptorSet> m_descriptorSets;

	std::map<std::uint32_t, DescriptorSetBindingData> m_bindingsData;
	std::unordered_map<VkDescriptorType, std::uint32_t> m_descriptorTypeCounts;

public:
	GraphicsPipeline(const class Renderer& renderer, const Config& config);
	~GraphicsPipeline() noexcept;
	
	void Destroy() noexcept;
	
	void RefreshUniformBuffers();

	template <typename T>
	inline void SetUniform(const std::uint32_t binding, const T& data)
	{
		SetUniformBufferData(binding, &data);
	}

	inline VkPipeline GetHandle() const noexcept { return m_pipelineHandle; }
	inline VkPipelineLayout GetLayout() const noexcept { return m_pipelineLayout; }
	inline const std::vector<VkDescriptorSet>& GetDescriptorSets() const noexcept { return m_descriptorSets; }

	inline VkShaderStageFlags GetPushConstantStageFlags() const noexcept { return m_pushConstantRange.stageFlags; }

private:
	static std::uint32_t GetVertexInputSize(const spirv_cross::SPIRType& vertexInputType);
	static VkFormat GetVertexInputFormat(const spirv_cross::SPIRType& vertexInputType);

	std::pair< VkVertexInputBindingDescription, std::vector<VkVertexInputAttributeDescription>> GetVertexInputData(const std::vector<std::unique_ptr<ShaderModule>>& shaderModules);

	void InitialiseDescriptorSetLayouts(const Config& config, const std::vector<std::unique_ptr<ShaderModule>>& shaderModules);
	void DestroyDescriptorSetLayout() noexcept;

	void InitialisePipelineLayout(const Config& config, const std::vector<std::unique_ptr<ShaderModule>>& shaderModules);
	void DestroyPipelineLayout() noexcept;

	void InitialisePipeline(const Config& config);
	void DestroyPipeline() noexcept;

	void InitialiseDescriptorPool();
	void DestroyDescriptorPool() noexcept;

	void InitialiseUniformBuffers();
	void ResizeUniformBuffers();
	void DestroyUniformBuffers() noexcept;

	void InitialiseDescriptorSets();

	void SetUniformBufferData(const std::uint32_t updatedBinding, const void* data);
};