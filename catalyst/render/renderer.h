#pragma once
#include <vector>
#include <optional>
#include <string>

#include <vulkan/vulkan.h>

#include <catalyst/application/application.h>

namespace catalyst {
class Application :: Renderer {
 public:
  Renderer(Application* app);
  void StartUp();
  void LoadScene(const Scene& scene);
  void UnloadScene();
  void Update();
  void EarlyShutDown();
  void LateShutDown();

 private:
  class QueueFamilyIndexCollection {
   public:
    std::optional<uint32_t> graphics_queue_index_;
    std::optional<uint32_t> present_queue_index_;
    std::optional<uint32_t> compute_queue_index_;
    std::optional<uint32_t> transfer_queue_index_;

    bool IsComplete();
    std::vector<uint32_t> GetUniqueIndices();
  };
  struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
  };
  struct ComputeDetails {
    uint32_t subgroup_size;
    uint32_t workgroup_size;
  };
  struct PushConstantData {
    glm::mat4 model_to_world_transform;
    glm::mat4 world_to_view_transform;
    glm::mat4 view_to_clip_transform;
    uint32_t material_id;
  };
  struct ComputePushConstantData {
    int input_dim;
    bool input0;
  };
  struct DirectionalLight {
    alignas(16) glm::mat4 world_to_light_transform;
    alignas(16) glm::mat4 light_to_clip_transform;
    alignas(16) glm::vec3 color;
  };
  class DirectionalLightUniform {
   public:
    std::vector<DirectionalLight> lights_;
    uint32_t light_count_;
    DirectionalLightUniform();
    static size_t GetSize();
  };
  struct MaterialUniform {
    glm::vec4 albedo;
    float reflectance;
    float metallic;
    float roughness;
    int albedo_texture_id;
    int metallic_texture_id;
    int roughness_texture_id;
    int normal_texture_id;
    int ao_texture_id;
  };
  class MaterialUniformBlock {
   public:
    std::vector<MaterialUniform> materials_;
    uint32_t material_count_;
    MaterialUniformBlock();
    static size_t GetSize();
  };
  struct SkyboxUniform {
    int specular_cubemap_id;
    int diffuse_cubemap_id;
    float specular_intensity;
    float diffuse_intensity;
  };
  struct TonemappingUniform {
    float log_illuminance_sum;
    uint32_t num_pixels;
    float exposure_adjustment_;
    float _pad;
  };
  struct SsrUniform {
    float step_size;
    float thickness;
    float _pad[2];
  };
  struct SceneDrawDetails {
    PushConstantData push_constants;
    DirectionalLightUniform directional_light_uniform;
    MaterialUniformBlock material_uniform_block;
    SkyboxUniform skybox_uniform;
    TonemappingUniform tonemap_uniform;
    SsrUniform ssr_uniform;
    uint32_t debugdraw_offset_;
  };
  struct SceneResourceDetails {
    uint32_t mesh_count;
    uint32_t texture_count;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t cubemap_count;
    std::vector<uint32_t> vertex_offsets_;
    std::vector<uint32_t> index_offsets_;
  };

#ifndef NDEBUG
  static const bool debug_enabled_ = true;
#else
  static const bool debug_enabled_ = false;
#endif

  Application* app_;

  VkInstance instance_;

  VkPhysicalDevice physical_device_;
  VkDevice device_;
  VkPhysicalDeviceMemoryProperties mem_props_;

  Window *window_;
  VkSurfaceKHR surface_;
  VkSwapchainKHR swapchain_;
  VkExtent2D swapchain_extent_;
  VkExtent2D half_swapchain_extent_;
  VkFormat swapchain_image_format_;
  uint32_t frame_count_;
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_image_views_;
  VkFormat depth_format_;
  std::vector<VkDeviceMemory> depth_memory_;
  std::vector<VkImage> depth_images_;
  std::vector<VkImageView> depth_image_views_;
  VkFormat ssao_format_;
  std::vector<VkDeviceMemory> ssao_memory_;
  std::vector<VkImage> ssao_images_;
  std::vector<VkImageView> ssao_image_views_;
  VkFormat hdr_format_;
  std::vector<VkDeviceMemory> hdr_memory_;
  std::vector<VkImage> hdr_images_;
  std::vector<VkImageView> hdr_image_views_;
  VkDeviceMemory ssn_memory_;
  VkFormat ssn_format_;
  VkImage ssn_image_;
  VkImageView ssn_image_view_;
  VkSampler texture_sampler_;
  VkSampler shadowmap_sampler_;

