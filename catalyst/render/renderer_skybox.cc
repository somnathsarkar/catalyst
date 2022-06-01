#include <catalyst/render/renderer.h>

namespace catalyst {
void Application::Renderer::CreateSkyboxPipeline() {
  const std::vector<char> vert_shader_code =
      ReadFile("../assets/shaders/skybox.vert.spv");
  const std::vector<char> frag_shader_code =
      ReadFile("../assets/shaders/skybox.frag.spv");

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
  vertex_input_info.pVertexAttributeDescriptions = vertex_ads;

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
  multisample_state.rasterizationSamples = msaa_samples_;

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
  pipeline_ci.layout = graphics_pipeline_layout_;
  pipeline_ci.renderPass = render_pass_;
  pipeline_ci.subpass = 0;
  pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;

  VkResult create_result = vkCreateGraphicsPipelines(
      device_, pipeline_cache_, 1, &pipeline_ci, nullptr, &skybox_pipeline_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create graphics pipeline!");

  vkDestroyShaderModule(device_, vert_shader, nullptr);
  vkDestroyShaderModule(device_, frag_shader, nullptr);
}
}