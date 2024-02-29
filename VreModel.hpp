#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // forces depth to [0,1] instead of [-1,1]
#include <glm/glm.hpp>

#include <vector>
#include <cassert>
#include <cstring>

#include "VreDevice.hpp"

namespace vre {
	class VreModel {
	public:
		struct Vertex {
			glm::vec2 position;
			glm::vec3 color; 
			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};


		VreModel(vre::VreDevice &_device, const std::vector<Vertex> &_vertices);
		~VreModel();

		void bind(VkCommandBuffer _commandBuffer);
		void draw(VkCommandBuffer _commandBuffer);

		void createVertexBuffers(const std::vector<Vertex> &_vertices);

		vre::VreDevice &m_vreDevice;
		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;
		uint32_t m_vertexCount;
	};
}