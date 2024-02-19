#pragma once

#include <vector>
#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <vma/vk_mem_alloc.h>

#include "Game.hpp"
#include "VkBootstrap.h"
#include "VideoSettings.hpp"
#include "Structs.hpp"
#include "VkFuncs.hpp"
#include "PipelineBuilder.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

class View {
public:
	View(Game &_game);
	~View();

	void draw();
	void drawGeometry(VkCommandBuffer _cmd);
	void drawBackground();
	void drawImgui(VkCommandBuffer _cmd, VkImageView _targetImageView);

	void initialize();

	// project
	SDL_Window *getWindow() { return m_window; }
	Game *m_game;
	bool m_initialized;
	bool m_stopRendering;

	// sdl
	SDL_Window *m_window;
	VkExtent2D m_windowExtent{ WINDOW_WIDTH, WINDOW_HEIGHT };

	// 
	// Core
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkSurfaceKHR m_surface;

	//
	// Swapchain
	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchainImageFormat;
	std::vector<VkImage> m_swapchainImages; // VkImage is the texture we render onto
	std::vector<VkImageView> m_swapchainImageViews; // VkImageView is wrapper for VkImage
	VkExtent2D m_swapchainExtent;

	//
	// command
	FrameData &getCurrentFrame() { return m_frames[m_frameNumber % FRAME_OVERLAP]; }
	uint32_t m_frameNumber;
	FrameData m_frames[FRAME_OVERLAP];
	VkQueue m_graphicsQueue;
	uint32_t m_graphicsQueueFamily;
	DeletionQueue m_deletionQueue;

	// 
	// memory
	VmaAllocator m_allocator;
	AllocatedImage m_drawImage;
	VkExtent2D m_drawExtent;

	// 
	// descriptors
	DescriptorAllocator m_globalDescriptorAllocator;
	VkDescriptorSet m_drawImageDescriptors;
	VkDescriptorSetLayout m_drawImageDescriptorLayout;

	// 
	// pipeline
	VkPipeline m_gradientPipeline;
	VkPipelineLayout m_gradientPipelineLayout;

	// 
	// Immediate submit structures
	VkFence m_immFence;
	VkCommandBuffer m_immCommandBuffer;
	VkCommandPool m_immCommandPool;

	// 
	// imgui stuff
	void imguiEventProcessing(SDL_Event *_event) {ImGui_ImplSDL2_ProcessEvent(_event);}
	std::vector<ComputeEffect> m_backgroundEffects;
	int m_currentBackgroundEffect{ 0 };
private:
	// core
	void initVulkan();

	// swapchain
	void initSwapchain();
	void createSwapchain(uint32_t _width, uint32_t _height);
	void destroySwapchain();
	void initCommands();
	void initSyncStructures();

	// descriptors
	void initDescriptors();

	// pipeline
	void initPipelines();
	void initBackgroundPipelines();

