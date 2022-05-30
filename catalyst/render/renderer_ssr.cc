#include <catalyst/render/renderer.h>

namespace catalyst {
void Application::Renderer::CreateSsrResources() {
  ssr_format_ = hdr_format_;
  ssr_images_.resize(frame_count_);
  ssr_image_views_.resize(frame_count_);
  ssr_memory_.resize(frame_count_);
  ssr_uniform_memory_.resize(frame_count_);
  ssr_uniform_.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    CreateImage(
        ssr_images_[frame_i], ssr_memory_[frame_i], 0, ssr_format_,
        {half_swapchain_extent_.width, half_swapchain_extent_.height, 1}, 1, 1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CreateImageView(ssr_image_views_[frame_i], ssr_images_[frame_i],
                    VK_IMAGE_VIEW_TYPE_2D, ssr_format_,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    CreateBuffer(ssr_uniform_[frame_i], ssr_uniform_memory_[frame_i],
                 sizeof(SsrUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }
}
void Application::Renderer::CreateSsrRenderPass() {
  VkAttachmentDescription ssr_attachment{};
  ssr_attachment.flags = 0;
  ssr_attachment.format = ssr_format_;
  ssr_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  ssr_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  ssr_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  ssr_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  ssr_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  ssr_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ssr_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference ssr_ref{};
  ssr_ref.attachment = 0;
  ssr_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &ssr_ref;
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
  render_pass_ci.pAttachments = &ssr_attachment;
  render_pass_ci.subpassCount = 1;
  render_pass_ci.pSubpasses = &subpass;
  render_pass_ci.dependencyCount = 1;
  render_pass_ci.pDependencies = &dependency;
  VkResult create_result =
      vkCreateRenderPass(device_, &render_pass_ci, nullptr, &ssr_render_pass_);
  ASSERT(create_result == VK_SUCCESS, "Could not create SSR render pass!");
}
void Application::Renderer::CreateSsrPipeline() {
  const std::vector<char> vert_shader_code =
      ReadFile("../assets/shaders/ssr.vert.spv");
  const std::vector<char> frag_shader_code =
      ReadFile("../assets/shaders/ssr.frag.spv");

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
  layout_ci.pSetLayouts = &ssr_descriptor_set_layout_;
  VkResult result = vkCreatePipelineLayout(device_, &layout_ci, nullptr,
                                           &ssr_pipeline_layout_);
  ASSERT(result == VK_SUCCESS, "Failed to create SSR pipeline layout!");

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
  pipeline_ci.layout = ssr_pipeline_layout_;
  pipeline_ci.renderPass = ssr_render_pass_;
  pipeline_ci.subpass = 0;
  pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;

  VkResult create_result = vkCreateGraphicsPipelines(
      device_, pipeline_cache_, 1, &pipeline_ci, nullptr, &ssr_pipeline_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create SSR pipeline!");

  vkDestroyShaderModule(device_, vert_shader, nullptr);
  vkDestroyShaderModule(device_, frag_shader, nullptr);
}
void Application::Renderer::CreateSsrFramebuffers() {
  ssr_framebuffers_.resize(frame_count_);
  VkFramebufferCreateInfo framebuffer_ci{};
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    VkFramebufferCreateInfo framebuffer_ci{};
    framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.pNext = nullptr;
    framebuffer_ci.flags = 0;
    framebuffer_ci.renderPass = ssr_render_pass_;
    framebuffer_ci.attachmentCount = 1;
    framebuffer_ci.pAttachments = &ssr_image_views_[frame_i];
    framebuffer_ci.width = half_swapchain_extent_.width;
    framebuffer_ci.height = half_swapchain_extent_.height;
    framebuffer_ci.layers = 1;
    VkResult create_result = vkCreateFramebuffer(
        device_, &framebuffer_ci, nullptr, &ssr_framebuffers_[frame_i]);
    ASSERT(create_result == VK_SUCCESS, "Could not create SSR framebuffer!");
  }
}
void Application::Renderer::BeginSsrRenderPass(VkCommandBuffer& cmd,
                                                uint32_t swapchain_image_i) {
  VkClearValue color_clear;
  color_clear.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  VkRenderPassBeginInfo render_pass_bi{};
  render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_bi.pNext = nullptr;
  render_pass_bi.renderPass = ssr_render_pass_;
  render_pass_bi.framebuffer = ssr_framebuffers_[swapchain_image_i];
  render_pass_bi.renderArea.extent = half_swapchain_extent_;
  render_pass_bi.renderArea.offset.x = 0;
  render_pass_bi.renderArea.offset.y = 0;
  render_pass_bi.clearValueCount = 1;
  render_pass_bi.pClearValues = &color_clear;
  vkCmdBeginRenderPass(cmd, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
}
void Application::Renderer::ComputeSsrMap(VkCommandBuffer& cmd,
                                          uint32_t image_i, SceneDrawDetails& details) {
  uint32_t prev_image_i = (image_i + frame_count_ - 1) % frame_count_;
  // Clear SSR map
  BeginSsrRenderPass(cmd, image_i);
  // If previous frame is not available (for eg, due to resizing), stop here
  if (!rendered_frames_[prev_image_i]) {
    vkCmdEndRenderPass(cmd);
    return;
  }
  void* uniform_data = nullptr;
  vkMapMemory(device_, ssr_uniform_memory_[image_i], 0, VK_WHOLE_SIZE, 0, &uniform_data);
  memcpy(uniform_data, &details.ssr_uniform, sizeof(details.ssr_uniform));
  vkUnmapMemory(device_, ssr_uniform_memory_[image_i]);
  vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ssr_pipeline_layout_, 0, 1,
      &ssr_descriptor_sets_[image_i], 0, nullptr);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ssr_pipeline_);
  vkCmdDraw(cmd, 6, 1, 0, 0);
  vkCmdEndRenderPass(cmd);
}
}