  VkPipelineCache pipeline_cache_;
  VkPipelineLayout graphics_pipeline_layout_;
  VkPipelineLayout debugdraw_pipeline_layout_;
  VkPipelineLayout shadowmap_pipeline_layout_;
  VkPipelineLayout depthmap_pipeline_layout_;
  VkPipelineLayout ssao_pipeline_layout_;
  VkPipelineLayout hdr_pipeline_layout_;
  VkPipeline graphics_pipeline_;
  VkPipeline debugdraw_pipeline_;
  VkPipeline debugdraw_lines_pipeline_;
  VkPipeline shadowmap_pipeline_;
  VkPipeline skybox_pipeline_;
  VkPipeline depthmap_pipeline_;
  VkPipeline ssao_pipeline_;
  VkPipeline hdr_pipeline_;
  VkRenderPass render_pass_;
  VkRenderPass debugdraw_render_pass_;
  VkRenderPass shadowmap_render_pass_;
  VkRenderPass depthmap_render_pass_;
  VkRenderPass ssao_render_pass_;
  VkRenderPass hdr_render_pass_;
  std::vector<VkFramebuffer> framebuffers_;
  std::vector<std::vector<VkFramebuffer>> shadowmap_framebuffers_;
  std::vector<VkFramebuffer> depthmap_framebuffers_;
  std::vector<VkFramebuffer> ssao_framebuffers_;
  std::vector<VkFramebuffer> hdr_framebuffers_;

  VkDescriptorSetLayout descriptor_set_layout_;
  VkDescriptorSetLayout ssao_descriptor_set_layout_;
  VkDescriptorSetLayout hdr_descriptor_set_layout_;
  VkDescriptorPool descriptor_pool_;
  std::vector<VkDescriptorSet> descriptor_sets_;
  std::vector<VkDescriptorSet> ssao_descriptor_sets_;
  std::vector<VkDescriptorSet> hdr_descriptor_sets_;

  VkCommandPool command_pool_;
  std::vector<VkCommandBuffer> command_buffers_;
  std::vector<VkSemaphore> image_acquired_semaphores_;
  std::vector<VkSemaphore> image_presented_semaphores_;
  std::vector<VkFence> in_flight_fences_;

  const Scene* scene_;
  SceneResourceDetails scene_resource_details_;
  VkDeviceMemory vertex_memory_;
  VkBuffer vertex_buffer_;
  VkDeviceMemory index_memory_;
  VkBuffer index_buffer_;
  std::vector<VkDeviceMemory> directional_light_uniform_memory_;
  std::vector<VkBuffer> directional_light_uniform_buffers_;
  std::vector<std::vector<VkDeviceMemory>> shadowmap_memory_;
  std::vector<std::vector<VkImage>> shadowmap_images_;
  std::vector<std::vector<VkImageView>> shadowmap_image_views_;
  std::vector<VkBuffer> debugdraw_buffer_;
  std::vector<VkDeviceMemory> debugdraw_memory_;
  std::vector<VkBuffer> material_uniform_buffers_;
  std::vector<VkDeviceMemory> material_uniform_memory_;
  std::vector<VkDeviceMemory> texture_memory_;
  std::vector<VkImage> texture_images_;
  std::vector<VkImageView> texture_image_views_;
  std::vector<VkDeviceMemory> cubemap_memory_;
  std::vector<VkImage> cubemap_images_;
  std::vector<VkImageView> cubemap_image_views_;
  std::vector<VkDeviceMemory> skybox_uniform_memory_;
  std::vector<VkBuffer> skybox_uniform_buffers_;
  VkDeviceMemory skybox_vertex_memory_;
  VkBuffer skybox_vertex_buffer_;
  VkDeviceMemory ssao_sample_memory_;
  VkBuffer ssao_sample_uniform_;
  std::vector<VkDeviceMemory> hdr_tonemapping_memory_;
  std::vector<VkBuffer> hdr_tonemapping_buffers_;

  std::vector<bool> rendered_frames_;
  VkFormat ssr_format_;
  std::vector<VkImage> ssr_images_;
  std::vector<VkDeviceMemory> ssr_memory_;
  std::vector<VkImageView> ssr_image_views_;
  std::vector<VkFramebuffer> ssr_framebuffers_;
  VkDescriptorSetLayout ssr_descriptor_set_layout_;
  std::vector<VkDescriptorSet> ssr_descriptor_sets_;
  VkRenderPass ssr_render_pass_;
  VkPipelineLayout ssr_pipeline_layout_;
  VkPipeline ssr_pipeline_;
  std::vector<VkBuffer> ssr_uniform_;
  std::vector<VkDeviceMemory> ssr_uniform_memory_;

