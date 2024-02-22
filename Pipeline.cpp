#include "Pipeline.hpp"

vre::Pipeline::Pipeline(
    vre::VreDevice &_device,
    const PipelineConfigInfo &_info, 
    const std::string &_vertexFile,
    const std::string &_fragFile) : m_device(_device) {
    createGraphicsPipeline(_vertexFile, _fragFile, _info);
}

vre::Pipeline::~Pipeline() {
}

vre::PipelineConfigInfo vre::Pipeline::defaultPipelineConfigInfo(
    uint32_t _width, uint32_t _height
) {
    PipelineConfigInfo info{};
    return info;
}


void vre::Pipeline::createGraphicsPipeline(
    const std::string &_vertexFile,
    const std::string &_fragFile,
    const PipelineConfigInfo &_info
) {
    auto vertCode = readFile(_vertexFile);
    auto fragCode = readFile(_fragFile);

    std::cout << "vertex " << vertCode.size() << std::endl;
    std::cout << "frag " << fragCode.size() << std::endl;
}

void vre::Pipeline::createShaderModule(
    const std::vector<char> &_code, 
    VkShaderModule *_shaderModule
) {
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = _code.size();
    info.pCode = reinterpret_cast<const uint32_t *>(_code.data());

    if (vkCreateShaderModule(m_device.device(), &info, nullptr, _shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
}

std::vector<char> vre::Pipeline::readFile(const std::string &_file) {
    std::ifstream file(_file, std::ios::ate | std::ios::binary);

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
    file.read(buffer.data(), fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();
    return buffer;
}
