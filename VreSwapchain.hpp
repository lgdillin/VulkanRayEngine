#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#include "VreDevice.hpp"

namespace vre {
	class VreSwapchain {
	public:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		VreSwapchain(VreDevice &_device, VkExtent2D _extent);
		VreSwapchain(VreDevice &_device, VkExtent2D _extent, 
			std::shared_ptr<VreSwapchain> _previous);
		~VreSwapchain();

		VkFramebuffer getFramebuffer(int _index) { return m_swapchainFramebuffers[_index]; }
		VkRenderPass getRenderPass() { return m_renderPass; }
		VkImageView getImageView(int _index) { return m_swapchainImageViews[_index]; }
		size_t imageCount() { return m_swapchainImages.size(); }
		VkFormat getSwapchainImageFormat() { return m_swapchainImageFormat; }
		VkExtent2D getSwapchainExtent() { return m_swapchainExtent; }
		uint32_t width() { return m_swapchainExtent.width; }
		uint32_t height() { return m_swapchainExtent.height; }

		float extentAspectRatio() {
			return static_cast<float>(m_swapchainExtent.width) / static_cast<float>(m_swapchainExtent.height);
		}

		VkFormat findDepthFormat();
		VkResult acquireNextImage(uint32_t *_imageIndex);
		VkResult submitCommandBuffers(const VkCommandBuffer *_buffers, uint32_t *_imageIndex);
	private:
		void init();
		void createSwapchain();
		void createImageViews();
		void createDepthResources();
		void createRenderpass();
		void createFramebuffers();
		void createSyncObjects();

		// helpers
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(
			const std::vector<VkSurfaceFormatKHR> &_availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(
			const std::vector<VkPresentModeKHR> &_availablePresentModes);
		VkExtent2D chooseSwapExtent(
			const VkSurfaceCapabilitiesKHR &_capabilities);

		VreDevice &m_vreDevice;
		VkExtent2D m_windowExtent;

		VkSwapchainKHR m_swapchain;
		std::shared_ptr<VreSwapchain> m_oldSwapchain;

		VkFormat m_swapchainImageFormat;
		VkExtent2D m_swapchainExtent;

		std::vector<VkFramebuffer> m_swapchainFramebuffers;
		VkRenderPass m_renderPass;

		std::vector<VkImage> m_depthImages;
		std::vector<VkDeviceMemory> m_depthImageMemorys;
		std::vector<VkImageView> m_depthImageViews;
		std::vector<VkImage> m_swapchainImages;
		std::vector<VkImageView> m_swapchainImageViews;

		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		std::vector<VkFence> m_imagesInFlight;
		size_t m_currentFrame = 0;
	};
}