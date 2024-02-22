#include "Pipeline.hpp"

vre::Pipeline::Pipeline(
    VkDevice &_device, 
    const PipelineConfigInfo &_info
) : m_device(_device) {

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
