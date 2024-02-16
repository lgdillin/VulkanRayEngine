#define VMA_IMPLEMENTATION
#include "View.hpp"

View::View(Game &_game) : m_game(&_game) {
	m_stopRendering = false;
}

View::~View() {
	cleanup();
}

void View::draw() {
	//check if window is minimized and skip drawing
	if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
		return;

	// wait until the gpu has finished rendering the last frame. 
	// Timeout of 1 second
	VK_CHECK(vkWaitForFences(m_device, 1, &getCurrentFrame().renderFence, 
		true, 1000000000));

	// swap this line out for the deletion queue
	VK_CHECK(vkResetFences(m_device, 1, &getCurrentFrame().renderFence));
	getCurrentFrame().deletionQueue.flush();

	//request image from the swapchain
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, 
		getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

	//naming it cmd for shorter writing
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

	// now that we are sure that the commands finished executing, we can safely
	// reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo 
		= vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_drawExtent.width = m_drawImage.imageExtent.width;
	m_drawExtent.height = m_drawImage.imageExtent.height;

	//start the command buffer recording
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// transition our main draw image into general layout so we can write into it
	// we will overwrite it all so we dont care about what was the older layout
	vkutil::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL);

	drawBackground();

	// transition the draw image and the swapchain image into their correst transfer layout
	vkutil::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transitionImage(cmd, m_swapchainImages[swapchainImageIndex], 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vkutil::copyImageToImage(cmd, m_drawImage.image, 
		m_swapchainImages[swapchainImageIndex], m_drawExtent, m_swapchainExtent);

	// set swapchain image layout to Present so we can show it on the screen
	vkutil::transitionImage(cmd, m_swapchainImages[swapchainImageIndex], 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

	VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));

	//increase the number of frames drawn
	m_frameNumber++;
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

	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradientPipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, 
		m_gradientPipelineLayout, 0, 1, &m_drawImageDescriptors, 0, nullptr);

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
}

void View::initialize() {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

	m_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		m_windowExtent.width,
		m_windowExtent.height,
		window_flags
	);

	initVulkan();
	initSwapchain();
	initCommands();
	initSyncStructures();
	initDescriptors();
	initPipelines();
	initImgui();

	m_initialized = true;
}

void View::initVulkan() {
	int major = VULKAN_VERSION[0];
	int minor = VULKAN_VERSION[1];
	vkb::InstanceBuilder builder{};

	// make vulkan instance w/ basic debug features
	auto inst_ret = builder.set_app_name("SDL2/Vulkan")
		.request_validation_layers(ENABLE_VALIDATION_LAYERS)
		.use_default_debug_messenger()
		.require_api_version(major, minor, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//grab the instance
	m_instance = vkb_inst.instance;
	m_debugMessenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);

	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features{};
	// dynamic rendering allows us to completely skip renderpasses/framebuffers
	features.dynamicRendering = minor == 3 ? true : false;
	// upgraded version of previous synchonrization functions
	features.synchronization2 = minor == 3 ? true : false;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{};
	// bda will let us use GPU pointers without binding buffers
	features12.bufferDeviceAddress = true;
	// gives us bindless textures
	features12.descriptorIndexing = true;

	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(major, minor)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(m_surface)
		.select()
		.value();


	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	m_device = vkbDevice.device;
	m_gpu = physicalDevice.physical_device;

	// use vkbootstrap to get a Graphics queue
	m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = m_gpu;
	allocatorInfo.device = m_device;
	allocatorInfo.instance = m_instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &m_allocator);

	m_deletionQueue.push_function([&]() {
		vmaDestroyAllocator(m_allocator);
		});
}

