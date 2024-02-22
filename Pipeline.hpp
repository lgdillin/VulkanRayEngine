#pragma once

#include <iostream>
#include <vector>
#include <fstream>

#include <vulkan/vulkan.h>
#include "VreDevice.hpp"

namespace vre {
	struct PipelineConfigInfo {

	};

	class Pipeline {
	public:
		Pipeline(vre::VreDevice &_device, const PipelineConfigInfo &_info, const std::string &_vertexFile,
			const std::string &_fragFile);
		~Pipeline();

		static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t _width, uint32_t _height);

		void createGraphicsPipeline(const std::string &_vertexFile,
			const std::string &_fragFile, const PipelineConfigInfo &_info);

		void createShaderModule(const std::vector<char> &_code, 
			VkShaderModule *_shaderModule);

		static std::vector<char> readFile(const std::string &_file);

		VreDevice &m_device;
		VkPipeline m_graphicsPipeline;
		VkShaderModule m_vShader;
		VkShaderModule m_fShader;
	};
}