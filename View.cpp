#include "View.hpp"

View::View() {
	m_window = nullptr;
	m_surface = nullptr;
	m_instance = nullptr;


}

View::~View() {

	if (enableValidationLayers) {
		//validation::destroyDebugUtilsMessengerEXT(m_instance,
		//	m_debugMessenger, nullptr);
	}
	vkDestroyInstance(m_instance, nullptr);
	SDL_DestroyWindow(m_window);
}

void View::initialize() {
	// core
	initSDL();
	createWindow();
	createInstance();
	createDebug();
	createSurface();
	selectPhysicalDevice();
	selectQueueFamily();
	createDevice();

	// screen
	createSwapchain();
}

/// <summary>
/// Initialize SDL and load the SDL/Vulkan libraries
/// </summary>
void View::initSDL() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Log("Couldn't Initialize SDL: Error %s\n", SDL_GetError());
		SDL_Quit();
		return;
	}

	if (SDL_Vulkan_LoadLibrary(NULL) < 0) {
		printf("SDL could not load Vulkan library! SDL_Error: %s\n", SDL_GetError());
		SDL_Quit();
		return;
	}
}

/// <summary>
/// Create a SDL window
/// </summary>
void View::createWindow() {
	m_window = SDL_CreateWindow("Vulkan/SDL2",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_VULKAN);

	if (!m_window) {
		printf("SDL could not create window! SDL_Error: %s\n", SDL_GetError());
		SDL_Quit();
		return;
	}
}

/// <summary>
/// Create a vulkan instance via enumerate extensions using SDL helper funcs
/// </summary>
void View::createInstance() {
	unsigned int count;
	if (SDL_Vulkan_GetInstanceExtensions(m_window, &count, nullptr) != SDL_TRUE) {
		printf("Couldn't get extensions count! SDL_Error: %s\n", SDL_GetError());
	}

	const char **extensions = new const char *[count];
	if (SDL_Vulkan_GetInstanceExtensions(m_window, &count, extensions) != SDL_TRUE) {
		printf("Failed to get required instance extensions! SDL_Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(m_window);
		SDL_Quit();
		return;
	}

	// App info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "SDL2/Vulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Create Vulkan instance
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Specify enabled extensions
	createInfo.enabledExtensionCount = count;
	createInfo.ppEnabledExtensionNames = extensions;

	for (int i = 0; i < count; ++i) {
		std::cout << extensions[i] << std::endl;
	}

	SDL_Vulkan_GetInstanceExtensions(m_window, &count, (const char **)createInfo.ppEnabledExtensionNames);

	VkResult result = vkCreateInstance(&createInfo, NULL, &m_instance);
	if (result != VK_SUCCESS) {
		printf("Failed to create Vulkan instance! Error code: %d\n", result);
		SDL_DestroyWindow(m_window);
		SDL_Quit();
		return;
	}
}

void View::createDebug() {
	SDL2_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)SDL_Vulkan_GetVkGetInstanceProcAddr();

	VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo = {};
	debugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	debugCallbackCreateInfo.pfnCallback = VulkanReportFunc;

	SDL2_vkCreateDebugReportCallbackEXT(m_instance, &debugCallbackCreateInfo, 0, &m_debugCallback);
}

/// <summary>
/// Create an SDL/Vulkan Surface
/// </summary>
void View::createSurface() {
	if (!SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface)) {
		printf("Failed to create Vulkan surface! SDL_Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(m_window);
		SDL_Quit();
		return;
	}
}



void View::selectPhysicalDevice() {
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());
	m_physicalDevice = physicalDevices[0];
}

void View::selectQueueFamily() {
	std::vector<VkQueueFamilyProperties> queueFamilyProperties{};
	uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
	queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	int graphicIndex = -1;
	int presentIndex = -1;

	int i = 0;
	for (const auto &queueFamily : queueFamilyProperties) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicIndex = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport) {
			presentIndex = i;
		}

		if (graphicIndex != -1 && presentIndex != -1) {
			break;
		}

		i++;
	}

	m_graphics_queueFamilyIndex = graphicIndex;
	m_present_queueFamilyIndex = presentIndex;
}