  std::vector<VkImage> billboard_images_;
  std::vector<VkImageView> billboard_image_views_;
  std::vector<VkDeviceMemory> billboard_memory_;
  VkDescriptorSetLayout debugdraw_descriptor_set_layout_;
  std::vector<VkDescriptorSet> debugdraw_descriptor_sets_;

  QueueFamilyIndexCollection queue_family_indices_;
  VkQueue graphics_queue_;
  VkQueue compute_queue_;
  VkQueue transfer_queue_;
  VkQueue present_queue_;

  ComputeDetails compute_details_;
  std::vector<std::vector<VkBuffer>> illuminance_buffers_;
  std::vector<std::vector<VkDeviceMemory>> illuminance_memory_;
  VkPipelineLayout illuminance_pipeline_layout_;
  VkPipeline log_illuminance_pipeline_;
  VkPipeline reduce_illuminance_pipeline_;
  VkDescriptorSetLayout illuminance_descriptor_set_layout_;
  std::vector<VkDescriptorSet> illuminance_descriptor_sets_;

  void CreateInstance();

  // Device Creation: rendermanager_device.cc
  void SelectPhysicalDevice();
  bool CheckPhysicalDeviceSuitability(VkPhysicalDevice physical_device);
  bool CheckPhysicalDeviceExtensionSupport(VkPhysicalDevice physical_device);
  QueueFamilyIndexCollection FindQueueFamilyIndices(
      VkPhysicalDevice physical_device);
  void CheckComputeDetails();
  void CreateLogicalDevice();
  void CreateDevice();

  // Surface Handling: rendermanager_surface.cc
  SwapchainSupportDetails CheckSwapchainSupport(
      VkPhysicalDevice physical_device);
  VkSurfaceFormatKHR SelectSwapSurfaceFormat(
      std::vector<VkSurfaceFormatKHR> formats);
  VkFormat SelectFormat(const std::vector<VkFormat>& candidates,
                        VkImageTiling req_tiling, VkFormatFeatureFlags req_flags);
  VkFormat SelectDepthFormat();
  VkPresentModeKHR SelectSwapPresentMode(
      std::vector<VkPresentModeKHR> available_present_modes);
  VkExtent2D SelectSwapExtent(VkSurfaceCapabilitiesKHR capabilities);
  void CreateSwapchain();
  void CreateDepthResources();
  void DestroySwapchain();
  void RecreateSwapchain();

  // Rendering Pipeline - Generic
  VkShaderModule CreateShaderModule(const std::vector<char>& buffer);
  void CreateSamplers();
  void CreatePipelines(bool include_fixed_size = true);
  void CreateRenderPasses(bool include_fixed_size = true);
  void CreateFramebuffers(bool include_fixed_size = true);

  // Rendering Pipeline - Graphics
  void CreatePipelineCache();
  void CreateGraphicsPipeline();
  void CreateGraphicsRenderPass();
  void CreateGraphicsFramebuffers();
  void BeginGraphicsRenderPass(VkCommandBuffer& cmd,
                               uint32_t swapchain_image_i);

  // Rendering Pipeline - Debug Draw
  void CreateDebugDrawResources();
  void CreateDebugDrawRenderPass();
  void CreateDebugDrawPipeline();
  void CreateDebugDrawLinesPipeline();
  void BeginDebugDrawRenderPass(VkCommandBuffer& cmd,
                                uint32_t swapchain_image_i);

  // Rendering Pipeline - Shadowmaps
  void CreateShadowmapRenderPass();
  void CreateShadowmapPipeline();
  void CreateDirectionalShadowmapFramebuffers();
  void BeginShadowmapRenderPass(VkCommandBuffer& cmd,
                                VkFramebuffer& framebuffer);

  // Rendering Pipeline - Depthmap
  void CreateDepthmapRenderPass();
  void CreateDepthmapPipeline();
  void CreateDepthmapFramebuffers();
  void BeginDepthmapRenderPass(VkCommandBuffer& cmd, uint32_t frame_i);

  // Rendering Pipeline - Skybox
  void CreateSkyboxPipeline();

  // Rendering Pipeline - SSAO
  void CreateSsaoResources();
  void CreateSsaoRenderPass();
  void CreateSsaoPipeline();
  void CreateSsaoFramebuffers();
  void BeginSsaoRenderPass(VkCommandBuffer& cmd, uint32_t swapchain_image_i);

