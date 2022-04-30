#pragma warning(disable : 26812)
#include <catalyst/render/renderer.h>

#include <algorithm>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <catalyst/dev/dev.h>

namespace catalyst {
Application::Renderer::Renderer(Application* app) {
  app_ = app;
  scene_ = nullptr;
}
void Application::Renderer::StartUp() {
  CreateInstance();

  // Window interaction
  window_ = app_->main_window;
  window_->SetVkInstance(instance_);
  surface_ = window_->GetVkSurface();

  CreateDevice();
  CreateSwapchain();
  CreateDepthResources();
  CreateShadowmapResources();

  // One-time resources
  CreateDebugDrawResources();

  CreateDescriptorSetLayout();
  CreateDescriptorPool();
  CreateDescriptorSets();

  CreatePipelineCache();
  CreateSampler();
  CreateRenderPasses();
  CreatePipelines();
  CreateFramebuffers();

  // Prerequisites to draw the first frame, need one set of these per window
  CreateCommandPool();
  CreateCommandBuffers();
  CreateSyncObjects();
}
void Application::Renderer::Update() { DrawFrame(); }
void Application::Renderer::EarlyShutDown() {
  vkDeviceWaitIdle(device_);
  UnloadScene();
  DestroySwapchain();
}
void Application::Renderer::LateShutDown() {
  vkDeviceWaitIdle(device_);
  // Destroy Sync Objects
  for (VkSemaphore sem : image_acquired_semaphores_)
    vkDestroySemaphore(device_, sem, nullptr);
  for (VkSemaphore sem : image_presented_semaphores_)
    vkDestroySemaphore(device_, sem, nullptr);
  for (VkFence fence : in_flight_fences_)
    vkDestroyFence(device_, fence, nullptr);
  
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    vkDestroyBuffer(device_, debugdraw_buffer_[frame_i], nullptr);
    vkFreeMemory(device_, debugdraw_memory_[frame_i], nullptr);
  }
  debugdraw_buffer_.clear();
  debugdraw_memory_.clear();
  vkFreeCommandBuffers(device_, command_pool_,
                       static_cast<uint32_t>(command_buffers_.size()),
                       command_buffers_.data());
  vkDestroyCommandPool(device_, command_pool_, nullptr);
  vkDestroySampler(device_, sampler_, nullptr);
  vkDestroyPipelineCache(device_, pipeline_cache_, nullptr);
  vkDestroyDevice(device_, nullptr);
  vkDestroyInstance(instance_, nullptr);
}
void Application::Renderer::CreateInstance() {
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Catata";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "Catalyst";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo instance_ci{};
  instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_ci.pApplicationInfo = &app_info;

  std::vector<const char*> instance_extensions;
  uint32_t required_extension_count = 0;
  const char **required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
  for (uint32_t ext_i = 0; ext_i < required_extension_count; ext_i++)
    instance_extensions.push_back(required_extensions[ext_i]);
  std::vector<const char*> instance_layers;
  if (debug_enabled_) {
    instance_extensions.push_back("VK_EXT_debug_utils");
    instance_layers.push_back("VK_LAYER_KHRONOS_validation");
  }

  instance_ci.enabledExtensionCount =
      static_cast<uint32_t>(instance_extensions.size());
  instance_ci.ppEnabledExtensionNames = instance_extensions.data();
  instance_ci.enabledLayerCount = static_cast<uint32_t>(instance_layers.size());
  instance_ci.ppEnabledLayerNames = instance_layers.data();

  if(debug_enabled_){
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_ci{};

    PopulateDebugMessengerCreateInfo(debug_messenger_ci);
    instance_ci.pNext =
        (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_ci;
    // Synchronization Validation
    VkValidationFeatureEnableEXT enables[] = {
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};
    VkValidationFeaturesEXT features = {};
    features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    features.enabledValidationFeatureCount = 1;
    features.pEnabledValidationFeatures = enables;
    ((VkDebugUtilsMessengerCreateInfoEXT*)(instance_ci.pNext))->pNext =
        (VkValidationFeatureEnableEXT*)&features;
  }

  VkResult create_result = vkCreateInstance(&instance_ci, nullptr, &instance_);
  ASSERT(create_result==VK_SUCCESS, "Failed to create Vulkan Instance!");
}
void Application::Renderer::PopulateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& debug_messenger_ci) {
  debug_messenger_ci = {};
  debug_messenger_ci.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_messenger_ci.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug_messenger_ci.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debug_messenger_ci.pfnUserCallback = DebugCallback;
}
VKAPI_ATTR VkBool32 VKAPI_CALL Application::Renderer::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  }

  return VK_FALSE;
}
bool Application::Renderer::QueueFamilyIndexCollection::IsComplete() {
  return graphics_queue_index_.has_value() &&
         present_queue_index_.has_value() && compute_queue_index_.has_value() &&
         transfer_queue_index_.has_value();
}
std::vector<uint32_t>
Application::Renderer::QueueFamilyIndexCollection::GetUniqueIndices() {
  std::vector<uint32_t> result;
  if (graphics_queue_index_.has_value())
    result.push_back(graphics_queue_index_.value());
  if (compute_queue_index_.has_value())
    result.push_back(compute_queue_index_.value());
  if (transfer_queue_index_.has_value())
    result.push_back(transfer_queue_index_.value());
  if (present_queue_index_.has_value())
    result.push_back(present_queue_index_.value());

  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}
}  // namespace catalyst