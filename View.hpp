#pragma once

#include <SDL2/SDL.h>
#include <SDL2/sdl_vulkan.h>
#include <vulkan/vulkan.h>

#include "VideoSettings.hpp"
#include "VreDevice.hpp"
#include "Pipeline.hpp"
#include "Game.hpp"

class View {
public:

	View(Game &_game);
	~View();

	void update();

	SDL_Window *getWindow() { return m_window; }
private:
	Game *m_game;
	SDL_Window *m_window;
	vre::VreDevice m_vreDevice;
	vre::Pipeline m_pipeline{ m_vreDevice, 
		vre::Pipeline::defaultPipelineConfigInfo(WINDOW_WIDTH, WINDOW_HEIGHT),
		"./triangle.vert", "./triangle.frag" };
};