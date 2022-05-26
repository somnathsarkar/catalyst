#include <catalyst/render/renderer.h>

#include <glm/gtx/transform.hpp>

#include <catalyst/scene/scene.h>
#include <catalyst/scene/sceneobject.h>
#include <catalyst/time/timemanager.h>
#include <catalyst/filesystem/importer.h>

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
  scene_resource_details_.vertex_offsets_.clear();
  scene_resource_details_.index_offsets_.clear();
  LoadSceneResources();
}
void Application::Renderer::LoadSceneResources() {
  LoadMeshes();
  LoadTextures();
  LoadCubemaps();
}
void Application::Renderer::LoadMeshes() {
  uint32_t mesh_count = static_cast<uint32_t>(scene_->meshes_.size());
  if (scene_resource_details_.mesh_count == mesh_count) return;
  VkCommandBuffer cmd;
  BeginSingleUseCommandBuffer(cmd);
  VkBuffer vertex_staging_buffer;
  VkDeviceMemory vertex_staging_memory;
  VkBuffer index_staging_buffer;
  VkDeviceMemory index_staging_memory;
  uint32_t vertex_buffer_size = 0;
  uint32_t index_buffer_size = 0;
  for (uint32_t mesh_i = scene_resource_details_.mesh_count;
       mesh_i < mesh_count; mesh_i++) {
    const Mesh* mesh = scene_->meshes_[mesh_i];
    uint32_t vertex_size = static_cast<uint32_t>(sizeof(mesh->vertices[0]) *
                                               mesh->vertices.size());
    uint32_t index_size =
        static_cast<uint32_t>(sizeof(mesh->indices[0]) * mesh->indices.size());
    vertex_buffer_size += vertex_size;
    index_buffer_size += index_size;
  }
  CreateBuffer(vertex_staging_buffer, vertex_staging_memory, vertex_buffer_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  CreateBuffer(index_staging_buffer, index_staging_memory, index_buffer_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void* vertex_data;
  void* index_data;
  vkMapMemory(device_, vertex_staging_memory, 0, VK_WHOLE_SIZE, 0, &vertex_data);
  vkMapMemory(device_, index_staging_memory, 0, VK_WHOLE_SIZE, 0, &index_data);
  size_t vertex_prefix = scene_resource_details_.vertex_count;
  size_t vertex_buffer_prefix = 0;
  size_t index_prefix = scene_resource_details_.index_count;
  size_t index_buffer_prefix = 0;
  for (uint32_t mesh_i = scene_resource_details_.mesh_count;
       mesh_i < mesh_count; mesh_i++) {
    Mesh* mesh = scene_->meshes_[mesh_i];
    size_t mesh_vertices = mesh->vertices.size();
    size_t mesh_indices = mesh->indices.size();
    size_t mesh_size = sizeof(Vertex) * mesh_vertices;
    size_t index_size = sizeof(uint32_t) * mesh_indices;
    memcpy(reinterpret_cast<char*>(vertex_data) + vertex_buffer_prefix, mesh->vertices.data(), mesh_size);
    memcpy(reinterpret_cast<char*>(index_data) + index_buffer_prefix,
           mesh->indices.data(), index_size);
    vertex_buffer_prefix += mesh_size;
    index_buffer_prefix += index_size;
    vertex_prefix += mesh_vertices;
    index_prefix += mesh_indices;
  }
  vkUnmapMemory(device_, vertex_staging_memory);
  VkBufferCopy vertex_buffer_cp{};
  vertex_buffer_cp.srcOffset = 0;
  vertex_buffer_cp.dstOffset = sizeof(Vertex)*scene_resource_details_.vertex_count;
  vertex_buffer_cp.size = vertex_buffer_size;
  vkCmdCopyBuffer(cmd, vertex_staging_buffer, vertex_buffer_, 1,
                  &vertex_buffer_cp);
  VkBufferCopy index_buffer_cp{};
  index_buffer_cp.srcOffset = 0;
  index_buffer_cp.dstOffset =
      sizeof(uint32_t) * scene_resource_details_.index_count;
  index_buffer_cp.size = index_buffer_size;
  vkCmdCopyBuffer(cmd, index_staging_buffer, index_buffer_, 1,
                  &index_buffer_cp);
  EndSingleUseCommandBuffer(cmd);

  for (uint32_t mesh_i = scene_resource_details_.mesh_count;
       mesh_i < mesh_count; mesh_i++) {
    if (mesh_i > 0) {
      const Mesh* mesh = scene_->meshes_[mesh_i-1];
      uint32_t mesh_size = static_cast<uint32_t>(mesh->vertices.size());
      uint32_t index_size = static_cast<uint32_t>(mesh->indices.size());
      scene_resource_details_.index_offsets_.push_back(
          scene_resource_details_.index_offsets_.back() + index_size);
      scene_resource_details_.vertex_offsets_.push_back(
          scene_resource_details_.vertex_offsets_.back() + mesh_size);
    } else {
      scene_resource_details_.vertex_offsets_.push_back(0);
      scene_resource_details_.index_offsets_.push_back(0);
    }
  }
  scene_resource_details_.vertex_count = static_cast<uint32_t>(vertex_prefix);
  scene_resource_details_.index_count = static_cast<uint32_t>(index_prefix);
  scene_resource_details_.mesh_count =
      static_cast<uint32_t>(scene_->meshes_.size());
  vkDestroyBuffer(device_, vertex_staging_buffer, nullptr);
  vkFreeMemory(device_, vertex_staging_memory, nullptr);
  vkDestroyBuffer(device_, index_staging_buffer, nullptr);
  vkFreeMemory(device_, index_staging_memory, nullptr);
}
void Application::Renderer::LoadTextures() {
  uint32_t tex_count = static_cast<uint32_t>(scene_->textures_.size());
  TextureImporter texture_importer;
  if (scene_resource_details_.texture_count == tex_count) return;
  for (uint32_t tex_i = scene_resource_details_.texture_count;
       tex_i < tex_count; tex_i++) {
    bool read_success =
        texture_importer.ReadFile(scene_->textures_[tex_i]->path_);
    ASSERT(read_success, "Could not load texture file!");
    const TextureData* tex_data = texture_importer.GetData();
    size_t tex_size =
        (tex_data->height) * (tex_data->width) * (tex_data->channels);
    // Staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    CreateBuffer(staging_buffer, staging_memory, tex_size,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // Copy texture data to staging buffer
    void* data;
    vkMapMemory(device_, staging_memory, 0, VK_WHOLE_SIZE, 0, &data);
    memcpy(data, tex_data->data, tex_size);
    vkUnmapMemory(device_, staging_memory);

    // Staging image
    VkImage staging_image = VK_NULL_HANDLE;
    VkDeviceMemory staging_image_memory;
    CreateImage(
        staging_image, staging_image_memory, 0, VK_FORMAT_R8G8B8A8_SRGB,
        {tex_data->width, tex_data->height, 1}, 1, 1,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkBufferImageCopy buffer_cp{};
    buffer_cp.bufferOffset = 0;
    buffer_cp.bufferRowLength = 0;
    buffer_cp.bufferImageHeight = 0;
    buffer_cp.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_cp.imageSubresource.mipLevel = 0;
    buffer_cp.imageSubresource.baseArrayLayer = 0;
    buffer_cp.imageSubresource.layerCount = 1;
    buffer_cp.imageOffset = {0};
    buffer_cp.imageExtent.width = tex_data->width;
    buffer_cp.imageExtent.height = tex_data->height;
    buffer_cp.imageExtent.depth = 1;

    // Copy texture data to staging image
    VkCommandBuffer cmd;
    TransitionImageLayout(
        staging_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    BeginSingleUseCommandBuffer(cmd);
    vkCmdCopyBufferToImage(cmd, staging_buffer, staging_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &buffer_cp);
    EndSingleUseCommandBuffer(cmd);
    TransitionImageLayout(
        staging_image, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT);

    // Blit texture to every mip map
    TransitionImageLayout(texture_images_[tex_i], VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                          VK_ACCESS_TRANSFER_WRITE_BIT);
    BeginSingleUseCommandBuffer(cmd);
    std::vector<VkImageBlit> blits(Scene::kMaxTextureMipLevels);
    for (uint32_t mip_i = 0; mip_i < Scene::kMaxTextureMipLevels; mip_i++) {
      VkImageBlit& blit = blits[mip_i];
      blit.srcOffsets[0] = {0, 0, 0};
      blit.srcOffsets[1] = {static_cast<int32_t>(tex_data->width),
                            static_cast<int32_t>(tex_data->height), 1};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.srcSubresource.layerCount = 1;
      blit.srcSubresource.mipLevel = 0;
      blit.dstOffsets[0] = {0, 0, 0};
      int32_t mip_dim = Scene::kMaxTextureResolution / (1 << mip_i);
      blit.dstOffsets[1] = {mip_dim, mip_dim, 1};
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.baseArrayLayer = 0;
      blit.dstSubresource.layerCount = 1;
      blit.dstSubresource.mipLevel = mip_i;
    }
    vkCmdBlitImage(cmd, staging_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   texture_images_[tex_i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   Scene::kMaxTextureMipLevels, blits.data(), VK_FILTER_LINEAR);
    EndSingleUseCommandBuffer(cmd);
    TransitionImageLayout(
        texture_images_[tex_i], VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    vkFreeMemory(device_, staging_image_memory, nullptr);
    vkDestroyImage(device_, staging_image, nullptr);
    vkFreeMemory(device_,staging_memory,nullptr);
    vkDestroyBuffer(device_,staging_buffer,nullptr);
  }
  scene_resource_details_.texture_count = tex_count;
}
void Application::Renderer::LoadCubemaps() {
  static const std::string faces[] = {"posx", "negx", "posy",
                                      "negy", "posz", "negz"};
  uint32_t cmap_count = static_cast<uint32_t>(scene_->cubemaps_.size());
  if (scene_resource_details_.cubemap_count == cmap_count) return;
  TextureImporter texture_importer;
  for (uint32_t cmap_i = scene_resource_details_.cubemap_count;
       cmap_i < cmap_count; cmap_i++) {
    TransitionImageLayout(cubemap_images_[cmap_i], VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                          VK_ACCESS_TRANSFER_WRITE_BIT);
    for (uint32_t face_i = 0; face_i < 6; face_i++) {
      std::string face_path =
          scene_->cubemaps_[cmap_i]->path_ + "/" + faces[face_i] + ".jpg";
      bool read_success = texture_importer.ReadFile(face_path);
      ASSERT(read_success, "Failed to read image: " + face_path);
      const TextureData* tex_data = texture_importer.GetData();
      size_t tex_size =
          (tex_data->height) * (tex_data->width) * (tex_data->channels);

      // Staging buffer
      VkBuffer staging_buffer;
      VkDeviceMemory staging_memory;
      CreateBuffer(staging_buffer, staging_memory, tex_size,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      // Copy texture data to staging buffer
      void* data;
      vkMapMemory(device_, staging_memory, 0, VK_WHOLE_SIZE, 0, &data);
      memcpy(data, tex_data->data, tex_size);
      vkUnmapMemory(device_, staging_memory);

      // Staging image
      VkImage staging_image = VK_NULL_HANDLE;
      VkDeviceMemory staging_image_memory;
      CreateImage(
          staging_image, staging_image_memory, 0, VK_FORMAT_R8G8B8A8_SRGB,
          {tex_data->width, tex_data->height, 1}, 1, 1,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      VkBufferImageCopy buffer_cp{};
      buffer_cp.bufferOffset = 0;
      buffer_cp.bufferRowLength = 0;
      buffer_cp.bufferImageHeight = 0;
      buffer_cp.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      buffer_cp.imageSubresource.mipLevel = 0;
      buffer_cp.imageSubresource.baseArrayLayer = 0;
      buffer_cp.imageSubresource.layerCount = 1;
      buffer_cp.imageOffset = {0};
      buffer_cp.imageExtent.width = tex_data->width;
      buffer_cp.imageExtent.height = tex_data->height;
      buffer_cp.imageExtent.depth = 1;

      // Copy texture data to staging image
      VkCommandBuffer cmd;
      TransitionImageLayout(
          staging_image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
          VK_ACCESS_TRANSFER_WRITE_BIT);
      BeginSingleUseCommandBuffer(cmd);
      vkCmdCopyBufferToImage(cmd, staging_buffer, staging_image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                             &buffer_cp);
      EndSingleUseCommandBuffer(cmd);
      TransitionImageLayout(
          staging_image, VK_IMAGE_ASPECT_COLOR_BIT,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
          VK_ACCESS_TRANSFER_READ_BIT);

      // Blit texture to every mip map
      BeginSingleUseCommandBuffer(cmd);
      std::vector<VkImageBlit> blits(Scene::kMaxTextureMipLevels);
      for (uint32_t mip_i = 0; mip_i < Scene::kMaxTextureMipLevels; mip_i++) {
        VkImageBlit& blit = blits[mip_i];
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {static_cast<int32_t>(tex_data->width),
                              static_cast<int32_t>(tex_data->height), 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.mipLevel = 0;
        blit.dstOffsets[0] = {0, 0, 0};
        int32_t mip_dim = Scene::kMaxTextureResolution / (1 << mip_i);
        blit.dstOffsets[1] = {mip_dim, mip_dim, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.baseArrayLayer = face_i;
        blit.dstSubresource.layerCount = 1;
        blit.dstSubresource.mipLevel = mip_i;
      }
      vkCmdBlitImage(
          cmd, staging_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          cubemap_images_[cmap_i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          Scene::kMaxTextureMipLevels, blits.data(), VK_FILTER_LINEAR);
      EndSingleUseCommandBuffer(cmd);

      vkFreeMemory(device_, staging_image_memory, nullptr);
      vkDestroyImage(device_, staging_image, nullptr);
      vkFreeMemory(device_, staging_memory, nullptr);
      vkDestroyBuffer(device_, staging_buffer, nullptr);
    }
    TransitionImageLayout(
        cubemap_images_[cmap_i], VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
  }
  scene_resource_details_.cubemap_count = cmap_count;
}
void Application::Renderer::CreateVertexBuffer() {
  CreateBuffer(
      vertex_buffer_, vertex_memory_, sizeof(Vertex)*Scene::kMaxVertices,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}
void Application::Renderer::CreateIndexBuffer() {
  CreateBuffer(
      index_buffer_, index_memory_, sizeof(uint32_t) * Scene::kMaxVertices,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}
void Application::Renderer::CreateDirectionalLightUniformBuffer() {
  uint32_t num_frames = frame_count_;
  directional_light_uniform_buffers_.resize(num_frames);
  directional_light_uniform_memory_.resize(num_frames);
  for (uint32_t frame_i = 0;
       frame_i < num_frames; frame_i++) {
    CreateBuffer(directional_light_uniform_buffers_[frame_i],
                 directional_light_uniform_memory_[frame_i],
                 DirectionalLightUniform::GetSize(),
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }
}
void Application::Renderer::UnloadScene() {
  ASSERT(scene_ != nullptr, "No scene loaded!");
  vkDeviceWaitIdle(device_);
  scene_ = nullptr;
}
void Application::Renderer::DrawScene(uint32_t frame_i, uint32_t image_i) {
  LoadSceneResources();
  VkCommandBuffer& cmd = command_buffers_[frame_i];
  VkDeviceSize vertex_offsets[] = {static_cast<uint64_t>(0)};
  vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer_, vertex_offsets);
  vkCmdBindIndexBuffer(cmd, index_buffer_, 0, VK_INDEX_TYPE_UINT32);

  SceneDrawDetails details;
  details.push_constants.world_to_view_transform = glm::mat4(1.0f);
  details.push_constants.view_to_clip_transform = glm::mat4(1.0f);
  details.directional_light_uniform.light_count_ = 0;
  details.material_uniform_block.material_count_ =
      static_cast<uint32_t>(scene_->materials_.size());
  for (uint32_t mat_i = 0;
       mat_i < details.material_uniform_block.material_count_; mat_i++) {
    const Material* mat = scene_->materials_[mat_i];
    MaterialUniform& mat_uniform =
        details.material_uniform_block.materials_[mat_i];
    mat_uniform.albedo = glm::vec4(mat->albedo_, 1.0f);
    mat_uniform.reflectance = mat->reflectance_;
    mat_uniform.metallic = mat->metallic_;
    mat_uniform.roughness = mat->roughness_;
    mat_uniform.albedo_texture_id = mat->albedo_texture_id_;
    mat_uniform.metallic_texture_id = mat->metallic_texture_id_;
    mat_uniform.roughness_texture_id = mat->roughness_texture_id_;
    mat_uniform.normal_texture_id = mat->normal_texture_id_;
  }
  details.skybox_uniform.specular_cubemap_id =
      scene_->skyboxes_[0]->specular_cubemap_id_;
  details.skybox_uniform.diffuse_cubemap_id =
      scene_->skyboxes_[0]->diffuse_cubemap_id_;
  details.skybox_uniform.specular_intensity =
      scene_->skyboxes_[0]->specular_intensity_;
  details.skybox_uniform.diffuse_intensity =
      scene_->skyboxes_[0]->diffuse_intensity_;
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

  DrawSceneZPrePass(cmd, image_i, details);

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          graphics_pipeline_layout_, 0, 1,
                          &descriptor_sets_[frame_i], 0, nullptr);
  BeginGraphicsRenderPass(cmd, image_i);

  void* uniform_data;
  vkMapMemory(device_, directional_light_uniform_memory_[frame_i], 0, VK_WHOLE_SIZE, 0,
              &uniform_data);
  size_t light_array_size =
      Scene::kMaxDirectionalLights * sizeof(DirectionalLight);
  memcpy(uniform_data, details.directional_light_uniform.lights_.data(),
         light_array_size);
  memcpy(static_cast<char*>(uniform_data)+light_array_size, &details.directional_light_uniform.light_count_,
         sizeof(uint32_t));
  vkUnmapMemory(device_, directional_light_uniform_memory_[frame_i]);

  void* material_uniform_data;
  vkMapMemory(device_, material_uniform_memory_[frame_i], 0, VK_WHOLE_SIZE, 0,
              &material_uniform_data);
  size_t material_array_size = Scene::kMaxMaterials * sizeof(MaterialUniform);
  memcpy(material_uniform_data, details.material_uniform_block.materials_.data(), material_array_size);
  memcpy(static_cast<char*>(material_uniform_data) + material_array_size,
         &details.material_uniform_block.material_count_, sizeof(uint32_t));
  vkUnmapMemory(device_, material_uniform_memory_[frame_i]);

  void* skybox_uniform_data;
  vkMapMemory(device_, skybox_uniform_memory_[frame_i], 0, VK_WHOLE_SIZE, 0,
              &skybox_uniform_data);
  memcpy(skybox_uniform_data, &details.skybox_uniform, sizeof(SkyboxUniform));
  vkUnmapMemory(device_, skybox_uniform_memory_[frame_i]);

  vkCmdBindVertexBuffers(cmd, 0, 1, &skybox_vertex_buffer_, vertex_offsets);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline_);

  vkCmdDraw(cmd, 6, 1, 0, 0);

  vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer_, vertex_offsets);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

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
      vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh->indices.size()), 1,
                       scene_resource_details_.index_offsets_[mesh_id],
                       scene_resource_details_.vertex_offsets_[mesh_id], 0);
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
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmap_pipeline_);
  for (uint32_t shadow_i = 0;
       shadow_i < details.directional_light_uniform.light_count_; shadow_i++) {
    DirectionalLight light =
        details.directional_light_uniform.lights_[shadow_i];
    vkCmdPushConstants(cmd, shadowmap_pipeline_layout_,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       offsetof(PushConstantData, world_to_view_transform),
                       sizeof(light.world_to_light_transform),
                       &light.world_to_light_transform);
    vkCmdPushConstants(
        cmd, shadowmap_pipeline_layout_,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       offsetof(PushConstantData, view_to_clip_transform),
                       sizeof(light.light_to_clip_transform),
                       &light.light_to_clip_transform);
    BeginShadowmapRenderPass(cmd, shadowmap_framebuffers_[swapchain_image_i][shadow_i]);
    DrawSceneMeshes(cmd, shadowmap_pipeline_layout_, details, scene_->root_, glm::mat4(1.0f));
    vkCmdEndRenderPass(cmd);
  }
}
void Application::Renderer::DrawSceneZPrePass(VkCommandBuffer& cmd,
                                              uint32_t swapchain_image_i,
                                              SceneDrawDetails& details) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, depthmap_pipeline_);
  BeginDepthmapRenderPass(cmd, swapchain_image_i);
  DrawSceneMeshes(cmd, depthmap_pipeline_layout_, details, scene_->root_,
                  glm::mat4(1.0f));
  vkCmdEndRenderPass(cmd);
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
        glm::mat4 light_to_world_transform = model_transform;
        // Directional lights should not scale the world
        light_to_world_transform[0] =
            glm::normalize(light_to_world_transform[0]);
        light_to_world_transform[1] =
            glm::normalize(light_to_world_transform[1]);
        light_to_world_transform[2] =
            glm::normalize(light_to_world_transform[2]);

        details.directional_light_uniform.lights_[light_count].world_to_light_transform =
            glm::inverse(light_to_world_transform);
        details.directional_light_uniform.lights_[light_count]
            .light_to_clip_transform = light_object->GetViewToClipTransform();
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