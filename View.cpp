#define VMA_IMPLEMENTATION
#include "View.hpp"

View::View(Game &_game) : m_game(&_game) {
	m_stopRendering = false;
	m_resizeRequested = false;

	m_device = 0;
	m_instance = 0;
	m_surface = 0;
}

View::~View() {
	cleanup();
}

void View::drawRays() {
	int r;
	int mapx;
	int mapy;
	int mapArrayPos;
	int depthOfField;
	float px = m_game->m_px;
	float py = m_game->m_py;

	float rayFirstHorizontal_x;
	float rayFirstVertical_y; 
	float rayAngle; 
	float rayOffset_x; 
	float rayOffset_y;

	rayAngle = m_game->m_pa; // set ray angle to player's angle
	for (r = 0; r < 1; r++) {
		depthOfField = 0;
		// check horizontal lines
		float aTan = -1 / glm::tan(rayAngle);
		if (rayAngle > PI) {
			// if the ray angle is > 180 degrees, looking down
			// round to the nearest 64th value
			rayFirstVertical_y = (((int)py >> 6) << 6) - 0.0001;
			rayFirstHorizontal_x = (py - rayFirstVertical_y) * aTan + px;
			rayOffset_y = -64;
			rayOffset_x = -rayOffset_y * aTan;
		}

		if (rayAngle < PI) {
			// round to the nearest 64th value
			rayFirstVertical_y = (((int)py >> 6) << 6) + 64;
			rayFirstHorizontal_x = (py - rayFirstVertical_y) * aTan + px;
			rayOffset_y = 64;
			rayOffset_x = -rayOffset_y * aTan;
		}

		if (rayAngle == 0 || rayAngle == PI) {
			rayFirstHorizontal_x = px;
			rayFirstVertical_y = py;
			depthOfField = 8;
		}

		while (depthOfField < 8) {
			mapx = (int)(rayFirstHorizontal_x) >> 6;
			mapy = (int)(rayFirstVertical_y) >> 6;
			mapArrayPos = mapy * MAP_WIDTH + mapx;

			if (mapArrayPos < MAP_WIDTH * MAP_HEIGHT // in bounds
				&& m_map[mapArrayPos] == 1 // hit a wall
			) {
				depthOfField = 8; 
			} else {
				rayFirstHorizontal_x += rayOffset_x;
				rayFirstVertical_y += rayOffset_y;
			}
		}
	}
}

