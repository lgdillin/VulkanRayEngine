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
}