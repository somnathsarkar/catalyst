#include <catalyst/render/renderer.h>

namespace catalyst {
void Application::Renderer::CreateIlluminanceResources() {
  illuminance_buffers_.resize(frame_count_);
  illuminance_memory_.resize(frame_count_);
  VkDeviceSize illuminance_size =
      sizeof(glm::vec4) * swapchain_extent_.height * swapchain_extent_.width;
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    illuminance_buffers_[frame_i].resize(2);
    illuminance_memory_[frame_i].resize(2);
    for (uint32_t buff_i = 0; buff_i < 2; buff_i++) {
      CreateBuffer(
          illuminance_buffers_[frame_i][buff_i],
          illuminance_memory_[frame_i][buff_i], illuminance_size,
          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
  }
}
void Application::Renderer::CreateIlluminancePipeline() {
  const std::vector<char> compute_shader_code =
      ReadFile("../assets/shaders/log_illuminance.comp.spv");

  VkShaderModule compute_shader = CreateShaderModule(compute_shader_code);

  uint32_t shader_constants[] = {compute_details_.subgroup_size,
                                 compute_details_.workgroup_size};
  VkSpecializationMapEntry constant_map[] = {
      {0, 0, sizeof(uint32_t)}, {1, sizeof(uint32_t), sizeof(uint32_t)}};
  VkSpecializationInfo constant_info{};
  constant_info.mapEntryCount = 2;
  constant_info.pMapEntries = constant_map;
  constant_info.dataSize = sizeof(shader_constants);
  constant_info.pData = shader_constants;

  VkPipelineShaderStageCreateInfo shader_ci{};
  shader_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_ci.pNext = nullptr;
  shader_ci.flags = 0;
  shader_ci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shader_ci.module = compute_shader;
  shader_ci.pName = "main";
  shader_ci.pSpecializationInfo = &constant_info;

  VkPushConstantRange push_constant{};
  push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  push_constant.offset = 0;
  push_constant.size = sizeof(ComputePushConstantData);
  
  VkPipelineLayoutCreateInfo layout_ci{};
  layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_ci.pNext = nullptr;
  layout_ci.flags = 0;
  layout_ci.pushConstantRangeCount = 1;
  layout_ci.pPushConstantRanges = &push_constant;
  layout_ci.setLayoutCount = 1;
  layout_ci.pSetLayouts = &illuminance_descriptor_set_layout_;
  VkResult create_result = vkCreatePipelineLayout(
      device_, &layout_ci, nullptr, &illuminance_pipeline_layout_);
  ASSERT(create_result == VK_SUCCESS, "Could not create illuminance pipeline layout!");

  VkComputePipelineCreateInfo pipeline_ci{};
  pipeline_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_ci.pNext = nullptr;
  pipeline_ci.flags = 0;
  pipeline_ci.stage = shader_ci;
  pipeline_ci.layout = illuminance_pipeline_layout_;
  pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_ci.basePipelineIndex = 0;
  create_result =
      vkCreateComputePipelines(device_, pipeline_cache_, 1, &pipeline_ci,
                               nullptr, &illuminance_pipeline_);
  ASSERT(create_result == VK_SUCCESS, "Could not create illuminance pipeline!");
  vkDestroyShaderModule(device_, compute_shader, nullptr);
}
void Application::Renderer::CalculateAverageIlluminance(
    VkCommandBuffer& cmd, uint32_t swapchain_image_i) {
  // Step 1: Copy HDR buffer contents to Illuminance storage buffer
  VkBufferImageCopy copy_info{};
  copy_info.bufferOffset = 0;
  copy_info.bufferRowLength = 0;
  copy_info.bufferImageHeight = 0;
  copy_info.imageOffset = {0, 0, 0};
  copy_info.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_info.imageSubresource.mipLevel = 0;
  copy_info.imageSubresource.baseArrayLayer = 0;
  copy_info.imageSubresource.layerCount = 1;
  copy_info.imageExtent = {swapchain_extent_.width, swapchain_extent_.height,
                           1};
  vkCmdCopyImageToBuffer(
      cmd, hdr_images_[swapchain_image_i], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      illuminance_buffers_[swapchain_image_i][0], 1, &copy_info);
  uint32_t num_pixels = swapchain_extent_.width * swapchain_extent_.height;
  uint32_t num_dispatches = (num_pixels + compute_details_.workgroup_size - 1) /
                            compute_details_.workgroup_size;
  // Step 2: Wait for copy to finish and make new data available to compute shader
  VkBufferMemoryBarrier buff_barrier{};
  buff_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  buff_barrier.pNext = nullptr;
  buff_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  buff_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  buff_barrier.srcQueueFamilyIndex =
      queue_family_indices_.graphics_queue_index_.value();
  buff_barrier.dstQueueFamilyIndex =
      queue_family_indices_.graphics_queue_index_.value();
  buff_barrier.offset = 0;
  buff_barrier.buffer = illuminance_buffers_[swapchain_image_i][0];
  buff_barrier.size = VK_WHOLE_SIZE;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                       &buff_barrier, 0, nullptr);
  // Step 3: Dispatch Log Illuminance Compute Shader
  ComputePushConstantData pc_data{};
  pc_data.input_dim = num_pixels;
  pc_data.input0 = true;
  vkCmdPushConstants(cmd, illuminance_pipeline_layout_,
                     VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(ComputePushConstantData), &pc_data);
  vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_COMPUTE, illuminance_pipeline_layout_, 0, 1,
      &illuminance_descriptor_sets_[swapchain_image_i], 0, nullptr);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, illuminance_pipeline_);
  vkCmdDispatch(cmd, num_dispatches, 1, 1);
  // Step 4: Transition HDR Image to Shader Read Only
  VkImageMemoryBarrier hdr_barrier{};
  hdr_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  hdr_barrier.pNext = nullptr;
  hdr_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  hdr_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  hdr_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  hdr_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  hdr_barrier.srcQueueFamilyIndex =
      queue_family_indices_.graphics_queue_index_.value();
  hdr_barrier.dstQueueFamilyIndex =
      queue_family_indices_.graphics_queue_index_.value();
  hdr_barrier.image = hdr_images_[swapchain_image_i];
  hdr_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  hdr_barrier.subresourceRange.baseMipLevel = 0;
  hdr_barrier.subresourceRange.levelCount = 1;
  hdr_barrier.subresourceRange.baseArrayLayer = 0;
  hdr_barrier.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1,
                       &hdr_barrier);
}
}  // namespace catalyst