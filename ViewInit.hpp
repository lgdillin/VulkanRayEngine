/*
#pragma once

#include <vector>
#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <vma/vk_mem_alloc.h>

#include "View.hpp"
#include "VkBootstrap.h"
#include "VideoSettings.hpp"
#include "Structs.hpp"
#include "VkFuncs.hpp"
#include "PipelineBuilder.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

class ViewInit {
public:
	ViewInit(View &_view);

	void initVulkan();

	// swapchain
	void initSwapchain();
	void createSwapchain(uint32_t _width, uint32_t _height) {
		m_view->createSwapchain(_width, _height); }

	void resizeSwapchain() { m_view->resizeSwapchain(); }
	void destroySwapchain() { m_view->destroySwapchain(); }
	void initCommands();
	void initSyncStructures();

	// descriptors
	void initDescriptors();

	// pipeline
	//void initPipelines();
	//void initBackgroundPipelines();

	// immediate submit
	//void immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&_function);
	void initImgui();

	View *m_view;
};
*/