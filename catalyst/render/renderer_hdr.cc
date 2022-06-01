#include <catalyst/render/renderer.h>

namespace catalyst {
void Application::Renderer::CreateHdrResources() {
  hdr_format_ = VK_FORMAT_R32G32B32A32_SFLOAT;
  hdr_images_.resize(frame_count_);
  hdr_image_views_.resize(frame_count_);
  hdr_memory_.resize(frame_count_);
  hdr_tonemapping_buffers_.resize(frame_count_);
  hdr_tonemapping_memory_.resize(frame_count_);
  hdr_msaa_images_.resize(frame_count_);
  hdr_msaa_memory_.resize(frame_count_);
  hdr_msaa_views_.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    CreateImage(hdr_images_[frame_i], hdr_memory_[frame_i], 0, hdr_format_,
                {swapchain_extent_.width, swapchain_extent_.height, 1}, 1, 1,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SAMPLE_COUNT_1_BIT);
    CreateImageView(hdr_image_views_[frame_i], hdr_images_[frame_i],
                    VK_IMAGE_VIEW_TYPE_2D, hdr_format_,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    CreateBuffer(
        hdr_tonemapping_buffers_[frame_i], hdr_tonemapping_memory_[frame_i],
        sizeof(TonemappingUniform),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    CreateImage(hdr_msaa_images_[frame_i], hdr_msaa_memory_[frame_i], 0,
                hdr_format_,
                {swapchain_extent_.width, swapchain_extent_.height, 1}, 1, 1,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, msaa_samples_);
    CreateImageView(hdr_msaa_views_[frame_i], hdr_msaa_images_[frame_i],
                    VK_IMAGE_VIEW_TYPE_2D, hdr_format_,
                    VK_IMAGE_ASPECT_COLOR_BIT);
  }
}
void Application::Renderer::CreateHdrRenderPass() {
  VkAttachmentDescription color_attachment{};
  color_attachment.flags = 0;
  color_attachment.format = swapchain_image_format_;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depth_attachment{};
  depth_attachment.flags = 0;
  depth_attachment.format = depth_format_;
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  depth_attachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_ref{};
  color_ref.attachment = 0;
  color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_ref{};
  depth_ref.attachment = 1;
  depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_ref;
  subpass.pResolveAttachments = nullptr;
  subpass.pDepthStencilAttachment = &depth_ref;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = nullptr;

  VkAttachmentDescription attachments[] = {color_attachment, depth_attachment};

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo render_pass_ci{};
  render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_ci.pNext = nullptr;
  render_pass_ci.flags = 0;
  render_pass_ci.attachmentCount = 2;
  render_pass_ci.pAttachments = attachments;
  render_pass_ci.subpassCount = 1;
  render_pass_ci.pSubpasses = &subpass;
  render_pass_ci.dependencyCount = 1;
  render_pass_ci.pDependencies = &dependency;
  VkResult create_result =
      vkCreateRenderPass(device_, &render_pass_ci, nullptr, &hdr_render_pass_);
  ASSERT(create_result == VK_SUCCESS, "Could not create HDR render pass!");
}
void Application::Renderer::CreateHdrPipeline() {
  const std::vector<char> vert_shader_code =
      ReadFile("../assets/shaders/hdr.vert.spv");
  const std::vector<char> frag_shader_code =
      ReadFile("../assets/shaders/hdr.frag.spv");

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
  viewport.width = (float)swapchain_extent_.width;
  viewport.height = (float)swapchain_extent_.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain_extent_;

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

  VkPipelineLayoutCreateInfo layout_ci{};
  layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_ci.pNext = nullptr;
  layout_ci.flags = 0;
  layout_ci.pushConstantRangeCount = 0;
  layout_ci.pPushConstantRanges = nullptr;
  layout_ci.setLayoutCount = 1;
  layout_ci.pSetLayouts = &hdr_descriptor_set_layout_;
  VkResult result = vkCreatePipelineLayout(device_, &layout_ci, nullptr,
                                           &hdr_pipeline_layout_);
  ASSERT(result == VK_SUCCESS, "Failed to create HDR pipeline layout!");

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
  pipeline_ci.layout = hdr_pipeline_layout_;
  pipeline_ci.renderPass = hdr_render_pass_;
  pipeline_ci.subpass = 0;
  pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;

  VkResult create_result = vkCreateGraphicsPipelines(
      device_, pipeline_cache_, 1, &pipeline_ci, nullptr, &hdr_pipeline_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create HDR pipeline!");

  vkDestroyShaderModule(device_, vert_shader, nullptr);
  vkDestroyShaderModule(device_, frag_shader, nullptr);
}
void Application::Renderer::CreateHdrFramebuffers() {
  hdr_framebuffers_.resize(frame_count_);
  VkFramebufferCreateInfo framebuffer_ci{};
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    VkImageView attachments[] = {swapchain_image_views_[frame_i],
                                 depth_image_views_[frame_i]};
    VkFramebufferCreateInfo framebuffer_ci{};
    framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.pNext = nullptr;
    framebuffer_ci.flags = 0;
    framebuffer_ci.renderPass = hdr_render_pass_;
    framebuffer_ci.attachmentCount = 2;
    framebuffer_ci.pAttachments = attachments;
    framebuffer_ci.width = swapchain_extent_.width;
    framebuffer_ci.height = swapchain_extent_.height;
    framebuffer_ci.layers = 1;
    VkResult create_result = vkCreateFramebuffer(
        device_, &framebuffer_ci, nullptr, &hdr_framebuffers_[frame_i]);
    ASSERT(create_result == VK_SUCCESS, "Could not create HDR framebuffer!");
  }
}
void Application::Renderer::BeginHdrRenderPass(VkCommandBuffer& cmd,
                                               uint32_t swapchain_image_i) {
  VkClearValue color_clear;
  color_clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  VkClearValue depth_clear;
  depth_clear.depthStencil.depth = 1.0f;
  depth_clear.depthStencil.stencil = 0;
  VkClearValue clear_values[] = {color_clear, depth_clear};
  VkRenderPassBeginInfo render_pass_bi{};
  render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_bi.pNext = nullptr;
  render_pass_bi.renderPass = hdr_render_pass_;
  render_pass_bi.framebuffer = hdr_framebuffers_[swapchain_image_i];
  render_pass_bi.renderArea.extent = swapchain_extent_;
  render_pass_bi.renderArea.offset.x = 0;
  render_pass_bi.renderArea.offset.y = 0;
  render_pass_bi.clearValueCount = 2;
  render_pass_bi.pClearValues = clear_values;
  vkCmdBeginRenderPass(cmd, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
}
}