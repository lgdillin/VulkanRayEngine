

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <iostream>
#include <chrono>

#include "Game.hpp"
#include "View.hpp"
#include "Controller.hpp"


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

	Controller controller(game);

	View view(game);
	view.initialize();

	controller.setView(view);

	game.initialize();

	bool quit = false;
	Uint32 frameStart;
	const int targetFramerate = 40;
	const int frameDelay = 1000 / targetFramerate;
	while (!quit) {
		frameStart = SDL_GetTicks();

		controller.update();

		// act if we are minimized
		if (view.m_stopRendering) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		game.update();

		// updating
		view.newFrame();
		view.draw();

		calculateFPS(&view);
	}

	return 0;
}

int main11() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Version Query";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	VkInstance instance;
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS) {
		std::cerr << "Failed to create Vulkan instance" << std::endl;
		return -1;
	}

	// Query Vulkan runtime version
	uint32_t apiVersion;
	vkEnumerateInstanceVersion(&apiVersion);
	std::cout << "Vulkan Runtime Version: " << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << std::endl;

	// Clean up
	vkDestroyInstance(instance, nullptr);

	return 0;
}