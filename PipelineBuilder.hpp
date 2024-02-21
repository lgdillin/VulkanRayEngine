#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "VkFuncs.hpp"

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
        auto bindingDescriptions = Vertex2::getBindingDescriptions();
        auto attributeDescriptions = Vertex2::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexAttributeDescriptionCount = 
            static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.vertexBindingDescriptionCount =
            static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    
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

    void setShaders(VkShaderModule _vertexShader, VkShaderModule _fragmentShader) {
        m_shaderStages.clear();

        m_shaderStages.push_back(
            vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, _vertexShader));

        m_shaderStages.push_back(
            vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, _fragmentShader));
    }

    void setInputTopology(VkPrimitiveTopology topology) {
        m_inputAssembly.topology = topology;
        // we are not going to use primitive restart on the entire tutorial so leave
        // it on false
        m_inputAssembly.primitiveRestartEnable = VK_FALSE;
    }

    void setPolygonMode(VkPolygonMode mode) {
        m_rasterizer.polygonMode = mode;
        m_rasterizer.lineWidth = 1.f;
    }

    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
        m_rasterizer.cullMode = cullMode;
        m_rasterizer.frontFace = frontFace;
    }

    void setMultisamplingNone()
    {
        m_multisampling.sampleShadingEnable = VK_FALSE;
        // multisampling defaulted to no multisampling (1 sample per pixel)
        m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_multisampling.minSampleShading = 1.0f;
        m_multisampling.pSampleMask = nullptr;
        //no alpha to coverage either
        m_multisampling.alphaToCoverageEnable = VK_FALSE;
        m_multisampling.alphaToOneEnable = VK_FALSE;
    }

    void disableBlending()
    {
        //default write mask
        m_colorBlendAttachment.colorWriteMask 
            = VK_COLOR_COMPONENT_R_BIT 
            | VK_COLOR_COMPONENT_G_BIT 
            | VK_COLOR_COMPONENT_B_BIT 
            | VK_COLOR_COMPONENT_A_BIT;
        //no blending
        m_colorBlendAttachment.blendEnable = VK_FALSE;
    }

    void setColorAttachmentFormat(VkFormat format)
    {
        m_colorAttachmentformat = format;
        //connect the format to the renderInfo  structure
        m_renderInfo.colorAttachmentCount = 1;
        m_renderInfo.pColorAttachmentFormats = &m_colorAttachmentformat;
    }

    void setDepthFormat(VkFormat format)
    {
        m_renderInfo.depthAttachmentFormat = format;
    }

    void disableDepthtest()
    {
        m_depthStencil.depthTestEnable = VK_FALSE;
        m_depthStencil.depthWriteEnable = VK_FALSE;
        m_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        m_depthStencil.depthBoundsTestEnable = VK_FALSE;
        m_depthStencil.stencilTestEnable = VK_FALSE;
        m_depthStencil.front = {};
        m_depthStencil.back = {};
        m_depthStencil.minDepthBounds = 0.f;
        m_depthStencil.maxDepthBounds = 1.f;
    }

    void enableBlendingAdditive() {
        // blend factor one
        // outColor = srcColor.rgb * 1.0 + dstColor.rgb * dstColor.a
        m_colorBlendAttachment.colorWriteMask 
            = VK_COLOR_COMPONENT_R_BIT 
            | VK_COLOR_COMPONENT_G_BIT 
            | VK_COLOR_COMPONENT_B_BIT 
            | VK_COLOR_COMPONENT_A_BIT;

        m_colorBlendAttachment.blendEnable = VK_TRUE;
        m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }


    void enableBlendingAlphaBlend() {
        // alpha-blend one
        // outColor = srcColor.rgb * (1.0 - dst.color.a) + dstColor.rgb * dstColor.a

        m_colorBlendAttachment.colorWriteMask 
            = VK_COLOR_COMPONENT_R_BIT 
            | VK_COLOR_COMPONENT_G_BIT 
            | VK_COLOR_COMPONENT_B_BIT 
            | VK_COLOR_COMPONENT_A_BIT;

        m_colorBlendAttachment.blendEnable = VK_TRUE;
        m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
};