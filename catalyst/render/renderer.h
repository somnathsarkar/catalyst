#pragma once
#include <vector>
#include <optional>
#include <string>

#include <vulkan/vulkan.h>

#include <catalyst/application/application.h>

namespace catalyst {
class Application :: Renderer {
 public:
  static const uint32_t kMaxDebugDrawVertices = 1024;
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
  struct PushConstantData {
    glm::mat4 model_to_world_transform;
    glm::mat4 world_to_view_transform;
    glm::mat4 view_to_clip_transform;
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
  struct SceneDrawDetails {
    PushConstantData push_constants;
    DirectionalLightUniform directional_light_uniform;
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
  VkFormat swapchain_image_format_;
  uint32_t frame_count_;
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_image_views_;
  VkFormat depth_format_;
  std::vector<VkDeviceMemory> depth_memory_;
  std::vector<VkImage> depth_images_;
  std::vector<VkImageView> depth_image_views_;
  std::vector<std::vector<VkDeviceMemory>> shadowmap_memory_;
  std::vector<std::vector<VkImage>> shadowmap_images_;
  std::vector<std::vector<VkImageView>> shadowmap_image_views_;
  VkSampler sampler_;

  VkPipelineCache pipeline_cache_;
  VkPipelineLayout graphics_pipeline_layout_;
  VkPipelineLayout debugdraw_pipeline_layout_;
  VkPipelineLayout depthmap_pipeline_layout_;
  VkPipeline graphics_pipeline_;
  VkPipeline debugdraw_pipeline_;
  VkPipeline debugdraw_lines_pipeline_;
  VkPipeline depthmap_pipeline_;
  VkRenderPass render_pass_;
  VkRenderPass depthmap_render_pass_;
  std::vector<VkFramebuffer> framebuffers_;
  std::vector<std::vector<VkFramebuffer>> shadowmap_framebuffers_;

  VkDescriptorSetLayout descriptor_set_layout_;
  VkDescriptorPool descriptor_pool_;
  std::vector<VkDescriptorSet> descriptor_sets_;

  VkCommandPool command_pool_;
  std::vector<VkCommandBuffer> command_buffers_;
  std::vector<VkSemaphore> image_acquired_semaphores_;
  std::vector<VkSemaphore> image_presented_semaphores_;
  std::vector<VkFence> in_flight_fences_;

  const Scene* scene_;
  VkDeviceMemory vertex_memory_;
  VkBuffer vertex_buffer_;
  VkDeviceSize vertex_buffer_size_;
  std::vector<VkDeviceMemory> uniform_memory_;
  std::vector<VkBuffer> uniform_buffers_;
  VkDeviceSize uniform_buffer_size_;
  std::vector<VkBuffer> debugdraw_buffer_;
  std::vector<VkDeviceMemory> debugdraw_memory_;
  VkDeviceSize debugdraw_buffer_size_;

  QueueFamilyIndexCollection queue_family_indices_;
  VkQueue graphics_queue_;
  VkQueue compute_queue_;
  VkQueue transfer_queue_;
  VkQueue present_queue_;

  void CreateInstance();

  // Device Creation: rendermanager_device.cc
  void SelectPhysicalDevice();
  bool CheckPhysicalDeviceSuitability(VkPhysicalDevice physical_device);
  bool CheckPhysicalDeviceExtensionSupport(VkPhysicalDevice physical_device);
  QueueFamilyIndexCollection FindQueueFamilyIndices(
      VkPhysicalDevice physical_device);
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
  void CreateShadowmapResources();
  void DestroySwapchain();
  void RecreateSwapchain();

  // Rendering Pipeline - Generic
  VkShaderModule CreateShaderModule(const std::vector<char>& buffer);
  void CreateSampler();
  void CreatePipelines();
  void CreateRenderPasses();
  void CreateFramebuffers();

  // Rendering Pipeline - Graphics
  void CreatePipelineCache();
  void CreateGraphicsPipeline();
  void CreateGraphicsRenderPass();
  void CreateGraphicsFramebuffers();
  void BeginGraphicsRenderPass(VkCommandBuffer& cmd,
                               uint32_t swapchain_image_i);

  // Rendering Pipeline - Debug Draw
  void CreateDebugDrawResources();
  void CreateDebugDrawPipeline();
  void CreateDebugDrawLinesPipeline();

  // Rendering Pipeline - Depthmaps
  void CreateDepthmapRenderPass();
  void CreateDepthmapPipeline();
  void CreateShadowmapFramebuffers();
  void BeginDepthmapRenderPass(VkCommandBuffer& cmd, VkFramebuffer& framebuffer);

  // Needed for each window, can be in rendermanager_surface.cc
  void CreateCommandPool();
  void CreateCommandBuffers();
  void CreateSyncObjects();

  // Descriptor management
  void CreateDescriptorSetLayout();
  void CreateDescriptorPool();
  void CreateDescriptorSets();

  // Scene Management
  uint32_t SelectMemoryType(const VkMemoryRequirements& mem_reqs, const VkMemoryPropertyFlags& req_props);
  void CreateBuffer(VkBuffer& buffer, VkDeviceMemory& memory,
                    const VkDeviceSize& size, const VkBufferUsageFlags& usage,
                    const VkMemoryPropertyFlags& req_props);
  void BeginSingleUseCommandBuffer(VkCommandBuffer& cmd);
  void EndSingleUseCommandBuffer(VkCommandBuffer& cmd);
  void CreateVertexBuffer();
  void CreateDirectionalLightUniformBuffer();
  void CreateDirectionalLightShadowmapSampler();
  void WriteDescriptorSets();

  // Draw Commands
  void DrawFrame();
  void DrawScene(uint32_t frame_i, uint32_t image_i);
  void DrawScenePrePass(VkCommandBuffer& cmd, SceneDrawDetails& details, const SceneObject* focus,
                    glm::mat4 model_transform);
  void DrawSceneMeshes(VkCommandBuffer& cmd, VkPipelineLayout& layout, SceneDrawDetails& details,
                       const SceneObject* focus, glm::mat4 model_transform);
  void DebugDrawScene(uint32_t frame_i);
  void DebugDrawAabb(uint32_t frame_i, const Aabb& aabb);
  void DrawSceneShadowmaps(VkCommandBuffer& cmd, uint32_t swapchain_image_i, SceneDrawDetails& details);

  // Debug Messenger for Vulkan Validation Layers
  static void PopulateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT& debug_ci);
  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData);

  // Filesystem operations, maybe take out of render manager
  static std::vector<char> ReadFile(const std::string& path);

  // Uncopyable
  Renderer(const Renderer& a) = delete;
  const Renderer& operator=(const Renderer& a) = delete;
};
}