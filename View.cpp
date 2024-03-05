#include "View.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // forces depth to [0,1] instead of [-1,1]
#include <glm/glm.hpp>

namespace vre {
	struct SimplePushConstantData {
		glm::vec2 offset;
		alignas(16) glm::vec3 color;
	};
}


View::View(Game &_game) : m_game(&_game) {
	loadModel();
	createPipelineLayout();
	recreateSwapchain();
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
	VkPushConstantRange pushConstantRange{};
	// this signal that we want access to the push constant data in both
	// the vertex and fragment shaders
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
		| VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0; // if we are using diff ranges for v/f shaders
	pushConstantRange.size = sizeof(vre::SimplePushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	// push constants are an effecient way of sending
	// a very small amount of data to our shader program
	pipelineLayoutInfo.pushConstantRangeCount = 1; // simple push constant
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(m_vreDevice.device(), &pipelineLayoutInfo,
		nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void View::createPipeline() {
	vre::PipelineConfigInfo pipelineConfig{};
	vre::VrePipeline::defaultPipelineConfigInfo(pipelineConfig);
	//auto pipelineConfig = vre::VrePipeline::defaultPipelineConfigInfo(
	//	m_vreSwapchain->width(), m_vreSwapchain->height());
	pipelineConfig.renderPass = m_vreSwapchain->getRenderPass();
	pipelineConfig.pipelineLayout = m_pipelineLayout;
	
	m_vrePipeline = std::make_unique<vre::VrePipeline>(
		m_vreDevice.m_device, pipelineConfig, "./triangle.vert.spv", "./triangle.frag.spv");
}

void View::loadModel() {
	std::vector<vre::VreModel::Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};
	m_model = std::make_unique<vre::VreModel>(m_vreDevice, vertices);
}

void View::createCommandBuffers() {
	m_commandBuffers.resize(m_vreSwapchain->imageCount());
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // there are primary and secondary command buffers
	allocInfo.commandPool = m_vreDevice.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	if (vkAllocateCommandBuffers(m_vreDevice.m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers");
	}

}

void View::drawFrame() {
	uint32_t imageIndex;
	// fetches the index of the frame we should render to next
	// also handles cpu/gpu sync for double/triple buff
	auto result = m_vreSwapchain->acquireNextImage(&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swapchain image");
	}

	// submit command buffer to device graphics queue while handling cpu/gpu sync
	recordCommandBuffer(imageIndex);
	result = m_vreSwapchain->submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR
		|| m_vreWindow.wasWindowResized()) {
		m_vreWindow.resetWindowResizedFlag();
		recreateSwapchain();
		return;
	}
	
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image");
	}
}

void View::freeCommandBuffers() {
	vkFreeCommandBuffers(m_vreDevice.m_device, m_vreDevice.getCommandPool(),
		static_cast<float>(m_commandBuffers.size()), m_commandBuffers.data());
	m_commandBuffers.clear();
}

void View::recreateSwapchain() {
	auto extent = m_vreWindow.getExtent();
	int w = 0;
	int h = 0;
	SDL_GetWindowSize(m_vreWindow.m_window, &w, &h);
	extent.width = w;
	extent.height = h;

	vkDeviceWaitIdle(m_vreDevice.m_device);

	if (m_vreSwapchain == nullptr) {
		m_vreSwapchain = std::make_unique<vre::VreSwapchain>(m_vreDevice, extent);
	} else {
		m_vreSwapchain = std::make_unique<vre::VreSwapchain>
			(m_vreDevice, extent, std::move(m_vreSwapchain));
		if (m_vreSwapchain->imageCount() != m_commandBuffers.size()) {
			freeCommandBuffers();
			createCommandBuffers();
		}
	}

	// if renderpass compatible do nothing else
	createPipeline();

}

void View::recordCommandBuffer(int _imageIndex) {
	static int frame = 0;
	frame = (frame + 1) % 1000;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(m_commandBuffers[_imageIndex], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_vreSwapchain->getRenderPass();
	renderPassInfo.framebuffer = m_vreSwapchain->getFramebuffer(_imageIndex);

	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = m_vreSwapchain->getSwapchainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
	// clearValues[0].depthStencil = ?  // this value would be ignored because of how we have our vertex buffer laid out
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	// VK_SUBPASS_CONTENTS_INLINE signals that the subsequent render pass commands will be directly embedded in the 
	// primary command buffer itself, and that no secondary cmd buffers will be used
	// VK_SUBPASS_CONTENTS_SECONDARY means that render pass command will be exeucted by secondary command buffer, no render pass can use inline/secondary command buffers
	vkCmdBeginRenderPass(m_commandBuffers[_imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_vreSwapchain->getSwapchainExtent().width);
	viewport.height = static_cast<float>(m_vreSwapchain->getSwapchainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, m_vreSwapchain->getSwapchainExtent() };
	vkCmdSetViewport(m_commandBuffers[_imageIndex], 0, 1, &viewport);
	vkCmdSetScissor(m_commandBuffers[_imageIndex], 0, 1, &scissor);

	m_vrePipeline->bind(m_commandBuffers[_imageIndex]);
	//vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);
	m_model->bind(m_commandBuffers[_imageIndex]);

	// Playing with push constants
	for (int j = 0; j < 4; j++) {
		vre::SimplePushConstantData push{};
		push.offset = { -0.5f + frame * 0.002f, -0.4f + j * 0.25f };
		push.color = { 0.0f, 0.0f, 0.2f + 0.2f * j };

		vkCmdPushConstants(m_commandBuffers[_imageIndex], m_pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(vre::SimplePushConstantData), &push);
		m_model->draw(m_commandBuffers[_imageIndex]);
		// don't forget to update shader files to expect push constants!
	}

	//m_model->draw(m_commandBuffers[_imageIndex]);

	vkCmdEndRenderPass(m_commandBuffers[_imageIndex]);

	if (vkEndCommandBuffer(m_commandBuffers[_imageIndex]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer");
	}
}
