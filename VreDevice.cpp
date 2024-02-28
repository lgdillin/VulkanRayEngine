#include "VreDevice.hpp"

namespace vreDebug {
    // local callback functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
        std::cerr << std::endl << "=========[VALIDATION LAYER]=========" 
            << std::endl << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance,
            "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance,
            "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
}

vre::VreDevice::VreDevice(SDL_Window *_window) : m_window(_window) {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
}

void vre::VreDevice::init() {
    
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();

    std::cout << m_device << std::endl;
    //doStuff();
}

vre::VreDevice::~VreDevice() {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyDevice(m_device, nullptr);

    if (ENABLE_VALIDATION_LAYERS) {
        vreDebug::DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}



uint32_t vre::VreDevice::findMemoryType(uint32_t _typeFilter, 
    VkMemoryPropertyFlags _properties
) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((_typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & _properties) == _properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

VkFormat vre::VreDevice::findSupportedFormat(
    const std::vector<VkFormat> &_candidates, 
    VkImageTiling _tiling, 
    VkFormatFeatureFlags _features
) {
    for (VkFormat format : _candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (_tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & _features) == _features) {
            return format;
        } else if (
            _tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & _features) == _features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

void vre::VreDevice::createBuffer(
    VkDeviceSize _size, 
    VkBufferUsageFlags _usage, 
    VkMemoryPropertyFlags _properties, 
    VkBuffer &_buffer, 
    VkDeviceMemory &_bufferMemory
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = _size;
    bufferInfo.usage = _usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, _buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &_bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(m_device, _buffer, _bufferMemory, 0);
}

VkCommandBuffer vre::VreDevice::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void vre::VreDevice::endSingleTimeCommands(VkCommandBuffer _cmd) {
    vkEndCommandBuffer(_cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_cmd;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &_cmd);
}

void vre::VreDevice::copyBuffer(
    VkBuffer _srcBuffer,
    VkBuffer _dstBuffer, 
    VkDeviceSize _size
) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = _size;
    vkCmdCopyBuffer(commandBuffer, _srcBuffer, _dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void vre::VreDevice::copyBufferToImage(
    VkBuffer _buffer, 
    VkImage _image, 
    uint32_t _width, 
    uint32_t _height, 
    uint32_t _layerCount
) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = _layerCount;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { _width, _height, 1 };

    vkCmdCopyBufferToImage(
        commandBuffer,
        _buffer,
        _image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
    endSingleTimeCommands(commandBuffer);
}

void vre::VreDevice::createImageWithInfo(
    const VkImageCreateInfo &_imageInfo, 
    VkMemoryPropertyFlags _props, 
    VkImage &_image, 
    VkDeviceMemory &_imageMemory
) {
    if (vkCreateImage(m_device, &_imageInfo, nullptr, &_image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, _image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _props);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    if (vkBindImageMemory(m_device, _image, _imageMemory, 0) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind image memory!");
    }
}

void vre::VreDevice::createInstance() {
    if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanRayEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    //if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create instance!");
    //}

    if (result == VK_ERROR_LAYER_NOT_PRESENT) {
        std::cerr << "Validation layers requested, but not available!" << std::endl;
    }
    else if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance!" << std::endl;
    }
    else {
        std::cout << "Vulkan instance created successfully!" << std::endl;
    }

    hasSDLRequiredInstanceExtensions();
}

void vre::VreDevice::setupDebugMessenger() {
    if (!ENABLE_VALIDATION_LAYERS) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (vreDebug::CreateDebugUtilsMessengerEXT(m_instance, &createInfo, 
        nullptr, &m_debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void vre::VreDevice::createSurface() {
    SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);
}

void vre::VreDevice::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::cout << "Device count: " << deviceCount << std::endl;
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto &device : devices) {
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physDeviceProps);
    std::cout << "physical device: " << m_physDeviceProps.deviceName << std::endl;
}

void vre::VreDevice::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // might not really be necessary anymore because device specific validation layers
    // have been deprecated
    if (ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily, 0, &m_presentQueue);
}

void vre::VreDevice::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags =
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

bool vre::VreDevice::isDeviceSuitable(VkPhysicalDevice _device) {
    QueueFamilyIndices indices = findQueueFamilies(_device);

    bool extensionsSupported = checkDeviceExtensionSupport(_device);

    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(_device);
        swapchainAdequate = !swapchainSupport.formats.empty() 
            && !swapchainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(_device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapchainAdequate &&
        supportedFeatures.samplerAnisotropy;
}

std::vector<const char *> vre::VreDevice::getRequiredExtensions() {
    uint32_t sdlExtensionsCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(m_window, &sdlExtensionsCount, nullptr) != SDL_TRUE) {
        throw std::runtime_error("Couldn't get an extensions cound");
    }

    const char **sdlExtensions = new const char *[sdlExtensionsCount];
    if (SDL_Vulkan_GetInstanceExtensions(m_window, &sdlExtensionsCount, sdlExtensions) != SDL_TRUE) {
        throw std::runtime_error("failed to get required extensions");
    }

    std::vector<const char *> extensions(sdlExtensions, 
        sdlExtensions + sdlExtensionsCount);

    if (ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool vre::VreDevice::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

vre::QueueFamilyIndices vre::VreDevice::findQueueFamilies(VkPhysicalDevice _device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            indices.graphicsFamilyHasValue = true;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, m_surface, &presentSupport);
        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
            indices.presentFamilyHasValue = true;
        }
        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}




void vre::VreDevice::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &_info
) {
    _info = {};
    _info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    _info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; 
        //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    _info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        //| VK_DEBUG_UTILS_MESSAGE_TYPE_;
    _info.pfnUserCallback = vreDebug::debugCallback;
    _info.pUserData = nullptr;  // Optional
}

void vre::VreDevice::hasSDLRequiredInstanceExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::cout << "available extensions:" << std::endl;
    std::unordered_set<std::string> available;
    for (const auto &extension : extensions) {
        std::cout << "\t" << extension.extensionName << std::endl;
        available.insert(extension.extensionName);
    }

    std::cout << "required extensions:" << std::endl;
    auto requiredExtensions = getRequiredExtensions();
    for (const auto &required : requiredExtensions) {
        std::cout << "\t" << required << std::endl;
        if (available.find(required) == available.end()) {
            throw std::runtime_error("Missing required SDL extension");
        }
    }
}

bool vre::VreDevice::checkDeviceExtensionSupport(VkPhysicalDevice _device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        _device,
        nullptr,
        &extensionCount,
        availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

vre::SwapchainSupportDetails vre::VreDevice::querySwapchainSupport(VkPhysicalDevice _device) {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, m_surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            _device,
            m_surface,
            &presentModeCount,
            details.presentModes.data());
    }
    return details;
}