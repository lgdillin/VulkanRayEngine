#include "VreSwapchain.hpp"

vre::VreSwapchain::VreSwapchain(VreDevice &_device, VkExtent2D _extent
) : m_vreDevice(_device), m_windowExtent(_extent) {
	init();
}

vre::VreSwapchain::VreSwapchain(VreDevice &_device, VkExtent2D _extent,
	std::shared_ptr<VreSwapchain> _previous
) : m_vreDevice(_device), m_windowExtent(_extent), m_oldSwapchain(_previous) {
	init();

	// clean up old swapchain since its no longer needed
	m_oldSwapchain = nullptr;
}

vre::VreSwapchain::~VreSwapchain() {
	for (auto &imageView : m_swapchainImageViews) {
		vkDestroyImageView(m_vreDevice.device(), imageView, nullptr);
	}
	m_swapchainImageViews.clear();

	if (m_swapchain != nullptr) {
		vkDestroySwapchainKHR(m_vreDevice.device(), m_swapchain, nullptr);
		m_swapchain = nullptr;
	}

	for (int i = 0; i < m_depthImages.size(); ++i) {
		vkDestroyImageView(m_vreDevice.device(), m_depthImageViews[i], nullptr);
		vkDestroyImage(m_vreDevice.device(), m_depthImages[i], nullptr);
		vkFreeMemory(m_vreDevice.device(), m_depthImageMemorys[i], nullptr);
	}

	for (auto &framebuffer : m_swapchainFramebuffers) {
		vkDestroyFramebuffer(m_vreDevice.device(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(m_vreDevice.device(), m_renderPass, nullptr);

	// cleanup sync objs
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(m_vreDevice.device(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_vreDevice.device(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_vreDevice.device(), m_inFlightFences[i], nullptr);
	}
}

VkFormat vre::VreSwapchain::findDepthFormat() {
	return m_vreDevice.findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, 
		VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
}

VkResult vre::VreSwapchain::acquireNextImage(uint32_t *_imageIndex) {
	vkWaitForFences(m_vreDevice.device(), 1, &m_inFlightFences[m_currentFrame],
		VK_TRUE, std::numeric_limits<uint64_t>::max());
	VkResult result = vkAcquireNextImageKHR(m_vreDevice.device(), m_swapchain,
		std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphores[m_currentFrame],
		VK_NULL_HANDLE, _imageIndex);
	return result;
}

VkResult vre::VreSwapchain::submitCommandBuffers(
	const VkCommandBuffer *_buffers, 
	uint32_t *_imageIndex
) {
	if (m_imagesInFlight[*_imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(m_vreDevice.device(), 1, &m_imagesInFlight[*_imageIndex],
			VK_TRUE, UINT64_MAX);
	}

	m_imagesInFlight[*_imageIndex] = m_inFlightFences[m_currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = _buffers;

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_vreDevice.device(), 1, &m_inFlightFences[m_currentFrame]);
	if (vkQueueSubmit(m_vreDevice.graphicsQueue(), 1, &submitInfo,
		m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw commandbuffer");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = { m_swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = _imageIndex;

	auto result = vkQueuePresentKHR(m_vreDevice.presentQueue(), &presentInfo);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return result;
}

void vre::VreSwapchain::init() {
	createSwapchain();
	createImageViews();
	createRenderpass();
	createDepthResources();
	createFramebuffers();
	createSyncObjects();
}

void vre::VreSwapchain::createSwapchain() {
	SwapchainSupportDetails swapchainSupport = m_vreDevice.getSwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
	if (swapchainSupport.capabilities.maxImageCount > 0
		&& imageCount > swapchainSupport.capabilities.maxImageCount) {
		imageCount = swapchainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_vreDevice.surface();

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = m_vreDevice.findPhysicalQueueFamilies();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // optional
		createInfo.pQueueFamilyIndices = nullptr; // optional
	}

	createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = m_oldSwapchain == nullptr ? VK_NULL_HANDLE : m_oldSwapchain->m_swapchain;



	if (vkCreateSwapchainKHR(m_vreDevice.device(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain!");
	}

	// we only specified a minimum number of images in the swapchain, so the implementation is
	// allowed to create a swapchain with more. thats why we'll first query the final number of
	// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again
	// to retrieve the handles
	vkGetSwapchainImagesKHR(m_vreDevice.device(), m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vreDevice.device(), m_swapchain, &imageCount, m_swapchainImages.data());

	m_swapchainImageFormat = surfaceFormat.format;
	m_swapchainExtent = extent;
}

void vre::VreSwapchain::createImageViews() {
	m_swapchainImageViews.resize(m_swapchainImages.size());

	for (size_t i = 0; i < m_swapchainImages.size(); i++) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_swapchainImageFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_vreDevice.device(), &viewInfo, nullptr,
			&m_swapchainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view");
		}
	}
}

void vre::VreSwapchain::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();
	VkExtent2D swapchainExtent = getSwapchainExtent();

	m_depthImages.resize(imageCount());
	m_depthImageMemorys.resize(imageCount());
	m_depthImageViews.resize(imageCount());

	for (int i = 0; i < m_depthImages.size(); i++) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = swapchainExtent.width;
		imageInfo.extent.height = swapchainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = 0;
	
		m_vreDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_depthImages[i], m_depthImageMemorys[i]);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_depthImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_vreDevice.device(), &viewInfo, nullptr,
			&m_depthImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture image view");
		}
	}
}

void vre::VreSwapchain::createRenderpass() {
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = getSwapchainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcAccessMask = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstSubpass = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_vreDevice.device(), &renderPassInfo, nullptr,
		&m_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

void vre::VreSwapchain::createFramebuffers() {
	m_swapchainFramebuffers.resize(imageCount());
	for (size_t i = 0; i < imageCount(); i++) {
		std::array<VkImageView, 2> attachments = { m_swapchainImageViews[i], m_depthImageViews[i] };

		VkExtent2D swapchainExtent = getSwapchainExtent();
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_vreDevice.device(), &framebufferInfo, nullptr,
			&m_swapchainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer");
		}
	}
}

void vre::VreSwapchain::createSyncObjects() {
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(m_vreDevice.device(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(m_vreDevice.device(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(m_vreDevice.device(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create sync objects for a frame");
		}
	}
}

VkSurfaceFormatKHR vre::VreSwapchain::chooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR> &_availableFormats
) {
	for (const auto &availableFormat : _availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return _availableFormats[0];
}

VkPresentModeKHR vre::VreSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &_availablePresentModes) {
	for (const auto &availablePresentMode : _availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			std::cout << "Present mode: Mailbox" << std::endl;
			return availablePresentMode;
		}
	}

	for (const auto &availablePresentMode : _availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			std::cout << "Present Mode: immediate available, but not selected" << std::endl;
			// return availablePresentMode;
			break;
		}
	}

	std::cout << "Present mode: V-Sync" << std::endl;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vre::VreSwapchain::chooseSwapExtent(
	const VkSurfaceCapabilitiesKHR &_capabilities
) {
	if (_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return _capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = m_windowExtent;
		actualExtent.width = std::max(
			_capabilities.minImageExtent.width,
			std::min(_capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(
			_capabilities.minImageExtent.height,
			std::min(_capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}
