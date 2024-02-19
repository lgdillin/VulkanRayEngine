#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

class PipelineBuilder {
public:
    PipelineBuilder() { clear(); }
	~PipelineBuilder() {}

    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
    VkPipelineRasterizationStateCreateInfo m_rasterizer;
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo m_multisampling;
    VkPipelineLayout m_pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo m_depthStencil;
    VkPipelineRenderingCreateInfo m_renderInfo;
    VkFormat m_colorAttachmentformat;

    void clear() {
        //clear all of the structs we need back to 0 with their correct stype	

        m_inputAssembly = { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

        m_rasterizer = { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

        m_colorBlendAttachment = {};

        m_multisampling = { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

        m_pipelineLayout = {};

        m_depthStencil = { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

        m_renderInfo = { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

        m_shaderStages.clear();
    }

    VkPipeline buildPipeline(VkDevice _device) {
        // make viewport state from our stored viewport and scissor.
    // at the moment we wont support multiple viewports or scissors
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.pNext = nullptr;

        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // setup dummy color blending. We arent using transparent objects yet
        // the blending is just "no blend", but we do write to the color attachment
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.pNext = nullptr;

        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &m_colorBlendAttachment;


        //completely clear VertexInputStateCreateInfo, as we have no need for it
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    
        // build the actual pipeline
        // we now use all of the info structs we have been writing into into this one
        // to create the pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo = { 
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        //connect the renderInfo to the pNext extension mechanism
        pipelineInfo.pNext = &m_renderInfo;

        pipelineInfo.stageCount = (uint32_t)m_shaderStages.size();
        pipelineInfo.pStages = m_shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &m_inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &m_rasterizer;
        pipelineInfo.pMultisampleState = &m_multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDepthStencilState = &m_depthStencil;
        pipelineInfo.layout = m_pipelineLayout;

        VkDynamicState state[] = { 
            VK_DYNAMIC_STATE_VIEWPORT, 
            VK_DYNAMIC_STATE_SCISSOR 
        };

        VkPipelineDynamicStateCreateInfo dynamicInfo = { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicInfo.pDynamicStates = &state[0];
        dynamicInfo.dynamicStateCount = 2;

        pipelineInfo.pDynamicState = &dynamicInfo;

        // its easy to error out on create graphics pipeline, so we handle it a bit
        // better than the common VK_CHECK case
        VkPipeline newPipeline;
        if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo,
            nullptr, &newPipeline)
            != VK_SUCCESS) {
            std::cout << "failed to create pipeline" << std::endl;
            return VK_NULL_HANDLE; // failed to create graphics pipeline
        }
        else {
            return newPipeline;
        }
    }
};