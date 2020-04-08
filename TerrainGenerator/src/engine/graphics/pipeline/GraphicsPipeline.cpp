#include "GraphicsPipeline.h"

#include <array>
#include <iterator>
#include <set>
#include <stdexcept>

#include "../renderer/Renderer.h"

GraphicsPipeline::GraphicsPipeline(const Renderer& renderer, const Config& config)
	: m_renderer(renderer)
{
	if (s_minOffsetAlignment == std::numeric_limits<VkDeviceSize>::max() || s_maxPushConstantBufferSize == 0)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties{ };
		vkGetPhysicalDeviceProperties(m_renderer.GetVulkanContext().GetPhysicalDevice(), &physicalDeviceProperties);

		s_minOffsetAlignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
		s_maxPushConstantBufferSize = physicalDeviceProperties.limits.maxPushConstantsSize;
	}

	InitialisePipeline(config);
	InitialiseDescriptorPool();
	InitialiseUniformBuffers();
}

GraphicsPipeline::~GraphicsPipeline() noexcept
{
	Destroy();
}

void GraphicsPipeline::Destroy() noexcept
{
	DestroyUniformBuffers();
	DestroyDescriptorPool();

	DestroyPipeline();
}

void GraphicsPipeline::RefreshUniformBuffers()
{
	m_renderer.GetVulkanContext().WaitOnGraphicsQueue();
	m_renderer.GetVulkanContext().WaitOnPresentationQueue();

	DestroyDescriptorPool();
	InitialiseDescriptorPool();

	InitialiseUniformBuffers();
}

std::uint32_t GraphicsPipeline::GetVertexInputSize(const spirv_cross::SPIRType& vertexInputType)
{
	std::uint32_t size = vertexInputType.vecsize;

	switch (vertexInputType.basetype)
	{
	case spirv_cross::SPIRType::BaseType::Boolean:
		break;

	case spirv_cross::SPIRType::BaseType::Half:
		size *= 2;

		break;

	case spirv_cross::SPIRType::BaseType::Int:
	case spirv_cross::SPIRType::BaseType::UInt:
	case spirv_cross::SPIRType::BaseType::Float:
		size *= 4;

		break;

	case spirv_cross::SPIRType::BaseType::Double:
		size *= 8;

		break;

	default:
		return 0;
	}

	return size;
}

VkFormat GraphicsPipeline::GetVertexInputFormat(const spirv_cross::SPIRType& vertexInputType)
{
	switch (vertexInputType.vecsize)
	{
	case 1:
		switch (vertexInputType.basetype)
		{
		case spirv_cross::SPIRType::BaseType::Boolean:
			return VK_FORMAT_R8_UINT;

		case spirv_cross::SPIRType::BaseType::Half:
			return VK_FORMAT_R16_SFLOAT;

		case spirv_cross::SPIRType::BaseType::Int:
			return VK_FORMAT_R32_SINT;

		case spirv_cross::SPIRType::BaseType::UInt:
			return VK_FORMAT_R32_UINT;

		case spirv_cross::SPIRType::BaseType::Float:
			return VK_FORMAT_R32_SFLOAT;

		case spirv_cross::SPIRType::BaseType::Double:
			return VK_FORMAT_R64_SFLOAT;

		default:
			break;
		}

		break;

	case 2:
		switch (vertexInputType.basetype)
		{
		case spirv_cross::SPIRType::BaseType::Boolean:
			return VK_FORMAT_R8G8_UINT;

		case spirv_cross::SPIRType::BaseType::Half:
			return VK_FORMAT_R16G16_SFLOAT;

		case spirv_cross::SPIRType::BaseType::Int:
			return VK_FORMAT_R32G32_SINT;

		case spirv_cross::SPIRType::BaseType::UInt:
			return VK_FORMAT_R32G32_UINT;

		case spirv_cross::SPIRType::BaseType::Float:
			return VK_FORMAT_R32G32_SFLOAT;

		case spirv_cross::SPIRType::BaseType::Double:
			return VK_FORMAT_R64G64_SFLOAT;

		default:
			break;
		}

		break;

	case 3:
		switch (vertexInputType.basetype)
		{
		case spirv_cross::SPIRType::BaseType::Boolean:
			return VK_FORMAT_R8G8B8_UINT;

		case spirv_cross::SPIRType::BaseType::Half:
			return VK_FORMAT_R16G16B16_SFLOAT;

		case spirv_cross::SPIRType::BaseType::Int:
			return VK_FORMAT_R32G32B32_SINT;

		case spirv_cross::SPIRType::BaseType::UInt:
			return VK_FORMAT_R32G32B32_UINT;

		case spirv_cross::SPIRType::BaseType::Float:
			return VK_FORMAT_R32G32B32_SFLOAT;

		case spirv_cross::SPIRType::BaseType::Double:
			return VK_FORMAT_R64G64B64_SFLOAT;

		default:
			break;
		}

		break;

	case 4:
		switch (vertexInputType.basetype)
		{
		case spirv_cross::SPIRType::BaseType::Boolean:
			return VK_FORMAT_R8G8B8A8_UINT;

		case spirv_cross::SPIRType::BaseType::Half:
			return VK_FORMAT_R16G16B16A16_SFLOAT;

		case spirv_cross::SPIRType::BaseType::Int:
			return VK_FORMAT_R32G32B32A32_SINT;

		case spirv_cross::SPIRType::BaseType::UInt:
			return VK_FORMAT_R32G32B32A32_UINT;

		case spirv_cross::SPIRType::BaseType::Float:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case spirv_cross::SPIRType::BaseType::Double:
			return VK_FORMAT_R64G64B64A64_SFLOAT;

		default:
			break;
		}

		break;

	default:
		break;
	}

	return VK_FORMAT_UNDEFINED;
}

