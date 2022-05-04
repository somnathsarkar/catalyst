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
Application::Renderer::MaterialUniformBlock::MaterialUniformBlock()
    : materials_(static_cast<size_t>(Scene::kMaxMaterials)),
      material_count_(0) {}
size_t Application::Renderer::MaterialUniformBlock::GetSize() {
  return sizeof(MaterialUniformBlock) * Scene::kMaxMaterials +
         sizeof(uint32_t);
}
void Application::Renderer::LoadScene(const Scene& scene) {
  scene_ = &scene;
  scene_resource_details_ = {0};
  LoadSceneResources();
}
void Application::Renderer::LoadSceneResources() {
  LoadMeshes();
  LoadTextures();
}
void Application::Renderer::LoadMeshes() {
  uint32_t mesh_count = static_cast<uint32_t>(scene_->meshes_.size());
  if (scene_resource_details_.mesh_count == mesh_count) return;
  VkCommandBuffer cmd;
  BeginSingleUseCommandBuffer(cmd);
  VkBuffer staging_buffer;
  VkDeviceMemory staging_memory;
  uint32_t buffer_size = 0;
  for (uint32_t mesh_i = scene_resource_details_.mesh_count;
       mesh_i < mesh_count; mesh_i++) {
    const Mesh* mesh = scene_->meshes_[mesh_i];
    uint32_t mesh_size = static_cast<uint32_t>(sizeof(mesh->vertices[0]) *
                                               mesh->vertices.size());
    buffer_size += mesh_size;
  }
  CreateBuffer(staging_buffer, staging_memory, buffer_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void* data;
  vkMapMemory(device_, staging_memory, 0, VK_WHOLE_SIZE, 0, &data);
  size_t vertex_prefix = scene_resource_details_.vertex_count;
  size_t buffer_prefix = 0;
  for (uint32_t mesh_i = scene_resource_details_.mesh_count;
       mesh_i < mesh_count; mesh_i++) {
    Mesh* mesh = scene_->meshes_[mesh_i];
    size_t mesh_vertices = mesh->vertices.size();
    size_t mesh_size = sizeof(Vertex) * mesh_vertices;
    memcpy(reinterpret_cast<char*>(data) + buffer_prefix, mesh->vertices.data(),
           mesh_size);
    buffer_prefix += mesh_size;
    vertex_prefix += mesh_vertices;
  }
  vkUnmapMemory(device_, staging_memory);
  VkBufferCopy buffer_cp{};
  buffer_cp.srcOffset = 0;
  buffer_cp.dstOffset = sizeof(Vertex)*scene_resource_details_.vertex_count;
  buffer_cp.size = buffer_size;
  vkCmdCopyBuffer(cmd, staging_buffer, vertex_buffer_, 1, &buffer_cp);

  scene_resource_details_.vertex_count = static_cast<uint32_t>(vertex_prefix);
  scene_resource_details_.mesh_count =
      static_cast<uint32_t>(scene_->meshes_.size());

  EndSingleUseCommandBuffer(cmd);
  vkDestroyBuffer(device_, staging_buffer, nullptr);
  vkFreeMemory(device_, staging_memory, nullptr);
}
void Application::Renderer::LoadTextures() {
  uint32_t tex_count = static_cast<uint32_t>(scene_->textures_.size());
  if (scene_resource_details_.texture_count = tex_count) return;
  for (uint32_t tex_i = scene_resource_details_.texture_count;
       tex_i < tex_count; tex_i++) {
  }
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
  LoadSceneResources();
  VkCommandBuffer& cmd = command_buffers_[frame_i];
  VkDeviceSize vertex_offsets[] = {static_cast<uint64_t>(0)};
  vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer_, vertex_offsets);

  SceneDrawDetails details;
  details.push_constants.world_to_view_transform = glm::mat4(1.0f);
  details.push_constants.view_to_clip_transform = glm::mat4(1.0f);
  details.directional_light_uniform.light_count_ = 0;
  details.material_uniform_block.material_count_ =
      static_cast<uint32_t>(scene_->materials_.size());
  for (uint32_t mat_i = 0;
       mat_i < details.material_uniform_block.material_count_; mat_i++) {
    MaterialUniform& mat_uniform =
        details.material_uniform_block.materials_[mat_i];
    const Material* mat = scene_->materials_[mat_i];
    details.material_uniform_block.materials_[mat_i].color = glm::vec4(mat->albedo_,1.0f);
    details.material_uniform_block.materials_[mat_i].reflectance =
        mat->reflectance_;
    details.material_uniform_block.materials_[mat_i].metallic = mat->metallic_;
    details.material_uniform_block.materials_[mat_i].roughness =
        mat->roughness_;
  }
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

  void* material_uniform_data;
  vkMapMemory(device_, material_uniform_memory_[frame_i], 0, VK_WHOLE_SIZE, 0,
              &material_uniform_data);
  size_t material_array_size = Scene::kMaxMaterials * sizeof(MaterialUniform);
  memcpy(material_uniform_data, details.material_uniform_block.materials_.data(), material_array_size);
  memcpy(static_cast<char*>(material_uniform_data) + material_array_size,
         &details.material_uniform_block.material_count_, sizeof(uint32_t));
  vkUnmapMemory(device_, material_uniform_memory_[frame_i]);

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
      const Mesh* mesh = scene_->meshes_[mesh_id];
      vkCmdPushConstants(cmd, layout,
                         VK_SHADER_STAGE_VERTEX_BIT,
                         offsetof(PushConstantData, model_to_world_transform),
                         sizeof(details.push_constants.model_to_world_transform),
                         &model_transform);
      vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT,
                         offsetof(PushConstantData, material_id),
                         sizeof(details.push_constants.material_id),
                         &mesh->material_id);
      vkCmdDraw(cmd, static_cast<uint32_t>(mesh->vertices.size()), 1,
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