void View::draw() {
	//check if window is minimized and skip drawing
	if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
		return;

	// wait until the gpu has finished rendering the last frame. 
	// Timeout of 1 second
	VK_CHECK(vkWaitForFences(m_device, 1, &getCurrentFrame().renderFence, 
		true, 1000000000));

	VK_CHECK(vkWaitForFences(m_device, 1, &getCurrentFrame().renderFence,
		true, 1000000000));

	// swap this line out for the deletion queue
	VK_CHECK(vkResetFences(m_device, 1, &getCurrentFrame().renderFence));
	getCurrentFrame().deletionQueue.flush();

	//request image from the swapchain
	uint32_t swapchainImageIndex;
	VkResult e = vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000,
		getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (e == VK_ERROR_OUT_OF_DATE_KHR) {
		m_resizeRequested = true;
		return;
	}
	//VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, 
	//	getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

	//naming it cmd for shorter writing
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

	// now that we are sure that the commands finished executing, we can safely
	// reset the command buffer to begin recording again.
 	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo 
		= vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_drawExtent.width = std::min(m_swapchainExtent.width, m_drawImage.imageExtent.width) * m_renderScale;
	m_drawExtent.height = std::min(m_swapchainExtent.height, m_drawImage.imageExtent.height) * m_renderScale;

	//start the command buffer recording
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// transition our main draw image into general layout so we can write into it
	// we will overwrite it all so we dont care about what was the older layout
	vkutil::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL);

	drawBackground();

	vkutil::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	drawGeometry(cmd);

	// transition the draw image and the swapchain image into their correst transfer layout
	vkutil::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transitionImage(cmd, m_swapchainImages[swapchainImageIndex], 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vkutil::copyImageToImage(cmd, m_drawImage.image, 
		m_swapchainImages[swapchainImageIndex], m_drawExtent, m_swapchainExtent);

	// set swapchain image layout to Present so we can show it on the screen
	vkutil::transitionImage(cmd, m_swapchainImages[swapchainImageIndex], 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	drawImgui(cmd, m_swapchainImageViews[swapchainImageIndex]);

	// set swapchain image layout to Present so we can draw it
	vkutil::transitionImage(cmd, m_swapchainImages[swapchainImageIndex], 
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	//prepare the submission to the queue. 
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmdinfo = vkinit::commandBufferSubmitInfo(cmd);

	VkSemaphoreSubmitInfo waitInfo = vkinit::semaphoreSubmitInfo(
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, 
		getCurrentFrame().swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkinit::semaphoreSubmitInfo(
		VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

	VkSubmitInfo2 submit = vkinit::submitInfo2(&cmdinfo, &signalInfo, &waitInfo);

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, getCurrentFrame().renderFence));

	//prepare present
	// this will put the image we just rendered to into the visible window.
	// we want to wait on the _renderSemaphore for that, 
	// as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	//VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));
	VkResult presentResult = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
		m_resizeRequested = true;
	}

	//increase the number of frames drawn
	m_frameNumber++;
}

void View::drawGeometry(VkCommandBuffer _cmd) {
	//begin a render pass  connected to our draw image
	VkRenderingAttachmentInfo colorAttachment 
		= vkinit::attachmentInfo(m_drawImage.imageView, 
			nullptr, VK_IMAGE_LAYOUT_GENERAL);

	VkRenderingInfo renderInfo 
		= vkinit::renderingInfo(m_drawExtent, &colorAttachment, nullptr);
	vkCmdBeginRendering(_cmd, &renderInfo);

	vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_trianglePipeline);

	//set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = m_drawExtent.width;
	viewport.height = m_drawExtent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(_cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = m_drawExtent.width;
	scissor.extent.height = m_drawExtent.height;

	vkCmdSetScissor(_cmd, 0, 1, &scissor);

	//launch a draw command to draw 3 vertices
	vkCmdDraw(_cmd, 3, 1, 0, 0);
	//vkCmdEndRendering(_cmd);

	vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipeline);

	GPUDrawPushConstants push_constants;
	push_constants.worldMatrix = glm::mat4{ 1.f };
	push_constants.vertexBuffer = m_rectangle.vertexBufferAddress;

	vkCmdPushConstants(_cmd, m_meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 
		0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(_cmd, m_rectangle.indexBuffer.buffer, 0, 
		VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(_cmd, 6, 1, 0, 0, 0);

	vkCmdEndRendering(_cmd);
}


void View::drawBackground() {
	//make a clear-color from frame number. This will flash with a 120 frame period.
	VkClearColorValue clearValue;
	float flash = abs(sin(m_frameNumber / 120.f));
	clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

	VkImageSubresourceRange clearRange = vkinit::imageSubresourceRange(
		VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
	//vkCmdClearColorImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	ComputeEffect &effect = m_backgroundEffects[m_currentBackgroundEffect];
	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, 
		m_gradientPipelineLayout, 0, 1, &m_drawImageDescriptors, 0, nullptr);

	ComputePushConstants pc;
	pc.data1 = glm::vec4(1, 0, 0, 1);
	pc.data2 = glm::vec4(0, 0, 0, 1);

	vkCmdPushConstants(cmd, m_gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 
		0, sizeof(ComputePushConstants), &effect.data);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(m_drawExtent.width / 16.0), 
		std::ceil(m_drawExtent.height / 16.0), 1);
}

void View::drawImgui(VkCommandBuffer _cmd, VkImageView _targetImageView) {
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachmentInfo(
		_targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingInfo renderInfo = vkinit::renderingInfo(m_swapchainExtent, 
		&colorAttachment, nullptr);

	vkCmdBeginRendering(_cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), _cmd);

	vkCmdEndRendering(_cmd);

	//ImGui::UpdatePlatformWindows();
	//ImGui::RenderPlatformWindowsDefault();
}

void View::initialize() {
	initPipelines();
	//initImgui();

	initDefaultData();

	m_initialized = true;
}

void View::createSwapchain(uint32_t _width, uint32_t _height) {
	vkb::SwapchainBuilder swapchainBuilder{ m_gpu, m_device, m_surface };

	m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{
			.format = m_swapchainImageFormat,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		// use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(_width, _height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	m_swapchainExtent = vkbSwapchain.extent;
	// store swapchain and its related images
	m_swapchain = vkbSwapchain.swapchain;
	m_swapchainImages = vkbSwapchain.get_images().value();
	m_swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void View::resizeSwapchain() {
	vkDeviceWaitIdle(m_device);
	destroySwapchain();

	int w = 0;
	int h = 0;
	SDL_GetWindowSize(m_window, &w, &h);
	m_windowExtent.width = w;
	m_windowExtent.height = h;

	createSwapchain(m_windowExtent.width, m_windowExtent.height);
	m_resizeRequested = false;
}

void View::destroySwapchain() {
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < m_swapchainImageViews.size(); i++) {

		vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
	}
}


void View::initPipelines() {
	initBackgroundPipelines();
	initTriangle();
	initMeshPipeline();
}

void View::initBackgroundPipelines() {
	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &m_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayout.pPushConstantRanges = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;



	VK_CHECK(vkCreatePipelineLayout(m_device, &computeLayout, 
		nullptr, &m_gradientPipelineLayout));

	//layout code
	VkShaderModule gradientShader;
	if (!vkutil::loadShaderModule("./floor_ceiling.comp.spv", m_device, &gradientShader)) {
		std::cout << "Error when building the compute shader \n";
	}

	VkShaderModule skyShader;
	if (!vkutil::loadShaderModule("./sky.spv", m_device, &skyShader)) {
		std::cout << "Error when building the compute shader \n";
	}

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = gradientShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = m_gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;

	ComputeEffect gradient;
	gradient.layout = m_gradientPipelineLayout;
	gradient.name = "gradient_color";
	gradient.data = {};

	//default colors
	gradient.data.data1 = glm::vec4(1, 0, 0, 1);
	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, 
		&computePipelineCreateInfo, nullptr, &gradient.pipeline));

	//change the shader module only to create the sky shader
	computePipelineCreateInfo.stage.module = skyShader;

	ComputeEffect sky;
	sky.layout = m_gradientPipelineLayout;
	sky.name = "sky";
	sky.data = {};
	//default sky parameters
	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

	VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1,
		&computePipelineCreateInfo, nullptr, &sky.pipeline));

	//add the 2 background effects into the array
	m_backgroundEffects.push_back(gradient);
	m_backgroundEffects.push_back(sky);

	vkDestroyShaderModule(m_device, gradientShader, nullptr);
	vkDestroyShaderModule(m_device, skyShader, nullptr);

	m_deletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(m_device, m_gradientPipelineLayout, nullptr);
		//vkDestroyPipeline(m_device, m_gradientPipeline, nullptr);
		vkDestroyPipeline(m_device, sky.pipeline, nullptr);
		vkDestroyPipeline(m_device, gradient.pipeline, nullptr);
		});
}

