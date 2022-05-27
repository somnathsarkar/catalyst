#include <catalyst/render/renderer.h>

#include <random>

namespace catalyst {
void Application::Renderer::CreateSsaoResources() {
  ssao_format_ = VK_FORMAT_R8G8B8A8_UNORM;
  ssn_format_ = VK_FORMAT_R8G8B8A8_UNORM;

  // Create Screen-Space Noise Resources
  CreateImage(ssn_image_, ssn_memory_, 0, ssn_format_,
              {half_swapchain_extent_.width, half_swapchain_extent_.height, 1},
              1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  CreateImageView(ssn_image_view_, ssn_image_, VK_IMAGE_VIEW_TYPE_2D,
                  ssn_format_, VK_IMAGE_ASPECT_COLOR_BIT);
  TransitionImageLayout(
      ssn_image_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
  VkDeviceSize ssn_size =
      sizeof(glm::vec3) * half_swapchain_extent_.width * half_swapchain_extent_.height;

  // Create Staging Buffer
  VkBuffer staging_buffer;
  VkDeviceMemory staging_memory;
  CreateBuffer(staging_buffer, staging_memory, ssn_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Generate Screen Space Noise Data
  std::uniform_real_distribution<float> random_float(0.0f, 1.0f);
  std::default_random_engine random_gen;
  std::vector<glm::vec3> noise_vec;
  for (uint32_t row_i = 0; row_i < half_swapchain_extent_.height; row_i++) {
    for (uint32_t col_i = 0; col_i < half_swapchain_extent_.width; col_i++) {
      glm::vec3 noise = glm::vec3(random_float(random_gen) * 2.0f - 1.0f,
                                  random_float(random_gen) * 2.0f - 1.0f,
                                  random_float(random_gen) * 2.0f - 1.0f);
      noise = glm::normalize(noise);
      noise_vec.push_back(noise);
    }
  }

  // Copy Noise Data to Staging Buffer
  void* staging_data;
  vkMapMemory(device_, staging_memory, 0, VK_WHOLE_SIZE, 0, &staging_data);
  memcpy(staging_data, noise_vec.data(), ssn_size);
  vkUnmapMemory(device_, staging_memory);

  // Copy Staging Buffer to SSN image
  VkCommandBuffer cmd;
  BeginSingleUseCommandBuffer(cmd);
  VkBufferImageCopy buffer_cp{};
  buffer_cp.bufferOffset = 0;
  buffer_cp.bufferRowLength = 0;
  buffer_cp.bufferImageHeight = 0;
  buffer_cp.imageOffset = {0, 0, 0};
  buffer_cp.imageExtent = {half_swapchain_extent_.width,
                           half_swapchain_extent_.height, 1};
  buffer_cp.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  buffer_cp.imageSubresource.baseArrayLayer = 0;
  buffer_cp.imageSubresource.layerCount = 1;
  buffer_cp.imageSubresource.mipLevel = 0;
  vkCmdCopyBufferToImage(cmd, staging_buffer, ssn_image_,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_cp);
  EndSingleUseCommandBuffer(cmd);
  // Destroy Staging Buffer
  vkFreeMemory(device_,staging_memory, nullptr);
  vkDestroyBuffer(device_,staging_buffer, nullptr);
  TransitionImageLayout(
      ssn_image_, VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_SHADER_READ_BIT);

  // Generate Sample Data
  static constexpr uint32_t kSsaoSamples = 64;
  VkDeviceSize sample_size = kSsaoSamples * sizeof(glm::vec4);
  glm::vec4 samples[kSsaoSamples];
  for (uint32_t sample_i = 0; sample_i < kSsaoSamples; sample_i++) {
    glm::vec3 noise = glm::vec3(random_float(random_gen) * 0.7f - 0.35f,
                                random_float(random_gen) * 0.7f - 0.35f,
                                random_float(random_gen));
    samples[sample_i] = glm::vec4(glm::normalize(noise),0.0f);
  }

  // Create Staging Buffer
  CreateBuffer(staging_buffer, staging_memory, sample_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Create SSAO Sample Uniform Buffer
  CreateBuffer(
      ssao_sample_uniform_, ssao_sample_memory_, sample_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // Copy Sample Data to Staging Buffer
  vkMapMemory(device_, staging_memory, 0, VK_WHOLE_SIZE, 0, &staging_data);
  memcpy(staging_data, samples, sample_size);
  vkUnmapMemory(device_, staging_memory);

  // Copy Staging Buffer to Sample Uniform
  BeginSingleUseCommandBuffer(cmd);
  VkBufferCopy sample_cp{};
  sample_cp.srcOffset = 0;
  sample_cp.dstOffset = 0;
  sample_cp.size = sample_size;
  vkCmdCopyBuffer(cmd, staging_buffer, ssao_sample_uniform_, 1, &sample_cp);
  EndSingleUseCommandBuffer(cmd);

  // Destroy Staging Buffer
  vkFreeMemory(device_, staging_memory, nullptr);
  vkDestroyBuffer(device_, staging_buffer, nullptr);

  ssao_images_.resize(frame_count_);
  ssao_image_views_.resize(frame_count_);
  ssao_memory_.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    CreateImage(ssao_images_[frame_i], ssao_memory_[frame_i], 0,
                ssao_format_,
                {half_swapchain_extent_.width, half_swapchain_extent_.height, 1},
                1, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CreateImageView(ssao_image_views_[frame_i], ssao_images_[frame_i],
                    VK_IMAGE_VIEW_TYPE_2D, ssao_format_,
                    VK_IMAGE_ASPECT_COLOR_BIT);
  }
}
void Application::Renderer::CreateSsaoRenderPass() {
  VkAttachmentDescription ssao_attachment{};
  ssao_attachment.flags = 0;
  ssao_attachment.format = ssao_format_;
  ssao_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  ssao_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  ssao_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  ssao_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  ssao_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  ssao_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ssao_attachment.finalLayout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference ssao_ref{};
  ssao_ref.attachment = 0;
  ssao_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &ssao_ref;
  subpass.pResolveAttachments = nullptr;
  subpass.pDepthStencilAttachment = nullptr;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = nullptr;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = 0;
  dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo render_pass_ci{};
  render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_ci.pNext = nullptr;
  render_pass_ci.flags = 0;
  render_pass_ci.attachmentCount = 1;
  render_pass_ci.pAttachments = &ssao_attachment;
  render_pass_ci.subpassCount = 1;
  render_pass_ci.pSubpasses = &subpass;
  render_pass_ci.dependencyCount = 1;
  render_pass_ci.pDependencies = &dependency;
  VkResult create_result = vkCreateRenderPass(device_, &render_pass_ci, nullptr,
                                              &ssao_render_pass_);
  ASSERT(create_result == VK_SUCCESS, "Could not create SSAO render pass!");
}
void Application::Renderer::CreateSsaoPipeline() {
  const std::vector<char> vert_shader_code =
      ReadFile("../assets/shaders/ssao.vert.spv");
  const std::vector<char> frag_shader_code =
      ReadFile("../assets/shaders/ssao.frag.spv");

  VkShaderModule vert_shader = CreateShaderModule(vert_shader_code);
  VkShaderModule frag_shader = CreateShaderModule(frag_shader_code);

  VkPipelineShaderStageCreateInfo vert_shader_stage_ci{};
  vert_shader_stage_ci.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_ci.pNext = nullptr;
  vert_shader_stage_ci.flags = 0;
  vert_shader_stage_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_ci.module = vert_shader;
  vert_shader_stage_ci.pName = "main";
  vert_shader_stage_ci.pSpecializationInfo = nullptr;

  VkPipelineShaderStageCreateInfo frag_shader_stage_ci{};
  frag_shader_stage_ci.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_ci.pNext = nullptr;
  frag_shader_stage_ci.flags = 0;
  frag_shader_stage_ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_ci.module = frag_shader;
  frag_shader_stage_ci.pName = "main";
  frag_shader_stage_ci.pSpecializationInfo = nullptr;

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_ci,
                                                     frag_shader_stage_ci};

  VkVertexInputBindingDescription vertex_bd{};
  vertex_bd.binding = 0;
  vertex_bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertex_bd.stride = 3 * 4;

  VkVertexInputAttributeDescription vertex_pos_ad{};
  vertex_pos_ad.binding = 0;
  vertex_pos_ad.format = VK_FORMAT_R32G32B32_SFLOAT;
  vertex_pos_ad.location = 0;
  vertex_pos_ad.offset = 0;

  VkVertexInputAttributeDescription vertex_ads[] = {
      vertex_pos_ad,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions = &vertex_bd;
  vertex_input_info.pVertexAttributeDescriptions = &vertex_pos_ad;

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
  input_assembly_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_state.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)half_swapchain_extent_.width;
  viewport.height = (float)half_swapchain_extent_.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = half_swapchain_extent_;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer_state{};
  rasterizer_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer_state.depthClampEnable = VK_FALSE;
  rasterizer_state.rasterizerDiscardEnable = VK_FALSE;
  rasterizer_state.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer_state.lineWidth = 1.0f;
  rasterizer_state.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer_state.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisample_state{};
  multisample_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_state.sampleShadingEnable = VK_FALSE;
  multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
  depth_stencil_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_state.depthTestEnable = VK_FALSE;
  depth_stencil_state.depthWriteEnable = VK_FALSE;
  depth_stencil_state.depthCompareOp = VK_COMPARE_OP_ALWAYS;
  depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_state.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blend_state{};
  color_blend_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_state.logicOpEnable = VK_FALSE;
  color_blend_state.logicOp = VK_LOGIC_OP_COPY;
  color_blend_state.attachmentCount = 1;
  color_blend_state.pAttachments = &color_blend_attachment;
  color_blend_state.blendConstants[0] = 0.0f;
  color_blend_state.blendConstants[1] = 0.0f;
  color_blend_state.blendConstants[2] = 0.0f;
  color_blend_state.blendConstants[3] = 0.0f;

  VkPushConstantRange vertex_push{};
  vertex_push.offset = 0;
  vertex_push.size = sizeof(PushConstantData);
  vertex_push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo layout_ci{};
  layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_ci.pNext = nullptr;
  layout_ci.flags = 0;
  layout_ci.pushConstantRangeCount = 1;
  layout_ci.pPushConstantRanges = &vertex_push;
  layout_ci.setLayoutCount = 1;
  layout_ci.pSetLayouts = &ssao_descriptor_set_layout_;
  VkResult result = vkCreatePipelineLayout(device_, &layout_ci, nullptr,
                                           &ssao_pipeline_layout_);
  ASSERT(result == VK_SUCCESS, "Failed to create SSAO pipeline layout!");

  VkGraphicsPipelineCreateInfo pipeline_ci{};
  pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_ci.stageCount = 2;
  pipeline_ci.pStages = shader_stages;
  pipeline_ci.pVertexInputState = &vertex_input_info;
  pipeline_ci.pInputAssemblyState = &input_assembly_state;
  pipeline_ci.pViewportState = &viewport_state;
  pipeline_ci.pRasterizationState = &rasterizer_state;
  pipeline_ci.pMultisampleState = &multisample_state;
  pipeline_ci.pDepthStencilState = &depth_stencil_state;
  pipeline_ci.pColorBlendState = &color_blend_state;
  pipeline_ci.layout = ssao_pipeline_layout_;
  pipeline_ci.renderPass = ssao_render_pass_;
  pipeline_ci.subpass = 0;
  pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;

  VkResult create_result = vkCreateGraphicsPipelines(
      device_, pipeline_cache_, 1, &pipeline_ci, nullptr, &ssao_pipeline_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create SSAO pipeline!");

  vkDestroyShaderModule(device_, vert_shader, nullptr);
  vkDestroyShaderModule(device_, frag_shader, nullptr);
}

void Application::Renderer::CreateSsaoFramebuffers() {
  ssao_framebuffers_.resize(frame_count_);
  VkFramebufferCreateInfo framebuffer_ci{};
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    VkFramebufferCreateInfo framebuffer_ci{};
    framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.pNext = nullptr;
    framebuffer_ci.flags = 0;
    framebuffer_ci.renderPass = ssao_render_pass_;
    framebuffer_ci.attachmentCount = 1;
    framebuffer_ci.pAttachments = &ssao_image_views_[frame_i];
    framebuffer_ci.width = half_swapchain_extent_.width;
    framebuffer_ci.height = half_swapchain_extent_.height;
    framebuffer_ci.layers = 1;
    VkResult create_result = vkCreateFramebuffer(
        device_, &framebuffer_ci, nullptr, &ssao_framebuffers_[frame_i]);
    ASSERT(create_result == VK_SUCCESS,
           "Could not create SSAO framebuffer!");
  }
}

void Application::Renderer::BeginSsaoRenderPass(VkCommandBuffer& cmd,
                                                uint32_t swapchain_image_i) {
  VkClearValue color_clear;
  color_clear.color.float32[0] = 0.0f;
  VkRenderPassBeginInfo render_pass_bi{};
  render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_bi.pNext = nullptr;
  render_pass_bi.renderPass = ssao_render_pass_;
  render_pass_bi.framebuffer = ssao_framebuffers_[swapchain_image_i];
  render_pass_bi.renderArea.extent = half_swapchain_extent_;
  render_pass_bi.renderArea.offset.x = 0;
  render_pass_bi.renderArea.offset.y = 0;
  render_pass_bi.clearValueCount = 1;
  render_pass_bi.pClearValues = &color_clear;
  vkCmdBeginRenderPass(cmd, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
}
}