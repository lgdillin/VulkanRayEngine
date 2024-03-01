#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <SDL2/sdl_vulkan.h>
#include <vulkan/vulkan.h>

#include "VideoSettings.hpp"
#include "VreWindow.hpp"
#include "VreDevice.hpp"
#include "VrePipeline.hpp"
#include "VreSwapchain.hpp"
#include "Game.hpp"

class View {
public:

	View(Game &_game);
	~View();

	void update();

	VkExtent2D getExtent() { return { static_cast<uint32_t>(WINDOW_WIDTH), static_cast<uint32_t>(WINDOW_HEIGHT) }; }

	SDL_Window *getWindow() { return m_vreWindow.m_window; }
private:
	Game *m_game;
	vre::VreWindow m_vreWindow{};
	vre::VreDevice m_vreDevice{ m_vreWindow.m_window };
	
	//vre::VreSwapchain m_vreSwapchain{ m_vreDevice, getExtent()};
	std::unique_ptr<vre::VreSwapchain> m_vreSwapchain;

	//vre::VrePipeline m_vrePipeline{m_vreDevice.m_device, "./triangle.vert.spv", "./triangle.frag.spv"};
	std::unique_ptr<vre::VrePipeline> m_vrePipeline;
	std::unique_ptr<vre::VreModel> m_model;
	
	VkPipelineLayout m_pipelineLayout;
	std::vector<VkCommandBuffer> m_commandBuffers;
	
	void loadModel();
	void createPipelineLayout();
	void createPipeline();
	void createCommandBuffers();
	void freeCommandBuffers();
	void drawFrame();
	void recreateSwapchain();
	void recordCommandBuffer(int _imageIndex);

	//SDL_Window *m_window;
	//std::shared_ptr<vre::VreDevice> m_vreDevice;
	//std::shared_ptr<vre::Pipeline> m_pipeline;
	//std::shared_ptr<vre::Swapchain> m_swapchain;
};