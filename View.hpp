#pragma once

#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "Game.hpp"
#include "VkBootstrap.h"
#include "VideoSettings.hpp"
#include "Structs.hpp"
#include "VkFuncs.hpp"

class View {
public:
	View();
	View(Game &_game);
	~View();

	void draw();

	void initialize();

	// project
	SDL_Window *getWindow() { return m_window; }
	Game *m_game;
	bool m_initialized;

	// sdl
	SDL_Window *m_window;
	VkExtent2D m_windowExtent{ WINDOW_WIDTH, WINDOW_HEIGHT };

	// 
	// Core
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkSurfaceKHR m_surface;

	//
	// Swapchain
	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchainImageFormat;
	std::vector<VkImage> m_swapchainImages; // VkImage is the texture we render onto
	std::vector<VkImageView> m_swapchainImageViews; // VkImageView is wrapper for VkImage
	VkExtent2D m_swapchainExtent;

	//
	// command
	FrameData &getCurrentFrame() { return m_frames[m_frameNumber % FRAME_OVERLAP]; }
	uint32_t m_frameNumber;
	FrameData m_frames[FRAME_OVERLAP];
	VkQueue m_graphicsQueue;
	uint32_t m_graphicsQueueFamily;
	DeletionQueue m_deletionQueue;

	// 
	// memory
	VmaAllocator m_allocator;
	AllocatedImage m_drawImage;
	VkExtend2D m_drawExtent;

private:
	// core
	void initVulkan();

	// swapchain
	void initSwapchain();
	void createSwapchain(uint32_t _width, uint32_t _height);
	void destroySwapchain();

	void initCommands();
	void initSyncStructures();

	// cleanup
	void cleanup();
};