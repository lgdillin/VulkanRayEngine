#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <fstream>
#include <cassert>

#include <vulkan/vulkan.h>
#include "VreDevice.hpp"

#include "VkFuncs.hpp"

namespace vre {
	struct PipelineConfigInfo {
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkViewport viewport;
		VkRect2D scissor;
		//VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class VrePipeline {
	public:
		VrePipeline(VkDevice &_device, const std::string _vertexFile,
			const std::string _fragFile);
		~VrePipeline();

		//PipelineConfigInfo configureDefaultPipelineInfo(uint32_t _width, uint32_t _height);
		static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t _width, uint32_t _height);

		void createGraphicsPipeline(const std::string &_vertexFile,
			const std::string &_fragFile, const PipelineConfigInfo &_info);

		bool createShaderModule(
			std::string _filePath,
			VkShaderModule *_outShaderModule);

		static std::vector<char> readFile(const std::string &_file);

		VkDevice m_device;
		VkPipeline m_graphicsPipeline;
		VkShaderModule m_vShader;
		VkShaderModule m_fShader;
	};
}