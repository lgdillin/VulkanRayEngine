#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Structs.hpp"
#include "PipelineBuilder.hpp"

class Dot {
public:

	Dot() {}
	Dot(
		VkDevice &_device, 
		VkPhysicalDevice &_gpu,
		const std::vector<Vertex2> &_vertices, 
		int _x, 
		int _y
	) {
		m_gpu = _gpu;
		m_device = _device;
		m_xpos = _x;
		m_ypos = _y;
		createVertexBuffers(_vertices);

	}
	~Dot() {
		// vkDestroyBuffer(m_device, m_vb, nullptr);
		// vkFreeMemory(m_device, m_vbm, nullptr);
	}

	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkBuffer m_vb;
	VkDeviceMemory m_vbm;
	uint32_t m_vertexCount;

	int m_xpos;
	int m_ypos;

	void bind(VkCommandBuffer _cmd) {
		VkBuffer buffers[] = { m_vb };
		VkDeviceSize offsets[] = { 0 };
		// so this function will record to our command buffer to bind one
		// vertex buffer starting at binding 0, with an offset 0 into the buffer
		// this makes it easier to add extra bindings in the future
		vkCmdBindVertexBuffers(_cmd, 0, 1, buffers, offsets);
	}

	void draw(VkCommandBuffer _cmd) {
		vkCmdDraw(_cmd, m_vertexCount, 1, 0, 0);
	}

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	void createPipeline() {
		VkShaderModule vertexShader, fragmentShader;
		vkutil::loadShaderModule("./dot_shader.vert.spv", m_device, &vertexShader);
		vkutil::loadShaderModule("./dot_shader.frag.spv", m_device, &fragmentShader);

		VkPipelineLayoutCreateInfo info = vkinit::pipelineLayoutCreateInfo();
		VK_CHECK(vkCreatePipelineLayout(m_device, &info,
			nullptr, &m_pipelineLayout));

		PipelineBuilder pb;
		pb.m_pipelineLayout = m_pipelineLayout;
		pb.setShaders(vertexShader, fragmentShader);
		pb.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pb.setPolygonMode(VK_POLYGON_MODE_FILL);
		pb.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
		pb.setMultisamplingNone();
		pb.disableBlending();
		pb.disableDepthtest();
		pb.setColorAttachmentFormat(VK_FORMAT_R32G32_SFLOAT);
		pb.setDepthFormat(VK_FORMAT_UNDEFINED);
		m_pipeline = pb.buildPipeline(m_device);
	}

private:
	void createVertexBuffers(const std::vector<Vertex2> &_vertices) {
		m_vertexCount = static_cast<uint32_t>(_vertices.size());
		assert(m_vertexCount >= 3 && "Vertex count must be at least 3");

		VkDeviceSize bufferSize = sizeof(_vertices[0]) * m_vertexCount;
		vkinit::createBuffer(m_device, m_gpu, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_vb, m_vbm);

		void *data;
		// this function creates a region of host memory (cpu), mapped to device memory
		// (gpu) and sets *data to the beginning of mapped memory range
		vkMapMemory(m_device, m_vbm, 0, bufferSize, 0, &data);
		// memcpy copies the vertex data into the host mapped memory region (*data)
		// because we have the host coherent bit, the host memory will auotmatically
		// be flushed to update the device memory. if this bit is absent, we would be
		// required to called vkflushmappedmemoryrange to propagate
		memcpy(data, _vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(m_device, m_vbm);
	}
};