

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <iostream>
#include <chrono>

#include "Game.hpp"
#include "View.hpp"
#include "VkInit.hpp"


std::chrono::steady_clock::time_point frameStart, frameEnd;
int frameCount = 0;
static void calculateFPS(View *_view) {
	frameEnd = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart).count();

	if (duration >= 1000) {
		int fps = frameCount * 1000 / duration;
		//std::cout << "FPS: " << fps << std::endl;

		std::string title = "FPS Counter - FPS: " + std::to_string(fps);
		SDL_SetWindowTitle(_view->getWindow(), title.c_str());

		frameStart = std::chrono::steady_clock::now();
		frameCount = 0;
	} else {
		frameCount++;
	}
}

int main(int argc, char *argv[]) {
	// Init MVC
	Game game;

	View view(game);
	view.initialize();

	game.initialize();
	//instance::createInstance(view.getWindow(), *view.getInstance());

	glm::mat4 matrix(1);
	glm::vec4 vec(1);
	auto test = matrix * vec;

	bool quit = false;
	Uint32 frameStart;
	const int targetFramerate = 40;
	const int frameDelay = 1000 / targetFramerate;
	while (!quit) {
		frameStart = SDL_GetTicks();

		// updating
		view.repaint();

		calculateFPS(&view);
	}

	return 0;
}