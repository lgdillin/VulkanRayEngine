#pragma once

#include <vulkan/vulkan.h>

#include "Shader.hpp"

class Triangle {
public:
	Triangle(VkDevice _logicalDevice) : m_logicalDevice(&_logicalDevice) {
		m_vShader = shader::createModule("./shader1v.spv", *m_logicalDevice);
		m_fShader = shader::createModule("./shader1f.spv", *m_logicalDevice);
	}

	~Triangle() {}

	VkShaderModule m_vShader;
	VkShaderModule m_fShader;
private:
	VkDevice *m_logicalDevice;
};