#pragma once

#include <vulkan/vulkan.h>

#include <iostream>
#include <fstream>
#include <vector>

namespace shader {
	static std::vector<char> readFile(std::string _filename) {
		std::ifstream file(_filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			std::cout << "failed to load " << _filename << std::endl;
		}

		// tellg() queries location of pointer
		size_t filesize{ static_cast<size_t>(file.tellg()) };

		std::vector<char> buffer(filesize);
		file.seekg(0);
		file.read(buffer.data(), filesize);

		file.close();
		return buffer;
	}

	static VkShaderModule createModule(
		std::string _filename,
		VkDevice _logicalDevice
	) {
		std::vector<char> sourceCode = readFile(_filename);

		VkShaderModuleCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pNext = nullptr;
		//info.flags = vk::ShaderModuleCreateFlags(); // sets default values like sType, pNext
		info.codeSize = sourceCode.size();
		info.pCode = reinterpret_cast<const uint32_t*>(sourceCode.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(_logicalDevice, &info, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Couldn't create shader module " + _filename + "\n");
		}

		return shaderModule;
	}
}