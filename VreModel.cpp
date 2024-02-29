#include "VreModel.hpp"

vre::VreModel::VreModel(vre::VreDevice &_device, const std::vector<Vertex> &_vertices
) : m_vreDevice{ _device } {
	createVertexBuffers(_vertices);
}

vre::VreModel::~VreModel() {
	vkDestroyBuffer(m_vreDevice.m_device, m_vertexBuffer, nullptr);
	vkFreeMemory(m_vreDevice.m_device, m_vertexBufferMemory, nullptr);
}

void vre::VreModel::bind(VkCommandBuffer _commandBuffer) {
	VkBuffer buffers[] = { m_vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(_commandBuffer, 0, 1, buffers, offsets);
}

void vre::VreModel::draw(VkCommandBuffer _commandBuffer) {
	vkCmdDraw(_commandBuffer, m_vertexCount, 1, 0, 0);

}

void vre::VreModel::createVertexBuffers(const std::vector<Vertex> &_vertices) {
	m_vertexCount = static_cast<uint32_t>(_vertices.size());
	assert(m_vertexCount >= 3 && "Vertex count must be at least 3");

	VkDeviceSize bufferSize = sizeof(_vertices[0]) * m_vertexCount;

	// Host = CPU
	// Device = GPU
	m_vreDevice.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_vertexBuffer,
		m_vertexBufferMemory);

	void *data;
	vkMapMemory(m_vreDevice.m_device, m_vertexBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(m_vreDevice.m_device, m_vertexBufferMemory);
}

std::vector<VkVertexInputBindingDescription> vre::VreModel::Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> vre::VreModel::Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);
	return attributeDescriptions;

}
