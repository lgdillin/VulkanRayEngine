#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>

#define enableValidationLayers true

namespace validation {
	static VkResult createInstanceVL(const VkInstanceCreateInfo *_pCreateInfo,
		const VkAllocationCallbacks *_pAllocator, VkInstance *_instance
	) {
		if (_pCreateInfo == nullptr || _instance == nullptr) {
			SDL_Log("nullptr passed %s\n", SDL_GetError());
			return VK_ERROR_INITIALIZATION_FAILED;
		}
	}

	static bool checkValidationLayerSupport() {
		const std::vector<const char *> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char *layerName : validationLayers) {
			bool layerFound = false;

			for (const auto &layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}
	
	static std::vector<const char *> getRequiredExtensions(SDL_Window *_window) {
		uint32_t extCount = 0;
		const char **exts;
		SDL_Vulkan_GetInstanceExtensions(_window, &extCount, nullptr);
		exts = new const char *[extCount];
		bool value = SDL_Vulkan_GetInstanceExtensions(_window,
			&extCount, exts);

		std::vector<const char *> extensions(exts, exts + extCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT _messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *_pCallbackData,
		void *pUserData
	) {
		std::cerr << "Validation layer: " << _pCallbackData->pMessage
			<< std::endl;

		if (_messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			// Message is important enough to show
		}

		return VK_FALSE;
	}

	static void populateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT &_createInfo
	) {
		_createInfo = {};
		_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		_createInfo.pfnUserCallback = debugCallback;
	}

	static VkResult createDebugUtilsMessengerEXT(
		VkInstance _instance,
		const VkDebugUtilsMessengerCreateInfoEXT *_pCreateInfo,
		const VkAllocationCallbacks *_pAllocator,
		VkDebugUtilsMessengerEXT *_pDebugMessenger
	) {

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");


		if (func != nullptr) {
			return func(_instance, _pCreateInfo, _pAllocator,
				_pDebugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	static void setupDebugMessenger(VkInstance _instance,
		VkDebugUtilsMessengerEXT _debugMessenger
	) {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		if (createDebugUtilsMessengerEXT(_instance, &createInfo,
			nullptr, &_debugMessenger) != VK_SUCCESS
		) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	static void destroyDebugUtilsMessengerEXT(
		VkInstance _instance,
		VkDebugUtilsMessengerEXT _debugMessenger,
		const VkAllocationCallbacks *_pAllocator
	) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(_instance, _debugMessenger, _pAllocator);
		}
	}
}

namespace instance {
	static void createInstance(SDL_Window *_window, VkInstance _instance) {
		if (enableValidationLayers && !validation::checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested but not available");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "SDL2/Vulkan";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Ray Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t extensionsCount = 0;
		SDL_Vulkan_GetInstanceExtensions(_window, &extensionsCount, nullptr);
		const char **extensions;
		extensions = new const char *[extensionsCount];
		bool value = SDL_Vulkan_GetInstanceExtensions(_window,
			&extensionsCount, extensions);

		for (int i = 0; i < extensionsCount; ++i) {
			std::cout << extensions[i] << std::endl;
		}

		createInfo.enabledExtensionCount = extensionsCount;
		createInfo.ppEnabledExtensionNames = extensions;
		createInfo.enabledLayerCount = 0; // validation layers

		VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
		if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}

		uint32_t eCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &eCount, nullptr);
		std::vector<VkExtensionProperties> ext(eCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &eCount, ext.data());

		std::cout << "EXT AVAILABLE" << std::endl;
		for (const auto &e : ext) {
			std::cout << e.extensionName << std::endl;
		}
	}
}