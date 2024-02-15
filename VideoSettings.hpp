#pragma once

constexpr bool ENABLE_VALIDATION_LAYERS = true;

constexpr int VULKAN_VERSION[3] = { 1, 3, 0 };
constexpr unsigned int FRAME_OVERLAP = 2;

constexpr bool VYSNC = false;
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;


#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)