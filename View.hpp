#pragma once

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

#include "VkBootsrap.h"

constexpr bool ENABLE_VALIDATION_LAYERS = true;

class View {
public:
	View();
	~View();

	void initialize();

	// sdl
	SDL_Window *m_window;
	bool m_initialized;

	// vulkan
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkSurfaceKHR m_surface;
private:
	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initSyncStructures();
};