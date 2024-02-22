#pragma once

#include <iostream>
#include <vector>
#include <fstream>

#include <vulkan/vulkan.h>

namespace vre {
	struct PipelineConfigInfo {

	};

	class Pipeline {
	public:
		Pipeline(VkDevice &_device) : m_device(_device) { }
		Pipeline(VkDevice &_device, const PipelineConfigInfo &_info);

		void createGraphicsPipeline(const PipelineConfigInfo &_info);

		static std::vector<char> readFile(const std::string &_file);

		VkDevice &m_device;
		VkPipeline m_graphicsPipeline;
		VkShaderModule m_vShader;
		VkShaderModule m_fShader;
	};
}