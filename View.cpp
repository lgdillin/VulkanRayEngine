#include "View.hpp"

View::View() {

}

View::~View() {
}

void View::initialize() {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

	m_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags
	);

	initVulkan();
	initSwapchain();
	initCommands();
	initSyncStructures();

	m_initialized = true;
}

void View::initVulkan() {
	vkb::InstanceBuilder builder;

	// make vulkan instance w/ basic debug features
	auto inst_ret = builder.set_app_name("SDL2/Vulkan")
		.request_validation_layers(ENABLE_VALIDATION_LAYERS)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//grab the instance
	m_instance = vkb_inst.instance;
	m_debugMessenger = vkb_inst.debug_messenger;
}

void View::initSwapchain() {
}

void View::initCommands() {
}

void View::initSyncStructures() {
}
