#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>

namespace vkinit {
    static VkCommandPoolCreateInfo commandPoolCreateInfo(
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags /*= 0*/
    ) {
        VkCommandPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.queueFamilyIndex = queueFamilyIndex;
        info.flags = flags;
        return info;
    }

    static VkCommandBufferAllocateInfo commandBufferAllocateInfo(
        VkCommandPool pool, 
        uint32_t count /*= 1*/
    ) {
        VkCommandBufferAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext = nullptr;

        info.commandPool = pool;
        info.commandBufferCount = count;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        return info;
    }

    static VkFenceCreateInfo fenceCreateInfo(
        VkFenceCreateFlags flags /*= 0*/
    ) {
        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = nullptr;

        info.flags = flags;

        return info;
    }

    static VkSemaphoreCreateInfo semaphoreCreateInfo(
        VkSemaphoreCreateFlags flags /*= 0*/
    ) {
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;
        return info;
    }

    static VkCommandBufferBeginInfo commandBufferBeginInfo(
        VkCommandBufferUsageFlags flags /*= 0*/
    ) {
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.pNext = nullptr;

        info.pInheritanceInfo = nullptr;
        info.flags = flags;
        return info;
    }

    static VkImageSubresourceRange imageSubresourceRange(
        VkImageAspectFlags aspectMask
    ) {
        VkImageSubresourceRange subImage{};
        subImage.aspectMask = aspectMask;
        subImage.baseMipLevel = 0;
        subImage.levelCount = VK_REMAINING_MIP_LEVELS;
        subImage.baseArrayLayer = 0;
        subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

        return subImage;
    }

    static VkSemaphoreSubmitInfo semaphoreSubmitInfo(
        VkPipelineStageFlags2 stageMask, 
        VkSemaphore semaphore
    ) {
        VkSemaphoreSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.semaphore = semaphore;
        submitInfo.stageMask = stageMask;
        submitInfo.deviceIndex = 0;
        submitInfo.value = 1;

        return submitInfo;
    }

    static VkCommandBufferSubmitInfo commandBufferSubmitInfo(
        VkCommandBuffer cmd
    ) {
        VkCommandBufferSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        info.pNext = nullptr;
        info.commandBuffer = cmd;
        info.deviceMask = 0;

        return info;
    }

    static VkSubmitInfo2 submitInfo2(
        VkCommandBufferSubmitInfo *cmd, 
        VkSemaphoreSubmitInfo *signalSemaphoreInfo,
        VkSemaphoreSubmitInfo *waitSemaphoreInfo
    ) {
        VkSubmitInfo2 info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        info.pNext = nullptr;

        info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
        info.pWaitSemaphoreInfos = waitSemaphoreInfo;

        info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
        info.pSignalSemaphoreInfos = signalSemaphoreInfo;

        info.commandBufferInfoCount = 1;
        info.pCommandBufferInfos = cmd;

        return info;
    }

    static VkRenderPassBeginInfo renderpassBeginInfo(
        VkRenderPass renderPass, 
        VkExtent2D windowExtent, 
        VkFramebuffer framebuffer
    ) {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.pNext = nullptr;

        info.renderPass = renderPass;
        info.renderArea.offset.x = 0;
        info.renderArea.offset.y = 0;
        info.renderArea.extent = windowExtent;
        info.clearValueCount = 1;
        info.pClearValues = nullptr;
        info.framebuffer = framebuffer;

        return info;
    }

    static VkImageCreateInfo imageCreateInfo(
        VkFormat format, 
        VkImageUsageFlags usageFlags, 
        VkExtent3D extent
    ) {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext = nullptr;

        info.imageType = VK_IMAGE_TYPE_2D;

        info.format = format;
        info.extent = extent;

        info.mipLevels = 1;
        info.arrayLayers = 1;

        //for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
        info.samples = VK_SAMPLE_COUNT_1_BIT;

        //optimal tiling, which means the image is stored on the best gpu format
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usageFlags;

        return info;
    }

    static VkImageViewCreateInfo imageviewCreateInfo(
        VkFormat format, 
        VkImage image, 
        VkImageAspectFlags aspectFlags
    ) {
        // build a image-view for the depth image to use for rendering
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;

        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.image = image;
        info.format = format;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.aspectMask = aspectFlags;

        return info;
    }

    static VkRenderingAttachmentInfo attachmentInfo(
        VkImageView view,
        VkClearValue *clear,
        VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/
    ) {
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.pNext = nullptr;

        colorAttachment.imageView = view;
        colorAttachment.imageLayout = layout;
        colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (clear) {
            colorAttachment.clearValue = *clear;
        }

        return colorAttachment;
    }

    static VkRenderingInfo renderingInfo(
        VkExtent2D _renderExtent,
        VkRenderingAttachmentInfo *_colorAttachment,
        VkRenderingAttachmentInfo *_depthAttachment
    ) {
        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.pNext = nullptr;
        renderInfo.renderArea = VkRect2D{ VkOffset2D{0, 0}, _renderExtent };
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = _colorAttachment;
        renderInfo.pDepthAttachment = _depthAttachment;
        renderInfo.pStencilAttachment = nullptr;
        return renderInfo;
    }
}

namespace vkutil {
    static void transitionImage(
        VkCommandBuffer _cmd,
        VkImage _image,
        VkImageLayout _currentLayout,
        VkImageLayout _newLayout
    ) {
        VkImageMemoryBarrier2 imageBarrier{ 
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 
        };
        imageBarrier.pNext = nullptr;

        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask 
            = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = _currentLayout;
        imageBarrier.newLayout = _newLayout;

        VkImageAspectFlags aspectMask 
            = (_newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) 
            ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        imageBarrier.subresourceRange = vkinit::imageSubresourceRange(aspectMask);
        imageBarrier.image = _image;

        VkDependencyInfo depInfo{};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.pNext = nullptr;

        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(_cmd, &depInfo);
    }

    static void copyImageToImage(
        VkCommandBuffer cmd, 
        VkImage source, 
        VkImage destination, 
        VkExtent2D srcSize, 
        VkExtent2D dstSize
    ) {
        VkImageBlit2 blitRegion{ 
            .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, 
            .pNext = nullptr 
        };

        blitRegion.srcOffsets[1].x = srcSize.width;
        blitRegion.srcOffsets[1].y = srcSize.height;
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = dstSize.width;
        blitRegion.dstOffsets[1].y = dstSize.height;
        blitRegion.dstOffsets[1].z = 1;

        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.srcSubresource.mipLevel = 0;

        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount = 1;
        blitRegion.dstSubresource.mipLevel = 0;

        VkBlitImageInfo2 blitInfo{ 
            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, 
            .pNext = nullptr 
        };

        blitInfo.dstImage = destination;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage = source;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter = VK_FILTER_LINEAR;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &blitRegion;

        // alternate command is VkCmdCopyImage (faster).  However, copyimage requires
        // that the source and dest images are same resolution
        vkCmdBlitImage2(cmd, &blitInfo);
    }

    static bool loadShaderModule(
        const char *filePath,
        VkDevice device,
        VkShaderModule *outShaderModule
    ) {
        // open the file. With cursor at the end
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

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
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return false;
        }
        *outShaderModule = shaderModule;
        return true;
    }
}