  // Rendering Pipeline - SSR
  void CreateSsrResources();
  void CreateSsrRenderPass();
  void CreateSsrPipeline();
  void CreateSsrFramebuffers();
  void BeginSsrRenderPass(VkCommandBuffer& cmd, uint32_t swapchain_image_i);
  void ComputeSsrMap(VkCommandBuffer& cmd, uint32_t image_i, SceneDrawDetails& details);

  // Rendering Pipeline - HDR
  void CreateHdrResources();
  void CreateHdrRenderPass();
  void CreateHdrPipeline();
  void CreateHdrFramebuffers();
  void BeginHdrRenderPass(VkCommandBuffer& cmd, uint32_t swapchain_image_i);

  // Compute Pipeline - Illuminance
  void CreateIlluminanceResources();
  void CreateIlluminancePipelines();
  void ComputeTonemapping(VkCommandBuffer& cmd, uint32_t swapchain_image_i, SceneDrawDetails& details);

  // Needed for each window, can be in rendermanager_surface.cc
  void CreateCommandPool();
  void CreateCommandBuffers();
  void CreateSyncObjects();

  // Descriptor management
  void CreateDescriptorSetLayout();
  void CreateDescriptorPool();
  void CreateDescriptorSets();
  void WriteFixedSizeDescriptorSets();
  void WriteResizeableDescriptorSets();

  // Fixed Size Resources
  void CreateVertexBuffer();
  void CreateIndexBuffer();
  void CreateDirectionalLightUniformBuffer();
  void CreateDirectionalShadowmapResources();
  void CreateMaterialUniformBuffer();
  void CreateTextureResources();
  void CreateCubemapResources();
  void CreateSkyboxResources();
  void CreateBillboardResources();

  // Scene Resources
  void LoadSceneResources();
  void LoadMeshes();
  void LoadTextures();
  void LoadCubemaps();

  // Utilities - renderer_utilities.cc
  static std::vector<char> ReadFile(const std::string& path);
  uint32_t SelectMemoryType(const VkMemoryRequirements& mem_reqs,
                            const VkMemoryPropertyFlags& req_props);
  void CreateBuffer(VkBuffer& buffer, VkDeviceMemory& memory,
                    const VkDeviceSize& size, const VkBufferUsageFlags& usage,
                    const VkMemoryPropertyFlags& req_props);
  void CreateImage(VkImage& image, VkDeviceMemory& memory,
                   VkImageCreateFlags flags, const VkFormat format,
                   const VkExtent3D extent, const uint32_t mip_levels,
                   const uint32_t array_layers, const VkImageUsageFlags usage,
                   const VkMemoryPropertyFlags req_props);
  void CreateImageView(VkImageView& image_view, VkImage& image,
                       VkImageViewType type, const VkFormat format,
                       const VkImageAspectFlags aspect_flags);
  void TransitionImageLayout(VkImage& image,
                             const VkImageAspectFlagBits image_aspect,
                             const VkImageLayout initial_layout,
                             const VkImageLayout final_layout,
                             const VkPipelineStageFlagBits src_scope,
                             const VkPipelineStageFlagBits dst_scope,
                             const VkAccessFlags src_access_mask,
                             const VkAccessFlags dst_access_mask);
  void BeginSingleUseCommandBuffer(VkCommandBuffer& cmd);
  void EndSingleUseCommandBuffer(VkCommandBuffer& cmd);

  // Draw Commands
  void DrawFrame();
  void DrawScene(uint32_t image_i);
  void DrawScenePrePass(VkCommandBuffer& cmd, SceneDrawDetails& details, const SceneObject* focus,
                    glm::mat4 model_transform);
  void DrawSceneMeshes(VkCommandBuffer& cmd, VkPipelineLayout& layout, SceneDrawDetails& details,
                       const SceneObject* focus, glm::mat4 model_transform);
  void DebugDrawScene(uint32_t image_i, SceneDrawDetails& details);
  void DebugDrawSceneAabb(uint32_t image_i, const Aabb& aabb, SceneDrawDetails& details);
  void DebugDrawSceneBillboard(uint32_t image_i,
                               const DebugDrawBillboard* billboard,
                               SceneDrawDetails& details);
  void DrawSceneShadowmaps(VkCommandBuffer& cmd, uint32_t swapchain_image_i, SceneDrawDetails& details);
  void DrawSceneZPrePass(VkCommandBuffer& cmd, uint32_t swapchain_image_i,
                         SceneDrawDetails& details);

  // Debug Messenger for Vulkan Validation Layers
  static void PopulateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT& debug_ci);
  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData);

  // Uncopyable
  Renderer(const Renderer& a) = delete;
  const Renderer& operator=(const Renderer& a) = delete;
};
}