	// immediate submit
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&_function);
	void initImgui();

	// allocation
	AllocatedBuffer createBuffer(size_t _allocSize, VkBufferUsageFlags _usage, VmaMemoryUsage _memoryUsage);
	void destroyBuffer(const AllocatedBuffer &_buffer) {vmaDestroyBuffer(m_allocator, _buffer.buffer, _buffer.allocation);}

	// mesh
	GPUMeshBuffers uploadMesh(std::span<uint32_t> _indices, std::span<Vertex> _vertices);

	// cleanup
	void cleanup();

	// triangle example
	VkPipelineLayout m_trianglePipelineLayout;
	VkPipeline m_trianglePipeline;
	void initTriangle() {
		VkShaderModule triangleFragShader, triangleVertexShader;
		vkutil::loadShaderModule("./color_triangle.frag.spv",
			m_device, &triangleFragShader);
		vkutil::loadShaderModule("./color_triangle.vert.spv",
			m_device, &triangleVertexShader);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo
			= vkinit::pipelineLayoutCreateInfo();
		VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo,
			nullptr, &m_trianglePipelineLayout));

		PipelineBuilder pipelineBuilder;

		//use the triangle layout we created
		pipelineBuilder.m_pipelineLayout = m_trianglePipelineLayout;
		//connecting the vertex and pixel shaders to the pipeline
		pipelineBuilder.setShaders(triangleVertexShader, triangleFragShader);
		//it will draw triangles
		pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		//filled triangles
		pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
		//no backface culling
		pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
		//no multisampling
		pipelineBuilder.setMultisamplingNone();
		//no blending
		pipelineBuilder.disableBlending();
		//no depth testing
		pipelineBuilder.disableDepthtest();

		//connect the image format we will draw into, from draw image
		pipelineBuilder.setColorAttachmentFormat(m_drawImage.imageFormat);
		pipelineBuilder.setDepthFormat(VK_FORMAT_UNDEFINED);

		//finally build the pipeline
		m_trianglePipeline = pipelineBuilder.buildPipeline(m_device);

		//clean structures
		vkDestroyShaderModule(m_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(m_device, triangleVertexShader, nullptr);

		m_deletionQueue.push_function([&]() {
			vkDestroyPipelineLayout(m_device, m_trianglePipelineLayout, nullptr);
			vkDestroyPipeline(m_device, m_trianglePipeline, nullptr);
			});
	}

	// mesh example
	VkPipelineLayout m_meshPipelineLayout;
	VkPipeline m_meshPipeline;
	GPUMeshBuffers m_rectangle;
	void initMeshPipeline() {
		VkShaderModule triangleFragShader;
		vkutil::loadShaderModule("./color_triangle.frag.spv",
			m_device, &triangleFragShader);

		VkShaderModule triangleVertexShader;
		vkutil::loadShaderModule("./color_triangle_mesh.vert.spv",
			m_device, &triangleVertexShader);

		VkPushConstantRange bufferRange{};
		bufferRange.offset = 0;
		bufferRange.size = sizeof(GPUDrawPushConstants);
		bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipelineLayoutCreateInfo();
		pipeline_layout_info.pPushConstantRanges = &bufferRange;
		pipeline_layout_info.pushConstantRangeCount = 1;

		VK_CHECK(vkCreatePipelineLayout(m_device, &pipeline_layout_info, 
			nullptr, &m_meshPipelineLayout));

		PipelineBuilder pipelineBuilder;

		//use the triangle layout we created
		pipelineBuilder.m_pipelineLayout = m_meshPipelineLayout;
		//connecting the vertex and pixel shaders to the pipeline
		pipelineBuilder.setShaders(triangleVertexShader, triangleFragShader);
		//it will draw triangles
		pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		//filled triangles
		pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
		//no backface culling
		pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
		//no multisampling
		pipelineBuilder.setMultisamplingNone();
		//no blending
		pipelineBuilder.disableBlending();

		pipelineBuilder.disableDepthtest();

		//connect the image format we will draw into, from draw image
		pipelineBuilder.setColorAttachmentFormat(m_drawImage.imageFormat);
		pipelineBuilder.setDepthFormat(VK_FORMAT_UNDEFINED);

		//finally build the pipeline
		m_meshPipeline = pipelineBuilder.buildPipeline(m_device);

		//clean structures
		vkDestroyShaderModule(m_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(m_device, triangleVertexShader, nullptr);

		m_deletionQueue.push_function([&]() {
			vkDestroyPipelineLayout(m_device, m_meshPipelineLayout, nullptr);
			vkDestroyPipeline(m_device, m_meshPipeline, nullptr);
			});
	}

	void initDefaultData() {
		std::array<Vertex, 4> rect_vertices;

		rect_vertices[0].position = { 0.5,-0.5, 0 };
		rect_vertices[1].position = { 0.5,0.5, 0 };
		rect_vertices[2].position = { -0.5,-0.5, 0 };
		rect_vertices[3].position = { -0.5,0.5, 0 };

		rect_vertices[0].color = { 0,0, 0,1 };
		rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
		rect_vertices[2].color = { 1,0, 0,1 };
		rect_vertices[3].color = { 0,1, 0,1 };

		std::array<uint32_t, 6> rect_indices;

		rect_indices[0] = 0;
		rect_indices[1] = 1;
		rect_indices[2] = 2;

		rect_indices[3] = 2;
		rect_indices[4] = 1;
		rect_indices[5] = 3;

		m_rectangle = uploadMesh(rect_indices, rect_vertices);
	}

public:
	void newFrame();
};