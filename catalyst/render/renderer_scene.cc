#include <catalyst/render/renderer.h>

#include <glm/gtx/transform.hpp>

#include <catalyst/scene/scene.h>
#include <catalyst/scene/sceneobject.h>
#include <catalyst/time/timemanager.h>

namespace catalyst {
Application::Renderer::DirectionalLightUniform::DirectionalLightUniform()
    : lights_(static_cast<size_t>(Scene::kMaxDirectionalLights)),light_count_(0) {}
size_t Application::Renderer::DirectionalLightUniform::GetSize() {
  return sizeof(DirectionalLight) * Scene::kMaxDirectionalLights +
         sizeof(uint32_t);
}
void Application::Renderer::LoadScene(const Scene& scene) {
  scene_ = &scene;
  scene_resource_details_ = {0};
  LoadSceneResources();
}
void Application::Renderer::LoadSceneResources() {
  LoadMeshes();
}
void Application::Renderer::LoadMeshes() {
  VkCommandBuffer cmd;
  BeginSingleUseCommandBuffer(cmd);
  VkBuffer staging_buffer;
  VkDeviceMemory staging_memory;
  uint32_t buffer_size = 0;
  for (uint32_t mesh_i = scene_resource_details_.mesh_count;
       mesh_i < static_cast<uint32_t>(scene_->meshes_.size()); mesh_i++) {
    const Mesh& mesh = scene_->meshes_[mesh_i];
    uint32_t mesh_size =
        static_cast<uint32_t>(sizeof(mesh.vertices[0]) * mesh.vertices.size());
    buffer_size += mesh_size;
  }
  CreateBuffer(staging_buffer, staging_memory, buffer_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void* data;
  vkMapMemory(device_, staging_memory, 0, VK_WHOLE_SIZE, 0, &data);
  uint32_t vertex_prefix = scene_resource_details_.vertex_count;
  for (Mesh mesh : scene_->meshes_) {
    uint32_t mesh_size =
        static_cast<uint32_t>(sizeof(mesh.vertices[0]) * mesh.vertices.size());
    memcpy(reinterpret_cast<char*>(data) + vertex_prefix, mesh.vertices.data(),
           mesh_size);
    vertex_prefix += mesh_size;
  }
  vkUnmapMemory(device_, staging_memory);
  VkBufferCopy buffer_cp{};
  buffer_cp.srcOffset = 0;
  buffer_cp.dstOffset = sizeof(Vertex)*scene_resource_details_.vertex_count;
  buffer_cp.size = buffer_size;
  vkCmdCopyBuffer(cmd, staging_buffer, vertex_buffer_, 1, &buffer_cp);

  scene_resource_details_.vertex_count = vertex_prefix;
  scene_resource_details_.mesh_count =
      static_cast<uint32_t>(scene_->meshes_.size());

  EndSingleUseCommandBuffer(cmd);
  vkDestroyBuffer(device_, staging_buffer, nullptr);
  vkFreeMemory(device_, staging_memory, nullptr);
}
void Application::Renderer::CreateVertexBuffer() {
  CreateBuffer(
      vertex_buffer_, vertex_memory_, sizeof(Vertex)*Scene::kMaxVertices,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}
void Application::Renderer::CreateDirectionalLightUniformBuffer() {
  uint32_t num_frames = frame_count_;
  uniform_buffers_.resize(num_frames);
  uniform_memory_.resize(num_frames);
  for (uint32_t frame_i = 0;
       frame_i < num_frames; frame_i++) {
    CreateBuffer(
        uniform_buffers_[frame_i], uniform_memory_[frame_i],
        DirectionalLightUniform::GetSize(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
  }
}
void Application::Renderer::WriteDescriptorSets() {
  std::vector<VkDescriptorBufferInfo> set_bis;
  set_bis.resize(frame_count_);
  std::vector<VkWriteDescriptorSet> set_wis;
  set_wis.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    VkDescriptorBufferInfo& set_bi = set_bis[frame_i];
    set_bi.buffer = uniform_buffers_[frame_i];
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

  std::vector<std::vector<VkDescriptorImageInfo>> shadow_image_infos;
  shadow_image_infos.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    shadow_image_infos[frame_i].resize(Scene::kMaxDirectionalLights);
    for (uint32_t shadow_i = 0; shadow_i < Scene::kMaxDirectionalLights;
         shadow_i++) {
      VkDescriptorImageInfo& set_si = shadow_image_infos[frame_i][shadow_i];
      set_si.sampler = sampler_;
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
}
uint32_t Application::Renderer::SelectMemoryType(const VkMemoryRequirements& mem_reqs, const VkMemoryPropertyFlags& req_props) {
  bool mem_found = false;
  uint32_t mem_index = -1;
  for (uint32_t mem_i = 0; mem_i < mem_props_.memoryTypeCount; mem_i++) {
    if ((mem_reqs.memoryTypeBits >> mem_i)&1 &&
        (mem_props_.memoryTypes[mem_i].propertyFlags & req_props) == req_props) {
      mem_found = true;
      mem_index = mem_i;
    }
  }
  ASSERT(mem_found, "Could not find suitable memory type!");
  return mem_index;
}
void Application::Renderer::CreateBuffer(
    VkBuffer& buffer, VkDeviceMemory& memory, const VkDeviceSize& size,
    const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& req_props) {
  VkBufferCreateInfo buffer_ci{};
  buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_ci.pNext = nullptr;
  buffer_ci.flags = 0;
  buffer_ci.size = size;
  buffer_ci.usage = usage;
  buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_ci.queueFamilyIndexCount = 1;
  buffer_ci.pQueueFamilyIndices =
      &queue_family_indices_.transfer_queue_index_.value();
  VkResult create_result =
      vkCreateBuffer(device_, &buffer_ci, nullptr, &buffer);
  ASSERT(create_result == VK_SUCCESS, "Failed to create buffer!");

  VkMemoryRequirements mem_reqs{};
  vkGetBufferMemoryRequirements(device_, buffer, &mem_reqs);

  VkMemoryAllocateInfo memory_ai{};
  memory_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memory_ai.pNext = nullptr;
  memory_ai.allocationSize = mem_reqs.size;
  memory_ai.memoryTypeIndex = SelectMemoryType(mem_reqs, req_props);
  VkResult alloc_result = vkAllocateMemory(device_, &memory_ai, nullptr, &memory);
  ASSERT(alloc_result == VK_SUCCESS, "Failed to allocate memory!");

  VkResult bind_result = vkBindBufferMemory(device_, buffer, memory, 0);
  ASSERT(bind_result == VK_SUCCESS, "Failed to bind memory to buffer!");
}
void Application::Renderer::BeginSingleUseCommandBuffer(VkCommandBuffer& cmd) {
  VkCommandBufferAllocateInfo cmd_ai{};
  cmd_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_ai.pNext = nullptr;
  cmd_ai.commandPool = command_pool_;
  cmd_ai.commandBufferCount = 1;
  cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  VkResult alloc_result = vkAllocateCommandBuffers(device_, &cmd_ai, &cmd);
  ASSERT(alloc_result == VK_SUCCESS,
         "Failed to allocate single use command buffer!");

  VkCommandBufferBeginInfo cmd_bi{};
  cmd_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_bi.pNext = nullptr;
  cmd_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  cmd_bi.pInheritanceInfo = nullptr;
  VkResult begin_result = vkBeginCommandBuffer(cmd, &cmd_bi);
  ASSERT(begin_result == VK_SUCCESS, "Failed to begin recording single use command buffer!");
}
void Application::Renderer::EndSingleUseCommandBuffer(VkCommandBuffer& cmd) {
  vkEndCommandBuffer(cmd);
  
  VkSubmitInfo cmd_si{};
  cmd_si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  cmd_si.pNext = nullptr;
  cmd_si.commandBufferCount = 1;
  cmd_si.pCommandBuffers = &cmd;
  cmd_si.waitSemaphoreCount = 0;
  cmd_si.pWaitSemaphores = nullptr;
  cmd_si.pWaitDstStageMask = 0;
  cmd_si.signalSemaphoreCount = 0;
  cmd_si.pSignalSemaphores = nullptr;
  VkResult submit_result =
      vkQueueSubmit(graphics_queue_, 1, &cmd_si, VK_NULL_HANDLE);
  vkDeviceWaitIdle(device_);

  vkFreeCommandBuffers(device_, command_pool_, 1, &cmd);
}
void Application::Renderer::UnloadScene() {
  ASSERT(scene_ != nullptr, "No scene loaded!");
  vkDeviceWaitIdle(device_);
  vkDestroyBuffer(device_, vertex_buffer_, nullptr);
  vkFreeMemory(device_, vertex_memory_, nullptr);
  for (VkBuffer buffer_ : uniform_buffers_)
    vkDestroyBuffer(device_, buffer_, nullptr);
  for (VkDeviceMemory memory_ : uniform_memory_)
    vkFreeMemory(device_, memory_, nullptr);
  scene_ = nullptr;
}
void Application::Renderer::DrawScene(uint32_t frame_i, uint32_t image_i) {
  VkCommandBuffer& cmd = command_buffers_[frame_i];
  VkDeviceSize vertex_offsets[] = {static_cast<uint64_t>(0)};
  vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer_, vertex_offsets);

  SceneDrawDetails details;
  details.push_constants.world_to_view_transform = glm::mat4(1.0f);
  details.push_constants.view_to_clip_transform = glm::mat4(1.0f);
  details.directional_light_uniform.light_count_ = 0;
  DrawScenePrePass(cmd, details, scene_->root_, glm::mat4(1.0f));

  DrawSceneShadowmaps(cmd, frame_i, details);
  
  vkCmdPushConstants(cmd, graphics_pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT,
                     offsetof(PushConstantData, world_to_view_transform),
                     sizeof(details.push_constants.world_to_view_transform),
                     &details.push_constants.world_to_view_transform);
  vkCmdPushConstants(cmd, graphics_pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT,
                     offsetof(PushConstantData, view_to_clip_transform),
                     sizeof(details.push_constants.view_to_clip_transform),
                     &details.push_constants.view_to_clip_transform);

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          graphics_pipeline_layout_, 0, 1,
                          &descriptor_sets_[frame_i], 0, nullptr);
  BeginGraphicsRenderPass(cmd, image_i);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
  void* uniform_data;
  vkMapMemory(device_, uniform_memory_[frame_i], 0, VK_WHOLE_SIZE, 0,
              &uniform_data);
  size_t light_array_size =
      Scene::kMaxDirectionalLights * sizeof(DirectionalLight);
  memcpy(uniform_data, details.directional_light_uniform.lights_.data(),
         light_array_size);
  memcpy(static_cast<char*>(uniform_data)+light_array_size, &details.directional_light_uniform.light_count_,
         sizeof(uint32_t));
  vkUnmapMemory(device_, uniform_memory_[frame_i]);
  DrawSceneMeshes(cmd, graphics_pipeline_layout_, details, scene_->root_, glm::mat4(1.0f));

  if (debug_enabled_) {
    DebugDrawScene(frame_i);
  }
  vkCmdEndRenderPass(cmd);
}
void Application::Renderer::DrawSceneMeshes(VkCommandBuffer& cmd, VkPipelineLayout& layout,
                                            SceneDrawDetails& details,
                                            const SceneObject* focus,
                                            glm::mat4 model_transform) {
  model_transform *= focus->transform_.GetTransformationMatrix();
  switch (focus->type_) {
    case SceneObjectType::kMesh: {
      const MeshObject* mesh_object =
          reinterpret_cast<const MeshObject*>(focus);
      const uint32_t mesh_id = mesh_object->mesh_id_;
      const Mesh& mesh = scene_->meshes_[mesh_id];
      vkCmdPushConstants(cmd, layout,
                         VK_SHADER_STAGE_VERTEX_BIT,
                         offsetof(PushConstantData, model_to_world_transform),
                         sizeof(details.push_constants.model_to_world_transform),
                         &model_transform);
      vkCmdDraw(cmd, static_cast<uint32_t>(mesh.vertices.size()), 1,
                scene_->offsets_[mesh_id], 0);
      break;
    }
    default: {
      break;
    }
  }
  for (const SceneObject* child : focus->children_) {
    DrawSceneMeshes(cmd, layout, details, child, model_transform);
  }
}
void Application::Renderer::DebugDrawScene(uint32_t frame_i) {
  VkCommandBuffer& cmd = command_buffers_[frame_i];
  SceneDrawDetails dummy_details;
  for (const DebugDrawObject* debugdraw_object : scene_->debugdraw_objects_) {
    switch (debugdraw_object->type_) {
      case DebugDrawType::kWireframe: {
        VkDeviceSize vertex_offsets[] = {static_cast<uint64_t>(0)};
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          debugdraw_pipeline_);
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer_, vertex_offsets);
        const DebugDrawWireframe* wireframe =
            reinterpret_cast<const DebugDrawWireframe*>(debugdraw_object);
        DrawSceneMeshes(cmd, debugdraw_pipeline_layout_, dummy_details,
                        wireframe->scene_object_,
                        scene_->GetParentTransform(wireframe->scene_object_));
        break;
      }
      case DebugDrawType::kAABB: {
        const DebugDrawAABB* draw_aabb =
            static_cast<const DebugDrawAABB*>(debugdraw_object);
        const Aabb& aabb = draw_aabb->aabb_;
        DebugDrawAabb(frame_i, aabb);
        break;
      }
    }
  }
}
void Application::Renderer::DebugDrawAabb(uint32_t frame_i,
                                          const Aabb& aabb) {
  VkCommandBuffer& cmd = command_buffers_[frame_i];
  static auto BitmaskToVertex = [](uint32_t bm, const Aabb& aabb) -> Vertex {
    Vertex a{};
    if (bm & (1 << 0))
      a.position.x = aabb.xmax;
    else
      a.position.x = aabb.xmin;
    if (bm & (1 << 1))
      a.position.y = aabb.ymax;
    else
      a.position.y = aabb.ymin;
    if (bm & (1 << 2))
      a.position.z = aabb.zmax;
    else
      a.position.z = aabb.zmin;
    return a;
  };
  std::vector<Vertex> vertices;
  for (uint32_t aabb_mask = 0; aabb_mask < (1 << 3); aabb_mask++) {
    for (uint32_t bit_i = 0; bit_i < 3; bit_i++) {
      if (aabb_mask & (1 << bit_i)) continue;
      uint32_t a = aabb_mask;
      uint32_t b = aabb_mask | (1 << bit_i);
      Vertex v_a = BitmaskToVertex(a, aabb);
      Vertex v_b = BitmaskToVertex(b, aabb);
      vertices.push_back(v_a);
      vertices.push_back(v_b);
    }
  }
  void* data;
  vkMapMemory(device_, debugdraw_memory_[frame_i], 0, VK_WHOLE_SIZE, 0, &data);
  memcpy(data, vertices.data(), static_cast<size_t>(vertices.size() * sizeof(Vertex)));
  vkUnmapMemory(device_, debugdraw_memory_[frame_i]);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    debugdraw_lines_pipeline_);
  glm::mat4 identity(1.0f);
  vkCmdPushConstants(cmd, debugdraw_pipeline_layout_,
                     VK_SHADER_STAGE_VERTEX_BIT,
                     offsetof(PushConstantData, model_to_world_transform),
                     sizeof(glm::mat4), &identity);
  VkDeviceSize vertex_offsets[] = {static_cast<uint64_t>(0)};
  vkCmdBindVertexBuffers(cmd, 0, 1, &debugdraw_buffer_[frame_i], vertex_offsets);
  vkCmdDraw(cmd, vertices.size(), 1, 0, 0);
}
void Application::Renderer::DrawSceneShadowmaps(VkCommandBuffer& cmd,
                                                uint32_t swapchain_image_i,
                                                SceneDrawDetails& details) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, depthmap_pipeline_);
  for (uint32_t shadow_i = 0;
       shadow_i < details.directional_light_uniform.light_count_; shadow_i++) {
    DirectionalLight light =
        details.directional_light_uniform.lights_[shadow_i];
    vkCmdPushConstants(cmd, depthmap_pipeline_layout_,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       offsetof(PushConstantData, world_to_view_transform),
                       sizeof(light.world_to_light_transform),
                       &light.world_to_light_transform);
    vkCmdPushConstants(
        cmd, depthmap_pipeline_layout_,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       offsetof(PushConstantData, view_to_clip_transform),
                       sizeof(light.light_to_clip_transform),
                       &light.light_to_clip_transform);
    BeginDepthmapRenderPass(cmd, shadowmap_framebuffers_[swapchain_image_i][shadow_i]);
    DrawSceneMeshes(cmd, depthmap_pipeline_layout_, details, scene_->root_, glm::mat4(1.0f));
    vkCmdEndRenderPass(cmd);
  }
}
void Application::Renderer::DrawScenePrePass(VkCommandBuffer& cmd,
                                             SceneDrawDetails& details,
                                            const SceneObject* focus,
                                            glm::mat4 model_transform) {
  model_transform *= focus->transform_.GetTransformationMatrix();
  switch (focus->type_) {
    case SceneObjectType::kCamera: {
      const CameraObject* camera_object =
          reinterpret_cast<const CameraObject*>(focus);
      const uint32_t camera_id = camera_object->camera_id_;
      const Camera& camera = scene_->cameras_[camera_id];
      details.push_constants.world_to_view_transform = glm::inverse(model_transform);
      details.push_constants.view_to_clip_transform =
          camera.GetViewToClipTransform(
          swapchain_extent_.width, swapchain_extent_.height);
      break;
    }
    case SceneObjectType::kDirectionalLight: {
      const DirectionalLightObject* light_object =
          reinterpret_cast<const DirectionalLightObject*>(focus);
      uint32_t light_count = details.directional_light_uniform.light_count_;
      if (light_count < Scene::kMaxDirectionalLights) {
        const float kDirectionalLightDistance = 10.0f;
        glm::mat4 light_to_world_transform =
            light_object->transform_.GetOrientationMatrix();
        light_to_world_transform[3] =
            glm::vec4(glm::vec3(light_to_world_transform[1]) * -kDirectionalLightDistance, 1.0f);
        details.directional_light_uniform.lights_[light_count].world_to_light_transform =
            glm::inverse(light_to_world_transform);
        details.directional_light_uniform.lights_[light_count]
            .light_to_clip_transform =
            DirectionalLightObject::GetViewToClipTransform();
        details.directional_light_uniform.lights_[light_count].color =
            light_object->color_;
        details.directional_light_uniform.light_count_++;
      }
      break;
    }
    default: {
      break;
    }
  }
  for (const SceneObject* child : focus->children_) {
    DrawScenePrePass(cmd, details, child, model_transform);
  }
}
}  // namespace catalyst