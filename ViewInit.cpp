#include "ViewInit.hpp"

ViewInit::ViewInit(View &_view) : m_view(&_view) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(
		SDL_WINDOW_VULKAN
		| SDL_WINDOW_RESIZABLE);

	m_view->m_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		m_view->m_windowExtent.width,
		m_view->m_windowExtent.height,
		window_flags
	);

	initVulkan();
	initSwapchain();
	initCommands();
	initSyncStructures();
	initDescriptors();
	initImgui();
}

void ViewInit::initVulkan() {
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
	m_view->m_instance = vkb_inst.instance;
	m_view->m_debugMessenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(m_view->m_window, m_view->m_instance, &(m_view->m_surface));

	//SetupVulkan_SelectPhysicalDevice();

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
		.set_surface(m_view->m_surface)
		.select()
		.value();


	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	m_view->m_device = vkbDevice.device;
	m_view->m_gpu = physicalDevice.physical_device;

	// use vkbootstrap to get a Graphics queue
	m_view->m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_view->m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = m_view->m_gpu;
	allocatorInfo.device = m_view->m_device;
	allocatorInfo.instance = m_view->m_instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &m_view->m_allocator);

	m_view->m_deletionQueue.push_function([&]() {
		vmaDestroyAllocator(m_view->m_allocator);
		});
}

void ViewInit::initSwapchain() {
	createSwapchain(m_view->m_windowExtent.width, m_view->m_windowExtent.height);

	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		m_view->m_windowExtent.width,
		m_view->m_windowExtent.height,
		1
	};

	//hardcoding the draw format to 32 bit float
	m_view->m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	m_view->m_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkinit::imageCreateInfo(m_view->m_drawImage.imageFormat,
		drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(m_view->m_allocator, &rimg_info, &rimg_allocinfo,
		&m_view->m_drawImage.image, &m_view->m_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageviewCreateInfo(
		m_view->m_drawImage.imageFormat, m_view->m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(m_view->m_device, &rview_info, nullptr, 
		&m_view->m_drawImage.imageView));

	//add to deletion queues
	m_view->m_deletionQueue.push_function([=]() {
		vkDestroyImageView(m_view->m_device, m_view->m_drawImage.imageView, nullptr);
		vmaDestroyImage(m_view->m_allocator, m_view->m_drawImage.image, m_view->m_drawImage.allocation);
		});
}

void ViewInit::initCommands() {
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(
		m_view->m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VK_CHECK(vkCreateCommandPool(m_view->m_device, &commandPoolInfo, nullptr,
		&m_view->m_immCommandPool));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(
		m_view->m_immCommandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(m_view->m_device, &cmdAllocInfo, &m_view->m_immCommandBuffer));

	m_view->m_deletionQueue.push_function([=]() {
		vkDestroyCommandPool(m_view->m_device, m_view->m_immCommandPool, nullptr);
		});

	for (int i = 0; i < FRAME_OVERLAP; i++) {

		VK_CHECK(vkCreateCommandPool(m_view->m_device, &commandPoolInfo, 
			nullptr, &m_view->m_frames[i].commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(
			m_view->m_frames[i].commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(m_view->m_device, &cmdAllocInfo, 
			&m_view->m_frames[i].commandBuffer));
	}
}

void ViewInit::initSyncStructures() {
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo(0);

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(m_view->m_device, &fenceCreateInfo, nullptr,
			&m_view->m_frames[i].renderFence));

		VK_CHECK(vkCreateSemaphore(m_view->m_device, &semaphoreCreateInfo, nullptr,
			&m_view->m_frames[i].swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(m_view->m_device, &semaphoreCreateInfo, nullptr,
			&m_view->m_frames[i].renderSemaphore));
	}

	VK_CHECK(vkCreateFence(m_view->m_device, &fenceCreateInfo, nullptr, &m_view->m_immFence));
	m_view->m_deletionQueue.push_function([=]() {
		vkDestroyFence(m_view->m_device, m_view->m_immFence, nullptr);
		});
}

void ViewInit::initDescriptors() {
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	m_view->m_globalDescriptorAllocator.init_pool(m_view->m_device, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_view->m_drawImageDescriptorLayout = builder.build(m_view->m_device,
			VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our draw image
	m_view->m_drawImageDescriptors = m_view->m_globalDescriptorAllocator.allocate(
		m_view->m_device, m_view->m_drawImageDescriptorLayout);

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = m_view->m_drawImage.imageView;

	VkWriteDescriptorSet drawImageWrite = {};
	drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImageWrite.pNext = nullptr;

	drawImageWrite.dstBinding = 0;
	drawImageWrite.dstSet = m_view->m_drawImageDescriptors;
	drawImageWrite.descriptorCount = 1;
	drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImageWrite.pImageInfo = &imgInfo;

	vkUpdateDescriptorSets(m_view->m_device, 1, &drawImageWrite, 0, nullptr);
}

void ViewInit::initImgui() {
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
	VK_CHECK(vkCreateDescriptorPool(m_view->m_device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	ImGui::StyleColorsDark();
	ImGuiStyle &style = ImGui::GetStyle();
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
	//	style.WindowRounding = 0.0f;
	//	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	//}

	// this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(m_view->m_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_view->m_instance;
	init_info.PhysicalDevice = m_view->m_gpu;
	init_info.Device = m_view->m_device;
	init_info.Queue = m_view->m_graphicsQueue;
	init_info.QueueFamily = m_view->m_graphicsQueueFamily;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;

	init_info.UseDynamicRendering = true;
	init_info.ColorAttachmentFormat = m_view->m_swapchainImageFormat;
	//init_info.PipelineRenderingCreateInfo = {};
	//init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	//init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchainImageFormat;
	//init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = m_swapchainImageFormat;

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	//init_info.Allocator = m_allocator;

	ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

	// execute a gpu command to upload imgui font textures
	ImGui_ImplVulkan_CreateFontsTexture();
	//immediateSubmit([&](VkCommandBuffer cmd) { 
	//	ImGui_ImplVulkan_CreateFontsTexture(); 
	//});

	// clear font textures from cpu data
	//ImGui_ImplVulkan_
	//ImGui_ImplVulkan_DestroyFontsTexture();

	// add the destroy the imgui created structures
	m_view->m_deletionQueue.push_function([=]() {
		vkDestroyDescriptorPool(m_view->m_device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
		});
}
