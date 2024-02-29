#include "View.hpp"

View::View(Game &_game) : m_game(&_game) {
	createPipelineLayout();
	createPipeline();
	createCommandBuffers();
}

View::~View() {
	vkDestroyPipelineLayout(m_vreDevice.device(), m_pipelineLayout, nullptr);
}

void View::update() {
	drawFrame();

	// wait for all cpu/gpu operations to cease
	vkDeviceWaitIdle(m_vreDevice.m_device);
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
		m_vreDevice.m_device, pipelineConfig, "./triangle.vert.spv", "./triangle.frag.spv");
}

void View::createCommandBuffers() {
	m_commandBuffers.resize(m_vreSwapchain.imageCount());
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // there are primary and secondary command buffers
	allocInfo.commandPool = m_vreDevice.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	if (vkAllocateCommandBuffers(m_vreDevice.m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers");
	}

	//
	//
	// For now, we are doing a 1:1 relationship for commandBuffer:Renderpass
	for (int i = 0; i < m_commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vreSwapchain.getRenderPass();
		renderPassInfo.framebuffer = m_vreSwapchain.getFramebuffer(i);

		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = m_vreSwapchain.getSwapchainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		// clearValues[0].depthStencil = ?  // this value would be ignored because of how we have our vertex buffer laid out
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// VK_SUBPASS_CONTENTS_INLINE signals that the subsequent render pass commands will be directly embedded in the 
		// primary command buffer itself, and that no secondary cmd buffers will be used
		// VK_SUBPASS_CONTENTS_SECONDARY means that render pass command will be exeucted by secondary command buffer, no render pass can use inline/secondary command buffers
		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		m_vrePipeline->bind(m_commandBuffers[i]);
		vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_commandBuffers[i]);

		if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}
	}

}

void View::drawFrame() {
	uint32_t imageIndex;
	// fetches the index of the frame we should render to next
	// also handles cpu/gpu sync for double/triple buff
	auto result = m_vreSwapchain.acquireNextImage(&imageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swapchain image");
	}

	// submit command buffer to device graphics queue while handling cpu/gpu sync
	result = m_vreSwapchain.submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image");
	}
}
