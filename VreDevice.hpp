#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <unordered_set>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "VideoSettings.hpp"

const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

namespace vre {
	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices {
		uint32_t graphicsFamily;
		uint32_t presentFamily;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
	};


	class VreDevice {
	public:
		VreDevice() {}
		VreDevice(SDL_Window *_window);
		~VreDevice();

		VkCommandPool getCommandPool() { return m_commandPool; }
		VkDevice device() { return m_device; }
		VkSurfaceKHR surface() { return m_surface; }
		VkQueue graphicsQueue() { return m_graphicsQueue; }
		VkQueue presentQueue() { return m_presentQueue; }

		SwapchainSupportDetails getSwapChainSupport() { return querySwapchainSupport(m_physicalDevice); }
		uint32_t findMemoryType(uint32_t _typeFilter, VkMemoryPropertyFlags _properties);
		QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(m_physicalDevice); }
		VkFormat findSupportedFormat(const std::vector<VkFormat> &_candidates, 
			VkImageTiling _tiling, VkFormatFeatureFlags _features);
	
	
		// 
		// Buffer functions
		void createBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, 
			VkMemoryPropertyFlags _properties, VkBuffer &_buffer, 
			VkDeviceMemory &_bufferMemory);
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer _cmd);
		void copyBuffer(VkBuffer _srcBuffer, VkBuffer _dstBuffer, VkDeviceSize _size);
		void copyBufferToImage(VkBuffer _buffer, VkImage _image, uint32_t _width, 
			uint32_t _height, uint32_t _layerCount);
		
		void createImageWithInfo(const VkImageCreateInfo &_imageInfo, 
			VkMemoryPropertyFlags _props, VkImage &_image, VkDeviceMemory &_imageMemory);
	
		VkPhysicalDeviceProperties m_physDeviceProps;
	private:
		SDL_Window *m_window;

		VkInstance m_instance;
		void createInstance();

		VkDebugUtilsMessengerEXT m_debugMessenger;
		void setupDebugMessenger();

		VkSurfaceKHR m_surface;
		void createSurface();

		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		void pickPhysicalDevice();

		VkDevice m_device;
		void createLogicalDevice();

		VkCommandPool m_commandPool;
		void createCommandPool();

		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;

		// helper functions
		bool isDeviceSuitable(VkPhysicalDevice _device);

		std::vector<const char *> getRequiredExtensions();

		bool checkValidationLayerSupport();

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _device);

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &_info);

		void hasSDLRequiredInstanceExtensions();
		
		bool checkDeviceExtensionSupport(VkPhysicalDevice _device);

		SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice _device);
	};
}