#include "View.hpp"

View::View(Game &_game) : m_game(&_game) {

}

View::~View() {
}

void View::update() {

}

void View::createPipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	// push constants are an effecient way of sending
	// a very small amount of data to our shader program
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_vreDevice.device(), &pipelineLayoutInfo,
		nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void View::createPipeline() {
	auto pipelineConfig = vre::VrePipeline::defaultPipelineConfigInfo(
		m_vreSwapchain.width(), m_vreSwapchain.height());
	pipelineConfig.renderPass = m_vreSwapchain.getRenderPass();
	pipelineConfig.pipelineLayout = m_pipelineLayout;
	m_vrePipeline = std::make_unique<vre::VrePipeline>(
		m_vreDevice, "./triangle.vert.spv", "./triangle.frag.spv");
}