std::pair<VkVertexInputBindingDescription, std::vector<VkVertexInputAttributeDescription>> GraphicsPipeline::GetVertexInputData(const std::vector<std::unique_ptr<ShaderModule>>& shaderModules)
{
	std::set<VertexInputData> vertexInputs;

	for (const auto& shaderModule : shaderModules)
	{
		if (shaderModule->GetStage() == ShaderModule::Stage::Vertex)
		{
			const spirv_cross::ShaderResources shaderResources = shaderModule->GetDataReflector()->get_shader_resources();
			for (const auto& shaderInput : shaderResources.stage_inputs)
			{
				const spirv_cross::SPIRType type = shaderModule->GetDataReflector()->get_type(shaderInput.base_type_id);

				const VertexInputData currentVertexInput{
					.location = shaderModule->GetDataReflector()->get_decoration(shaderInput.id, spv::Decoration::DecorationLocation),
					.size = GetVertexInputSize(type),
					.format = GetVertexInputFormat(type)
				};

				if (currentVertexInput.format == VK_FORMAT_UNDEFINED)
				{
					throw std::runtime_error("Invalid vertex input type in Vulkan shader.");
				}

				vertexInputs.insert(currentVertexInput);
			}

			break;
		}
	}

	VkVertexInputBindingDescription vertexInputBindingDescription{ };
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBindingDescription.stride = 0;

	std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions(vertexInputs.size());
	std::size_t currentAttributeDescriptionIndex = 0;

	for (const auto vertexInput : vertexInputs)
	{
		vertexInputAttributeDescriptions[currentAttributeDescriptionIndex] = VkVertexInputAttributeDescription{ };
		vertexInputAttributeDescriptions[currentAttributeDescriptionIndex].binding = 0;
		vertexInputAttributeDescriptions[currentAttributeDescriptionIndex].location = vertexInput.location;
		vertexInputAttributeDescriptions[currentAttributeDescriptionIndex].format = vertexInput.format;
		vertexInputAttributeDescriptions[currentAttributeDescriptionIndex].offset = vertexInputBindingDescription.stride;

		vertexInputBindingDescription.stride += vertexInput.size;

		++currentAttributeDescriptionIndex;
	}

	return { vertexInputBindingDescription, vertexInputAttributeDescriptions };
}

