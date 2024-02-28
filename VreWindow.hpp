#pragma once

#include <SDL2/SDL.h>
#include <SDL2/sdl_vulkan.h>
#include <vulkan/vulkan.h>

#include "VideoSettings.hpp"

namespace vre {
	class VreWindow {
	public:
		VreWindow() {
			SDL_Init(SDL_INIT_VIDEO);
			SDL_WindowFlags window_flags = (SDL_WindowFlags)(
				SDL_WINDOW_VULKAN
				| SDL_WINDOW_SHOWN);

			m_window = SDL_CreateWindow(
				"Vulkan Engine",
				SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED,
				WINDOW_WIDTH,
				WINDOW_HEIGHT,
				window_flags
			);
		}
		~VreWindow() {}
		SDL_Window *m_window;
	};
}