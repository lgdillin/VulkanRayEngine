/*#pragma once

#include <set>

#include <vulkan/vulkan.hpp>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include "VkBootsrap.h"

#include "Game.hpp"
//#include "Controller.hpp"
#include "PipelineBuilder.hpp"
#include "Structs.hpp"

#define CLAMP(x, lo, hi)    ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;
constexpr bool enableValidationLayers = true;
constexpr std::array<const char *, 1> VALIDATION_LAYERS{
	//"VK_LAYER_LUNARG_standard_validation"
	"VK_LAYER_KHRONOS_validation"
};

class View {
public:
	//View(Game &_game, Controller &_controller);
	View(Game &_game);
	~View();

	// render loop
	void repaint();

	void initialize();

	// render pipeline
	void createPipeline(VkPipeline &_pipeline, int _mode);

	// render pass

	void acquireNextImage();
	void resetCommandBuffer();
	void beginCommandBuffer();
	void endCommandBuffer();
	void freeCommandBuffer();
	void beginRenderpass(VkClearColorValue _clearColor, VkClearDepthStencilValue _clearDepthStencil);
	void endRenderpass();
	void queueSubmit();
	void queuePresent();
	void setViewport(int _width, int _height);
	void setScissor(int _width, int _height);

	//
	// setup and initialization
	//////////////////////////////////////

	// core
	void initSDL();
	void createWindow();
	void createInstance();
	void createDebug();
	void createSurface();
	void selectPhysicalDevice();
	void selectQueueFamily();
	void createDevice();

	// screen
	bool createSwapchain();
	VkImageView createImageView(VkImage _img, VkFormat _fmt, VkImageAspectFlags _flags);
	void createImageViews();
	void setupDepthStencil();
	void createRenderpass();
	void createFramebuffer();

	// command
	void createCommandPool();
	void createCommandBuffers();
	void createSemaphores();
	void createSemaphore(VkSemaphore *_semaphore);
	void createFences();

	// cleanup
	void cleanup();
	DeletionQueue m_deletionQueue;

	// utility
	void createImage(uint32_t _width, uint32_t _height, VkFormat _format,
		VkImageTiling _tiling, VkImageUsageFlags _usage,
		VkMemoryPropertyFlags _properties, VkImage &_image,
		VkDeviceMemory &_imageMemory);
	uint32_t findMemoryByType(uint32_t _typeFilter, VkMemoryPropertyFlags _props);


	/////////////////////////////////////////
	SDL_Window *getWindow() { return m_window; }
	VkInstance *getInstance(){ return &m_instance; }

	// project
	int m_shaderMode;

private:
	// project
	Game *m_game;
	//Controller *m_controller;
	VkShaderModule m_vShader;
	VkShaderModule m_vShader2;
	VkShaderModule m_fShader;

	// vulkan
	VkDebugReportCallbackEXT m_debugCallback;

	VkInstance m_instance;
	VkSurfaceKHR m_surface;

	VkPhysicalDevice m_physicalDevice;

	uint32_t m_graphics_queueFamilyIndex;
	uint32_t m_present_queueFamilyIndex;

	VkDevice m_logicalDevice;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	//
	// WINDOW
	//////////////////////
	SDL_Window *m_window;

	VkSwapchainKHR m_swapchain;

	VkSurfaceCapabilitiesKHR m_surfaceCapabilities;//
	VkSurfaceFormatKHR m_surfaceFormat;//
	VkExtent2D m_swapchainSize;//

	std::vector<VkImage> m_swapchainImages;//
	uint32_t m_swapchainImageCount;//

	std::vector<VkImageView> m_swapchainImageViews;

	VkFormat m_depthFormat;//
	VkImage m_depthImage;//
	VkDeviceMemory m_depthImageMemory;//
	VkImageView m_depthImageView;//

	VkRenderPass m_renderpass;//

	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	// 
	// command
	/////////////
	VkCommandPool m_commandPool;

	VkCommandBuffer m_commandBuffer;
	std::vector<VkCommandBuffer> m_commandBuffers;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderingFinishedSemaphore;

	std::vector<VkFence> m_fences;

	//
	// pipeline
	///////////////
	PipelineBuilder m_pipelineBuilder;
	VkPipeline m_pipeline;
	VkPipeline m_pipeline2;

	//
	// other
	//////////
	uint32_t m_frameIndex;
	VkPipelineStageFlags m_waitDestStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkImage m_image;
/// <summary>
/// Debug/Callback utility functions
/// </summary>
public:
	PFN_vkCreateDebugReportCallbackEXT SDL2_vkCreateDebugReportCallbackEXT = nullptr;
	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanReportFunc(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char *layerPrefix,
		const char *msg,
		void *userData) {
		printf("VULKAN VALIDATION: %s\n", msg);
		return VK_FALSE;
	}

	VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat) {
		std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		for (auto &format : depthFormats) {
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				*depthFormat = format;
				return true;
			}
		}

		return false;
	}
};