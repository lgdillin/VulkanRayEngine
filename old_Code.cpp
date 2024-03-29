/*
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


*/