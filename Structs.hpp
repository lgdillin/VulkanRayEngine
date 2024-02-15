#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>


#include <deque>
#include <functional>

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;


	void push_function(std::function<void()> &&function) {
		deletors.push_back(function);
	}

	void flush() {
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)();
		}

		deletors.clear();
	}
};

struct FrameData {
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkSemaphore swapchainSemaphore;
	VkSemaphore renderSemaphore;
	VkFence renderFence;
};