#include "VrePipeline.hpp"

vre::VrePipeline::VrePipeline(
    VkDevice &_device,
    vre::PipelineConfigInfo _config,
    const std::string _vertexFile,
    const std::string _fragFile) : m_device(_device) {

    m_vShader = nullptr;
    m_fShader = nullptr;

    createGraphicsPipeline(_vertexFile, _fragFile, _config);
}

vre::VrePipeline::~VrePipeline() {
    vkDestroyShaderModule(m_device, m_vShader, nullptr);
    vkDestroyShaderModule(m_device, m_vShader, nullptr);
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
}

void vre::VrePipeline::bind(VkCommandBuffer _commandBuffer) {
    // VK_PIPELINE_BIND_POINT_GRAPHICS specifies this pipeline as a graphics pipeline\
    // there are also VK_PIPELINE_BIND_POINT_COMPUTE for a comptue pipeline
    // VK_PIPELINE_BIND_POINT_RAY_TRACING for ray tracing
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
}

vre::PipelineConfigInfo vre::VrePipeline::defaultPipelineConfigInfo(
    uint32_t _width, uint32_t _height
) {
    vre::PipelineConfigInfo info{};
    info.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // every 3 vertices are grouped with this flag
    info.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // break up a triangle strip using a special index value (if true)
    info.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // viewport describes the transformation between our output and the target image
    info.viewport.x = 0.0f;
    info.viewport.y = 0.0f;
    info.viewport.width = static_cast<float>(_width);
    // example: if we multiply this by 0.5, the output image will be squished 
    // into the top half of the screen
    info.viewport.height = static_cast<float>(_height);
    info.viewport.minDepth = 0.0f;
    info.viewport.maxDepth = 1.0f;

    // like viewport, but cuts instead of squishes
    // so if we multiply by 0.5, only the top half of the screen will be drawn
    info.scissor.offset = { 0, 0 };
    info.scissor.extent = { _width, _height };

    info.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // forces the z component of gl_position to be 0 <= z <= 1
    info.rasterizationInfo.depthClampEnable = VK_FALSE;
    // discards all primitive before rasterization, so you only use this 
    // in situations where you only want to use the first few stages of the 
    // graphicps pipeline
    info.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    // draw corners, edges, or the whole triangle filled in?
    info.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    info.rasterizationInfo.lineWidth = 1.0f;
    info.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    info.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    info.rasterizationInfo.depthBiasEnable = VK_FALSE;
    info.rasterizationInfo.depthBiasConstantFactor = 0.0f; // optional
    info.rasterizationInfo.depthBiasClamp = 0.0f; // optional
    info.rasterizationInfo.depthBiasSlopeFactor = 0.0f; // optional

    info.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.multisampleInfo.sampleShadingEnable = VK_FALSE;
    info.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info.multisampleInfo.minSampleShading = 1.0f; // optional
    info.multisampleInfo.pSampleMask = nullptr; // optional
    info.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // optional
    info.multisampleInfo.alphaToOneEnable = VK_FALSE; // optional

    info.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    info.colorBlendAttachment.blendEnable = VK_FALSE;
    info.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // optional
    info.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
    info.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // optional
    info.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // optional
    info.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
    info.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // optional

    info.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info.colorBlendInfo.logicOpEnable = VK_FALSE;
    info.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // optional
    info.colorBlendInfo.attachmentCount = 1;
    info.colorBlendInfo.pAttachments = &info.colorBlendAttachment;
    info.colorBlendInfo.blendConstants[0] = 0.0f; // optional
    info.colorBlendInfo.blendConstants[1] = 0.0f; // optional
    info.colorBlendInfo.blendConstants[2] = 0.0f; // optional
    info.colorBlendInfo.blendConstants[3] = 0.0f; // optional

    info.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.depthStencilInfo.depthTestEnable = VK_TRUE;
    info.depthStencilInfo.depthWriteEnable = VK_TRUE;
    info.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    info.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    info.depthStencilInfo.minDepthBounds = 0.0f; //optional
    info.depthStencilInfo.maxDepthBounds = 1.0f; // optional
    info.depthStencilInfo.stencilTestEnable = VK_FALSE;
    info.depthStencilInfo.front = {}; // optional
    info.depthStencilInfo.back = {}; // optional

    return info;
}

void vre::VrePipeline::createGraphicsPipeline(
    const std::string &_vertexFile,
    const std::string &_fragFile,
    const PipelineConfigInfo &_info
) {
    assert(_info.pipelineLayout != VK_NULL_HANDLE 
        && "Cannot create graphics pipeline, no pipelineLayout provided in configInfo");
    assert(_info.renderPass != VK_NULL_HANDLE
        && "Cannot create graphics pipeline, no renderpass provided in configInfo");
    
    //std::vector<char> vertCode = readFile(_vertexFile);
    //std::vector<char> fragCode = readFile(_fragFile);
    //
    //std::cout << vertCode.size() << std::endl;
    //std::cout << fragCode.size() << std::endl;

    createShaderModule(_vertexFile, &m_vShader);
    createShaderModule(_fragFile, &m_fShader);

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = m_vShader;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = m_fShader;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    // how we interpret the vertex data
    VkPipelineVertexInputStateCreateInfo vertexInfo{};
    vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInfo.vertexAttributeDescriptionCount = 0;
    vertexInfo.vertexBindingDescriptionCount = 0;
    vertexInfo.pVertexAttributeDescriptions = nullptr;
    vertexInfo.pVertexBindingDescriptions = nullptr;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &_info.viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &_info.scissor;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInfo;
    pipelineInfo.pInputAssemblyState = &_info.inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &_info.rasterizationInfo;
    pipelineInfo.pMultisampleState = &_info.multisampleInfo;
    pipelineInfo.pColorBlendState = &_info.colorBlendInfo;
    pipelineInfo.pDepthStencilState = &_info.depthStencilInfo;
    pipelineInfo.pDynamicState = nullptr;

    pipelineInfo.layout = _info.pipelineLayout;
    pipelineInfo.renderPass = _info.renderPass;
    pipelineInfo.subpass = _info.subpass;

    // in some cases this can improve performance. it can be less expensive for
    // a gpu to create a new graphics pipeline by driving from an existing one
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
        nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline");
    }


}

bool vre::VrePipeline::createShaderModule(
    std::string _filePath,
    VkShaderModule *_outShaderModule
) {
    // open the file. With cursor at the end
    std::ifstream file(_filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();

    // spirv expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    file.read((char *)buffer.data(), fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *_outShaderModule = shaderModule;
    return true;
}

std::vector<char> vre::VrePipeline::readFile(const std::string &_file) {
    std::ifstream file{ _file, std::ios::ate | std::ios::binary };

    if (!file.is_open()) {
        throw std::runtime_error(_file + " could not be opened\n");
    }

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    file.read((char *)buffer.data(), fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();


    return buffer;
}