void GraphicsPipeline::InitialiseDescriptorSetLayouts(const Config& config, const std::vector<std::unique_ptr<ShaderModule>>& shaderModules)
{
	for (const auto& shaderModule : shaderModules)
	{
		const spirv_cross::ShaderResources shaderResources = shaderModule->GetDataReflector()->get_shader_resources();

		for (const auto& uniform : shaderResources.uniform_buffers)
		{
			if (const std::uint32_t set = shaderModule->GetDataReflector()->get_decoration(uniform.id, spv::Decoration::DecorationDescriptorSet);
				set != 0)
			{
				throw std::runtime_error("Vulkan descriptor sets with an ID other than zero are not supported by this renderer.");
			}

			const std::uint32_t binding = shaderModule->GetDataReflector()->get_decoration(uniform.id, spv::Decoration::DecorationBinding);
			std::size_t size = shaderModule->GetDataReflector()->get_declared_struct_size(shaderModule->GetDataReflector()->get_type(uniform.base_type_id));
			size += s_minOffsetAlignment - (size % s_minOffsetAlignment);

			if (m_bindingsData.find(binding) == std::cend(m_bindingsData))
			{
				const DescriptorSetBindingData currentBindingData{
					.size = static_cast<std::uint32_t>(size),
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.shaderStages = static_cast<VkShaderStageFlags>(shaderModule->GetStage())
				};

				m_bindingsData[binding] = currentBindingData;
			}
			else
			{
				m_bindingsData[binding].shaderStages |= static_cast<VkShaderStageFlags>(shaderModule->GetStage());
			}
		}

		for (const auto& sampler : shaderResources.sampled_images)
		{
			if (const std::uint32_t set = shaderModule->GetDataReflector()->get_decoration(sampler.id, spv::Decoration::DecorationDescriptorSet);
				set != 0)
			{
				throw std::runtime_error("Vulkan descriptor sets with an ID other than zero are not supported by this renderer.");
			}

			const std::uint32_t binding = shaderModule->GetDataReflector()->get_decoration(sampler.id, spv::Decoration::DecorationBinding);

			if (m_bindingsData.find(binding) == std::cend(m_bindingsData))
			{
				const DescriptorSetBindingData currentBindingData{
					.size = std::nullopt,
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.shaderStages = static_cast<VkShaderStageFlags>(shaderModule->GetStage())
				};

				m_bindingsData[binding] = currentBindingData;
			}
			else
			{
				m_bindingsData[binding].shaderStages |= static_cast<VkShaderStageFlags>(shaderModule->GetStage());
			}
		}

		for (const auto& pushConstant : shaderResources.push_constant_buffers)
		{
			if (m_pushConstantRange.stageFlags == 0)
			{
				m_pushConstantRange.stageFlags = static_cast<VkShaderStageFlags>(shaderModule->GetStage());
				m_pushConstantRange.size = static_cast<std::uint32_t>(shaderModule->GetDataReflector()->get_declared_struct_size(shaderModule->GetDataReflector()->get_type(pushConstant.base_type_id)));
				m_pushConstantRange.offset = 0;

				if (m_pushConstantRange.size > s_maxPushConstantBufferSize)
				{
					throw std::runtime_error("Push constant buffer is too large in Vulkan shader.");
				}
			}
			else
			{
				m_pushConstantRange.stageFlags = static_cast<VkShaderStageFlags>(shaderModule->GetStage());
			}
		}
	}

	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings(m_bindingsData.size());
	std::size_t currentDescriptorSetLayoutBindingsIndex = 0;

	for (const auto& [binding, bindingData] : m_bindingsData)
	{
		descriptorSetLayoutBindings[currentDescriptorSetLayoutBindingsIndex] = VkDescriptorSetLayoutBinding{ };
		descriptorSetLayoutBindings[currentDescriptorSetLayoutBindingsIndex].binding = binding;
		descriptorSetLayoutBindings[currentDescriptorSetLayoutBindingsIndex].descriptorType = bindingData.type;
		descriptorSetLayoutBindings[currentDescriptorSetLayoutBindingsIndex].descriptorCount = 1;
		descriptorSetLayoutBindings[currentDescriptorSetLayoutBindingsIndex].stageFlags = bindingData.shaderStages;
		descriptorSetLayoutBindings[currentDescriptorSetLayoutBindingsIndex].pImmutableSamplers = nullptr;

		if (m_descriptorTypeCounts.find(bindingData.type) == std::cend(m_descriptorTypeCounts))
		{
			m_descriptorTypeCounts[bindingData.type] = 0;
		}

		++m_descriptorTypeCounts[bindingData.type];
		++currentDescriptorSetLayoutBindingsIndex;
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{ };
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<std::uint32_t>(descriptorSetLayoutBindings.size());
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(m_renderer.GetVulkanContext().GetLogicalDevice(), &descriptorSetLayoutCreateInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan descriptor set layout.");
	}
}

void GraphicsPipeline::DestroyDescriptorSetLayout() noexcept
{
	if (m_descriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_renderer.GetVulkanContext().GetLogicalDevice(), m_descriptorSetLayout, nullptr);
		m_descriptorSetLayout = VK_NULL_HANDLE;
	}
}

