#pragma once

#include <SDL2/SDL.h>
#include <SDL2/sdl_vulkan.h>
#include <vulkan/vulkan.h>

#include "VideoSettings.hpp"
#include "VreDevice.hpp"
#include "Game.hpp"

class View {
public:

	View(Game &_game);
	~View();

	SDL_Window *getWindow() { return m_window; }
private:
	Game *m_game;
	SDL_Window *m_window;
	vre::VreDevice m_vreDevice;
};