void View::immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&_function) {
	VK_CHECK(vkResetFences(m_device, 1, &m_immFence));
	VK_CHECK(vkResetCommandBuffer(m_immCommandBuffer, 0));

	VkCommandBuffer cmd = m_immCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	_function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkinit::commandBufferSubmitInfo(cmd);
	VkSubmitInfo2 submit = vkinit::submitInfo2(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, m_immFence));

	VK_CHECK(vkWaitForFences(m_device, 1, &m_immFence, true, 9999999999));
}

void View::initImgui() {

}

AllocatedBuffer View::createBuffer(size_t _allocSize, 
	VkBufferUsageFlags _usage, VmaMemoryUsage _memoryUsage
) {
	// allocate buffer
	VkBufferCreateInfo bufferInfo = { 
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext = nullptr;
	bufferInfo.size = _allocSize;

	bufferInfo.usage = _usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = _memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &vmaallocInfo,
		&newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

	return newBuffer;
}

GPUMeshBuffers View::uploadMesh(std::span<uint32_t> _indices, std::span<Vertex> _vertices) {
	const size_t vertexBufferSize = _vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = _indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface;

	//create vertex buffer
	newSurface.vertexBuffer = createBuffer(vertexBufferSize, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
		| VK_BUFFER_USAGE_TRANSFER_DST_BIT 
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	//find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAdressInfo{ 
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = newSurface.vertexBuffer.buffer };
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(m_device, 
		&deviceAdressInfo);

	//create index buffer
	newSurface.indexBuffer = createBuffer(indexBufferSize, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
		| VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	AllocatedBuffer staging 
		= createBuffer(vertexBufferSize + indexBufferSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VMA_MEMORY_USAGE_CPU_ONLY);

	void *data = staging.allocation->GetMappedData();

	// copy vertex buffer
	memcpy(data, _vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char *)data + vertexBufferSize, _indices.data(), indexBufferSize);

	immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
		});

	destroyBuffer(staging);
	return newSurface;
}

void View::cleanup() {
	if (m_initialized) {

		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(m_device);
		m_deletionQueue.flush();

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			vkDestroyCommandPool(m_device, m_frames[i].commandPool, nullptr);
			//destroy sync objects
			vkDestroyFence(m_device, m_frames[i].renderFence, nullptr);
			vkDestroySemaphore(m_device, m_frames[i].renderSemaphore, nullptr);
			vkDestroySemaphore(m_device, m_frames[i].swapchainSemaphore, nullptr);
		}

		destroySwapchain();

		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyDevice(m_device, nullptr);

		vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
		vkDestroyInstance(m_instance, nullptr);
		SDL_DestroyWindow(m_window);
	}
}

void View::newFrame() {
	// imgui new frame
	ImGui_ImplVulkan_NewFrame();    
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	//some imgui UI to test
	if (ImGui::Begin("background")) {

		ComputeEffect &selected = m_backgroundEffects[m_currentBackgroundEffect];

		ImGui::SliderFloat("Render Scale", &m_renderScale, 0.3f, 1.0f);

		ImGui::Text("Selected effect: ", selected.name);

		ImGui::SliderInt("Effect Index", &m_currentBackgroundEffect, 0, 
			m_backgroundEffects.size() - 1);

		ImGui::InputFloat4("data1", (float *)&selected.data.data1);
		ImGui::InputFloat4("data2", (float *)&selected.data.data2);
		ImGui::InputFloat4("data3", (float *)&selected.data.data3);
		ImGui::InputFloat4("data4", (float *)&selected.data.data4);

		ImGui::End();
	}

	ImGui::Render();
	//make imgui calculate internal draw structures
}
