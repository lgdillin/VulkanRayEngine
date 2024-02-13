// Ask the swapchain for the index of the swapchain image we can render onto
int image_index = request_image(mySwapchain);

// Create a new command buffer
VkCommandBuffer cmd = allocate_command_buffer();

// Initialize the command buffer
vkBeginCommandBuffer(cmd, ... );

// Start a new renderpass with the image index from swapchain as target to render onto
// Each framebuffer refers to a image in the swapchain
vkCmdBeginRenderPass(cmd, main_render_pass, framebuffers[image_index] );

// Rendering all objects
for(object in PassObjects){

    // Bind the shaders and configuration used to render the object
    vkCmdBindPipeline(cmd, object.pipeline);
    
    // Bind the vertex and index buffers for rendering the object
    vkCmdBindVertexBuffers(cmd, object.VertexBuffer,...);
    vkCmdBindIndexBuffer(cmd, object.IndexBuffer,...);

    // Bind the descriptor sets for the object (shader inputs)
    vkCmdBindDescriptorSets(cmd, object.textureDescriptorSet);
    vkCmdBindDescriptorSets(cmd, object.parametersDescriptorSet);

    // Execute drawing
    vkCmdDraw(cmd,...);
}

// Finalize the render pass and command buffer
vkCmdEndRenderPass(cmd);
vkEndCommandBuffer(cmd);


// Submit the command buffer to begin execution on GPU
vkQueueSubmit(graphicsQueue, cmd, ...);

// Display the image we just rendered on the screen
// renderSemaphore makes sure the image isn't presented until `cmd` is finished executing
vkQueuePresent(graphicsQueue, renderSemaphore);