#include "View.hpp"

View::View(Game &_game) : m_game(&_game) {
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

	m_vreDevice = vre::VreDevice(m_window);
}

View::~View() {
}

void View::update() {

}