void GraphicsPipeline::InitialisePipelineLayout(const Config& config, const std::vector<std::unique_ptr<ShaderModule>>& shaderModules)
{
	InitialiseDescriptorSetLayouts(config, shaderModules);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ };
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &m_pushConstantRange;

	if (vkCreatePipelineLayout(m_renderer.GetVulkanContext().GetLogicalDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan pipeline layout.");
	}
}

void GraphicsPipeline::DestroyPipelineLayout() noexcept
{
	if (m_pipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(m_renderer.GetVulkanContext().GetLogicalDevice(), m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
	}

	DestroyDescriptorSetLayout();
}

void GraphicsPipeline::InitialisePipeline(const Config& config)
{
	std::vector<std::unique_ptr<ShaderModule>> shaderModules;
	shaderModules.reserve(config.shaderInfo.size());

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
	shaderStageCreateInfos.reserve(config.shaderInfo.size());

	for (const auto& [shaderFilepath, shaderType] : config.shaderInfo)
	{
		shaderModules.emplace_back(std::make_unique<ShaderModule>(m_renderer.GetVulkanContext().GetLogicalDevice(), shaderFilepath, shaderType));
		shaderStageCreateInfos.push_back(shaderModules.back()->GetCreateInfo());
	}

	InitialisePipelineLayout(config, shaderModules);

	const auto [vertexInputBindingDescription, vertexInputAttributeDescriptions] = GetVertexInputData(shaderModules);

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{ };
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexInputAttributeDescriptions.size());
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{ };
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{ };
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_renderer.GetSwapchainExtent().width);
	viewport.height = static_cast<float>(m_renderer.GetSwapchainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissorArea{ };
	scissorArea.offset = VkOffset2D{ 0, 0 };
	scissorArea.extent = m_renderer.GetSwapchainExtent();

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{ };
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissorArea;

	VkPipelineRasterizationStateCreateInfo rasterisationStateCreateInfo{ };
	rasterisationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterisationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterisationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterisationStateCreateInfo.polygonMode = config.drawWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterisationStateCreateInfo.lineWidth = 1.0f;
	rasterisationStateCreateInfo.cullMode = config.enableCullFace ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
	rasterisationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterisationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterisationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterisationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterisationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{ };
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{ };
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	if (config.enableDepthTest)
	{
		depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
		depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	}
	else
	{
		depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
		depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
		depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	}
	
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 0.0f;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = VkStencilOpState{ };
	depthStencilStateCreateInfo.back = VkStencilOpState{ };

	VkPipelineColorBlendAttachmentState colourBlendAttachmentState{ };
	colourBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	if (config.enableBlending)
	{		
		colourBlendAttachmentState.blendEnable = VK_TRUE;
		colourBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colourBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}
	else
	{
		colourBlendAttachmentState.blendEnable = VK_FALSE;
		colourBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	}

	colourBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colourBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo{ };
	colourBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colourBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colourBlendStateCreateInfo.attachmentCount = 1;
	colourBlendStateCreateInfo.pAttachments = &colourBlendAttachmentState;

	for (auto& blendConstant : colourBlendStateCreateInfo.blendConstants)
	{
		blendConstant = 0.0f;
	}

	constexpr std::array<VkDynamicState, 2u> DynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{ };
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<std::uint32_t>(DynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = DynamicStates.data();

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{ };
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = static_cast<std::uint32_t>(shaderStageCreateInfos.size());
	graphicsPipelineCreateInfo.pStages = shaderStageCreateInfos.data();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterisationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colourBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = m_pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = m_renderer.GetRenderPass();
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_renderer.GetVulkanContext().GetLogicalDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_pipelineHandle) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan graphics pipeline.");
	}
}

void GraphicsPipeline::DestroyPipeline() noexcept
{
	if (m_pipelineHandle != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_renderer.GetVulkanContext().GetLogicalDevice(), m_pipelineHandle, nullptr);
		m_pipelineHandle = VK_NULL_HANDLE;
	}

	DestroyPipelineLayout();
}

void GraphicsPipeline::InitialiseDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes(m_descriptorTypeCounts.size());
	std::size_t poolSizeIndex = 0;

	for (const auto [type, count] : m_descriptorTypeCounts)
	{
		poolSizes[poolSizeIndex].type = type;
		poolSizes[poolSizeIndex].descriptorCount = count * m_renderer.GetSwapchainImageCount();

		++poolSizeIndex;
	}

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{ };
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = m_renderer.GetSwapchainImageCount();

	if (vkCreateDescriptorPool(m_renderer.GetVulkanContext().GetLogicalDevice(), &descriptorPoolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan descriptor pool.");
	}
}

