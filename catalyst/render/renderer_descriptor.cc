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

  VkDescriptorSetLayoutBinding bindings[] = {directional_light_binding,
                                             directional_shadow_binding};

  VkDescriptorSetLayoutCreateInfo layout_ci{};
  layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_ci.pNext = nullptr;
  layout_ci.flags = 0;
  layout_ci.bindingCount = 2;
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
  uniform_size.descriptorCount = frame_count;
  VkDescriptorPoolSize sampler_size;
  sampler_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_size.descriptorCount = frame_count * Scene::kMaxDirectionalLights;
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
}  // namespace catalyst