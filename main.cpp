

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <iostream>
#include <chrono>

#include "Game.hpp"
#include "View.hpp"
#include "ViewInit.hpp"
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
	//ViewInit viewInit(view);
	//view.initialize();

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
		//if (view.m_stopRendering) {
		//	std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//	continue;
		//}

		game.update();

		//if (view.m_resizeRequested) {
		//	view.resizeSwapchain();
		//}

		// updating
		//view.newFrame();
		//view.draw();
		view.update();

		calculateFPS(&view);
	}

	return 0;
}

int main1() {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;

	// Create Vulkan instance
	VkInstanceCreateInfo createInfo = {};
	// Fill in createInfo...
	vkCreateInstance(&createInfo, nullptr, &instance);

	// Enumerate physical devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		std::cerr << "No physical devices found." << std::endl;
		return -1;
	}
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

	// Select the first physical device
	physicalDevice = physicalDevices[0];

	// Query physical device properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	// Output API version
	uint32_t apiVersion = deviceProperties.apiVersion;
	std::cout << "API Version Supported by Physical Device: "
		<< VK_VERSION_MAJOR(apiVersion) << "."
		<< VK_VERSION_MINOR(apiVersion) << "."
		<< VK_VERSION_PATCH(apiVersion) << std::endl;

	// Cleanup
	vkDestroyInstance(instance, nullptr);

	return 0;
}