void View::createDevice() {
	const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	const float queue_priority[] = { 1.0f };

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { 
		m_graphics_queueFamilyIndex, 
		m_present_queueFamilyIndex 
	};

	float queuePriority = queue_priority[0];
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = m_graphics_queueFamilyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	//https://en.wikipedia.org/wiki/Anisotropic_filtering
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	createInfo.enabledLayerCount = VALIDATION_LAYERS.size();
	createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

	vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);

	vkGetDeviceQueue(m_logicalDevice, 
		m_graphics_queueFamilyIndex, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, 
		m_present_queueFamilyIndex, 0, &m_presentQueue);
}

bool View::createSwapchain() {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, 
		&m_surfaceCapabilities);

	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	uint32_t surfaceFormatsCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface,
		&surfaceFormatsCount,
		nullptr);
	surfaceFormats.resize(surfaceFormatsCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface,
		&surfaceFormatsCount,
		surfaceFormats.data());

	if (surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM) {
		throw std::runtime_error("surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM");
	}

	m_surfaceFormat = surfaceFormats[0];
	int width, height = 0;
	SDL_Vulkan_GetDrawableSize(m_window, &width, &height);
	width = CLAMP(width, m_surfaceCapabilities.minImageExtent.width, 
		m_surfaceCapabilities.maxImageExtent.width);
	height = CLAMP(height, m_surfaceCapabilities.minImageExtent.height,
		m_surfaceCapabilities.maxImageExtent.height);
	m_swapchainSize.width = width;
	m_swapchainSize.height = height;

	uint32_t imageCount = m_surfaceCapabilities.minImageCount + 1;
	if (m_surfaceCapabilities.maxImageCount > 0 
		&& imageCount > m_surfaceCapabilities.maxImageCount
	) {
		imageCount = m_surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = m_surfaceCapabilities.minImageCount;
	createInfo.imageFormat = m_surfaceFormat.format;
	createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
	createInfo.imageExtent = m_swapchainSize;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { 
		m_graphics_queueFamilyIndex, 
		m_present_queueFamilyIndex
	};


	if (m_graphics_queueFamilyIndex != m_present_queueFamilyIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = m_surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.clipped = VK_TRUE;

	vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapchain);

	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &m_swapchainImageCount, nullptr);
	m_swapchainImages.resize(m_swapchainImageCount);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, 
		&m_swapchainImageCount, m_swapchainImages.data());

	return true;
}

VkImageView View::createImageView(VkImage _img, VkFormat _fmt, VkImageAspectFlags _flags) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = _img;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = _fmt;
	viewInfo.subresourceRange.aspectMask = _flags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(m_logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void View::createImageViews() {
	m_swapchainImageViews.resize(m_swapchainImages.size());

	for (uint32_t i = 0; i < m_swapchainImages.size(); i++) {
		m_swapchainImageViews[i] = createImageView(m_swapchainImages[i],
			m_surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void View::setupDepthStencil() {
	VkBool32 validDepthFormat = getSupportedDepthFormat(
		m_physicalDevice, &m_depthFormat);

	createImage(
		m_swapchainSize.width, 
		m_swapchainSize.height,
		VK_FORMAT_D32_SFLOAT_S8_UINT, 
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_depthImage, 
		m_depthImageMemory
	);

	m_depthImageView = createImageView(m_depthImage, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void View::createImage(uint32_t _width, uint32_t _height, 
	VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, 
	VkMemoryPropertyFlags _properties, VkImage &_image, 
	VkDeviceMemory &_imageMemory
) {
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = _width;
	imageInfo.extent.height = _height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = _format;
	imageInfo.tiling = _tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = _usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(m_logicalDevice, &imageInfo, nullptr, &_image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_logicalDevice, _image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryByType(memRequirements.memoryTypeBits, _properties);

	if (vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(m_logicalDevice, _image, _imageMemory, 0);
}

uint32_t View::findMemoryByType(uint32_t _typeFilter, VkMemoryPropertyFlags _props) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((_typeFilter & (1 << i)) 
			&& (memProperties.memoryTypes[i].propertyFlags & _props) == _props) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}