void GraphicsPipeline::DestroyDescriptorPool() noexcept
{
	if (m_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_renderer.GetVulkanContext().GetLogicalDevice(), m_descriptorPool, nullptr);
		m_descriptorPool = VK_NULL_HANDLE;

		m_descriptorSets.clear();
	}
}

void GraphicsPipeline::InitialiseUniformBuffers()
{
	ResizeUniformBuffers();

	for (std::size_t i = 0; i < m_renderer.GetSwapchainImageCount(); ++i)
	{
		std::size_t currentUniformBufferSize = 0;

		for (const auto& [binding, bindingData] : m_bindingsData)
		{
			if (bindingData.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				continue;
			}

			currentUniformBufferSize += bindingData.size.value();
		}

		m_uniformBuffers[i]->Initialise(currentUniformBufferSize);
	}

	InitialiseDescriptorSets();
}

void GraphicsPipeline::ResizeUniformBuffers()
{
	DestroyUniformBuffers();
	m_uniformBuffers.resize(m_renderer.GetSwapchainImageCount());
	
	for (auto& uniformBuffer : m_uniformBuffers)
	{
		uniformBuffer = std::make_unique<UniformBuffer>(m_renderer);
	}
}

void GraphicsPipeline::DestroyUniformBuffers() noexcept
{
	if (!m_uniformBuffers.empty())
	{
		m_uniformBuffers.clear();
	}
}

void GraphicsPipeline::InitialiseDescriptorSets()
{
	m_descriptorSets.resize(m_renderer.GetSwapchainImageCount());

	VkDeviceSize currentOffset = 0;
	const std::vector<VkDescriptorSetLayout> currentLayouts(m_renderer.GetSwapchainImageCount(), m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = static_cast<std::uint32_t>(m_renderer.GetSwapchainImageCount());
	descriptorSetAllocateInfo.pSetLayouts = currentLayouts.data();

	if (vkAllocateDescriptorSets(m_renderer.GetVulkanContext().GetLogicalDevice(), &descriptorSetAllocateInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vulkan descriptor sets.");
	}

	for (std::size_t i = 0; i < m_renderer.GetSwapchainImageCount(); ++i)
	{
		for (const auto& [binding, bindingData] : m_bindingsData)
		{
			VkDescriptorBufferInfo descriptorBufferInfo{ };
			VkDescriptorImageInfo descriptorImageInfo{ };

			if (bindingData.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			{
				descriptorBufferInfo.buffer = m_uniformBuffers[i]->GetHandle();
				descriptorBufferInfo.offset = currentOffset;
				descriptorBufferInfo.range = bindingData.size.value();

				currentOffset += bindingData.size.value();
			}
			else if (bindingData.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				descriptorImageInfo.imageView = VK_NULL_HANDLE;
				descriptorImageInfo.sampler = VK_NULL_HANDLE;
			}

			VkWriteDescriptorSet writeDescriptorSet{ };
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = m_descriptorSets[i];
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorType = bindingData.type;
			writeDescriptorSet.descriptorCount = 1;

			if (bindingData.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			{
				writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
				writeDescriptorSet.pImageInfo = nullptr;
				writeDescriptorSet.pTexelBufferView = nullptr;
			}
			else if (bindingData.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				writeDescriptorSet.pBufferInfo = nullptr;
				writeDescriptorSet.pImageInfo = &descriptorImageInfo;
				writeDescriptorSet.pTexelBufferView = nullptr;
			}

			vkUpdateDescriptorSets(m_renderer.GetVulkanContext().GetLogicalDevice(), 1, &writeDescriptorSet, 0, nullptr);
		}

		currentOffset = 0;
	}
}

void GraphicsPipeline::SetUniformBufferData(const std::uint32_t updatedBinding, const void* data)
{
	VkDeviceSize offset = 0;
	std::size_t size = 0;

	for (const auto& [binding, bindingData] : m_bindingsData)
	{
		if (binding == updatedBinding)
		{
			size = bindingData.size.value();

			break;
		}

		offset += bindingData.size.value();
	}

	m_uniformBuffers[m_renderer.GetNextAcquiredImageIndex()]->SetBufferData(data, size, offset);
}