void View::initSwapchain() {
	createSwapchain(m_windowExtent.width, m_windowExtent.height);

	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		m_windowExtent.width,
		m_windowExtent.height,
		1
	};

	//hardcoding the draw format to 32 bit float
	m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	m_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkinit::imageCreateInfo(m_drawImage.imageFormat, 
		drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(m_allocator, &rimg_info, &rimg_allocinfo, 
		&m_drawImage.image, &m_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageviewCreateInfo(
		m_drawImage.imageFormat, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(m_device, &rview_info, nullptr, &m_drawImage.imageView));

	//add to deletion queues
	m_deletionQueue.push_function([=]() {
		vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
		vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.allocation);
		});
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

void View::destroySwapchain() {
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < m_swapchainImageViews.size(); i++) {

		vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
	}
}

void View::initCommands() {
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(
		m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr,
		&m_immCommandPool));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(
		m_immCommandPool, 1);

	m_deletionQueue.push_function([=]() {
		vkDestroyCommandPool(m_device, m_immCommandPool, nullptr);
	});

	//for (int i = 0; i < FRAME_OVERLAP; i++) {

	//	VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i].commandPool));

	//	// allocate the default command buffer that we will use for rendering
	//	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(
	//		m_frames[i].commandPool, 1);

	//	VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i].commandBuffer));
	//}
}

void View::initSyncStructures() {
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo(0);

	//for (int i = 0; i < FRAME_OVERLAP; i++) {
	//	VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, 
	//		&m_frames[i].renderFence));

	//	VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, 
	//		&m_frames[i].swapchainSemaphore));
	//	VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, 
	//		&m_frames[i].renderSemaphore));
	//}

	VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immFence));
	m_deletionQueue.push_function([=]() {
		vkDestroyFence(m_device, m_immFence, nullptr);
		});
}

void View::initDescriptors() {
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	m_globalDescriptorAllocator.init_pool(m_device, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_drawImageDescriptorLayout = builder.build(m_device, 
			VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our draw image
	m_drawImageDescriptors = m_globalDescriptorAllocator.allocate(m_device, 
		m_drawImageDescriptorLayout);

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = m_drawImage.imageView;

	VkWriteDescriptorSet drawImageWrite = {};
	drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImageWrite.pNext = nullptr;

	drawImageWrite.dstBinding = 0;
	drawImageWrite.dstSet = m_drawImageDescriptors;
	drawImageWrite.descriptorCount = 1;
	drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImageWrite.pImageInfo = &imgInfo;

	vkUpdateDescriptorSets(m_device, 1, &drawImageWrite, 0, nullptr);
}

void View::initPipelines() {
	initBackgroundPipelines();
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

	VK_CHECK(vkCreatePipelineLayout(m_device, &computeLayout, 
		nullptr, &m_gradientPipelineLayout));

	//layout code
	VkShaderModule computeDrawShader;
	if (!vkutil::loadShaderModule("./shader2.spv", m_device, &computeDrawShader)) {
		std::cout << "Error when building the compute shader \n";
	}

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = computeDrawShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = m_gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;

	VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, 
		&computePipelineCreateInfo, nullptr, &m_gradientPipeline));

	vkDestroyShaderModule(m_device, computeDrawShader, nullptr);

	m_deletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(m_device, m_gradientPipelineLayout, nullptr);
		vkDestroyPipeline(m_device, m_gradientPipeline, nullptr);
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
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(m_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_instance;
	init_info.PhysicalDevice = m_gpu;
	init_info.Device = m_device;
	init_info.Queue = m_graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;
	//init_info.ColorAttachmentFormat = m_swapchainImageFormat;

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	// execute a gpu command to upload imgui font textures
	immediateSubmit([&](VkCommandBuffer cmd) { 
		ImGui_ImplVulkan_CreateFontsTexture(); 
	});

	// clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontsTexture();

	// add the destroy the imgui created structures
	m_deletionQueue.push_function([=]() {
		vkDestroyDescriptorPool(m_device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
		});
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
	ImGui::ShowDemoWindow();

	//make imgui calculate internal draw structures
	ImGui::Render();
}
