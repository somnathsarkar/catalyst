#include <catalyst/render/renderer.h>

namespace catalyst {
void Application::Renderer::CreateDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding directional_light_binding{};
  directional_light_binding.binding = 0;
  directional_light_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  directional_light_binding.descriptorCount = 1;
  directional_light_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  directional_light_binding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding directional_shadow_binding{};
  directional_shadow_binding.binding = 1;
  directional_shadow_binding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  directional_shadow_binding.descriptorCount = Scene::kMaxDirectionalLights;
  directional_shadow_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  directional_shadow_binding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding material_uniform_binding{};
  material_uniform_binding.binding = 2;
  material_uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  material_uniform_binding.descriptorCount = 1;
  material_uniform_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  material_uniform_binding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding texture_binding{};
  texture_binding.binding = 3;
  texture_binding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  texture_binding.descriptorCount = Scene::kMaxTextures;
  texture_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  texture_binding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding bindings[] = {
      directional_light_binding, directional_shadow_binding,
      material_uniform_binding, texture_binding};

  VkDescriptorSetLayoutCreateInfo layout_ci{};
  layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_ci.pNext = nullptr;
  layout_ci.flags = 0;
  layout_ci.bindingCount = 4;
  layout_ci.pBindings = bindings;
  VkResult create_result = vkCreateDescriptorSetLayout(
      device_, &layout_ci, nullptr, &descriptor_set_layout_);
  ASSERT(create_result == VK_SUCCESS,
         "Failed to create descriptor set layout!");
}
void Application::Renderer::CreateDescriptorPool() {
  uint32_t frame_count = frame_count_;

  VkDescriptorPoolSize uniform_size;
  uniform_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniform_size.descriptorCount = frame_count*2;
  VkDescriptorPoolSize sampler_size;
  sampler_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_size.descriptorCount = frame_count * (Scene::kMaxDirectionalLights+Scene::kMaxTextures);
  VkDescriptorPoolSize pool_sizes[] = {uniform_size, sampler_size};

  VkDescriptorPoolCreateInfo pool_ci{};
  pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_ci.pNext = nullptr;
  pool_ci.flags = 0;
  pool_ci.maxSets = frame_count;
  pool_ci.poolSizeCount = 2;
  pool_ci.pPoolSizes = pool_sizes;
  VkResult create_result =
      vkCreateDescriptorPool(device_, &pool_ci, nullptr, &descriptor_pool_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create descriptor pool!");
}
void Application::Renderer::CreateDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts;
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++)
    layouts.push_back(descriptor_set_layout_);
  VkDescriptorSetAllocateInfo set_ai{};
  set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  set_ai.pNext = nullptr;
  set_ai.descriptorPool = descriptor_pool_;
  set_ai.descriptorSetCount = frame_count_;
  set_ai.pSetLayouts = layouts.data();
  descriptor_sets_.resize(frame_count_);
  VkResult alloc_result =
      vkAllocateDescriptorSets(device_, &set_ai, descriptor_sets_.data());
  ASSERT(alloc_result == VK_SUCCESS, "Failed to create descriptor set layout!");
}

void Application::Renderer::WriteDescriptorSets() {
  // Directional Lights
  std::vector<VkDescriptorBufferInfo> set_bis;
  set_bis.resize(frame_count_);
  std::vector<VkWriteDescriptorSet> set_wis;
  set_wis.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    VkDescriptorBufferInfo& set_bi = set_bis[frame_i];
    set_bi.buffer = directional_light_uniform_buffers_[frame_i];
    set_bi.offset = 0;
    set_bi.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet& set_wi = set_wis[frame_i];
    set_wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_wi.pNext = nullptr;
    set_wi.dstSet = descriptor_sets_[frame_i];
    set_wi.dstBinding = 0;
    set_wi.dstArrayElement = 0;
    set_wi.descriptorCount = 1;
    set_wi.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    set_wi.pBufferInfo = &set_bis[frame_i];
  }
  vkUpdateDescriptorSets(device_, frame_count_, set_wis.data(), 0, nullptr);

  // Directional Light Shadows
  std::vector<std::vector<VkDescriptorImageInfo>> shadow_image_infos;
  shadow_image_infos.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    shadow_image_infos[frame_i].resize(Scene::kMaxDirectionalLights);
    for (uint32_t shadow_i = 0; shadow_i < Scene::kMaxDirectionalLights;
         shadow_i++) {
      VkDescriptorImageInfo& set_si = shadow_image_infos[frame_i][shadow_i];
      set_si.sampler = shadowmap_sampler_;
      set_si.imageView = shadowmap_image_views_[frame_i][shadow_i];
      set_si.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet& set_wi = set_wis[frame_i];
    set_wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_wi.pNext = nullptr;
    set_wi.dstSet = descriptor_sets_[frame_i];
    set_wi.dstBinding = 1;
    set_wi.dstArrayElement = 0;
    set_wi.descriptorCount = Scene::kMaxDirectionalLights;
    set_wi.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    set_wi.pBufferInfo = nullptr;
    set_wi.pImageInfo = shadow_image_infos[frame_i].data();
  }
  vkUpdateDescriptorSets(device_, frame_count_, set_wis.data(), 0, nullptr);

  // Material Uniform
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    VkDescriptorBufferInfo& set_bi = set_bis[frame_i];
    set_bi.buffer = material_uniform_buffers_[frame_i];
    set_bi.offset = 0;
    set_bi.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet& set_wi = set_wis[frame_i];
    set_wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_wi.pNext = nullptr;
    set_wi.dstSet = descriptor_sets_[frame_i];
    set_wi.dstBinding = 2;
    set_wi.dstArrayElement = 0;
    set_wi.descriptorCount = 1;
    set_wi.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    set_wi.pBufferInfo = &set_bis[frame_i];
  }
  vkUpdateDescriptorSets(device_, frame_count_, set_wis.data(), 0, nullptr);

  // Texture Images
  std::vector<std::vector<VkDescriptorImageInfo>> texture_image_infos;
  texture_image_infos.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    texture_image_infos[frame_i].resize(Scene::kMaxTextures);
    for (uint32_t tex_i = 0; tex_i < Scene::kMaxDirectionalLights;
         tex_i++) {
      VkDescriptorImageInfo& set_si = texture_image_infos[frame_i][tex_i];
      set_si.sampler = texture_sampler_;
      set_si.imageView = texture_image_views_[tex_i];
      set_si.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet& set_wi = set_wis[frame_i];
    set_wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_wi.pNext = nullptr;
    set_wi.dstSet = descriptor_sets_[frame_i];
    set_wi.dstBinding = 3;
    set_wi.dstArrayElement = 0;
    set_wi.descriptorCount = Scene::kMaxTextures;
    set_wi.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    set_wi.pBufferInfo = nullptr;
    set_wi.pImageInfo = texture_image_infos[frame_i].data();
  }
  vkUpdateDescriptorSets(device_, frame_count_, set_wis.data(), 0, nullptr);
}
}  // namespace catalyst