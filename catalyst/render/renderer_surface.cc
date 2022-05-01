#include <catalyst/render/renderer.h>

#include <array>
#include <algorithm>
#include <fstream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <catalyst/time/timemanager.h>
#include <catalyst/dev/dev.h>

namespace catalyst {
Application::Renderer::SwapchainSupportDetails Application::Renderer::CheckSwapchainSupport(
    VkPhysicalDevice device) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_,
                                            &details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count,
                                       nullptr);

  if (format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count,
                                         details.formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_,
                                            &present_mode_count, nullptr);

  if (present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface_, &present_mode_count, details.present_modes.data());
  }

  return details;
}
VkSurfaceFormatKHR Application::Renderer::SelectSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> formats) {
  for (const auto& available_format : formats) {
    if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available_format;
    }
  }

  return formats[0];
}
VkPresentModeKHR Application::Renderer::SelectSwapPresentMode(
    std::vector<VkPresentModeKHR> available_present_modes) {
  for (const auto& present_mode : available_present_modes) {
    if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return present_mode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D Application::Renderer::SelectSwapExtent(
    VkSurfaceCapabilitiesKHR capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    std::pair<uint32_t, uint32_t> window_extent = window_->GetExtent();
    uint32_t width = 0, height = 0;
    std::tie(width, height) = window_extent;
    VkExtent2D actual_extent = {width, height};

    actual_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actual_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actual_extent;
  }
}
void Application::Renderer::CreateSwapchain() {
  SwapchainSupportDetails swapchain_support =
      CheckSwapchainSupport(physical_device_);

  VkSurfaceFormatKHR surface_format =
      SelectSwapSurfaceFormat(swapchain_support.formats);
  VkPresentModeKHR present_mode =
      SelectSwapPresentMode(swapchain_support.present_modes);
  VkExtent2D extent = SelectSwapExtent(swapchain_support.capabilities);

  uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
  if (swapchain_support.capabilities.maxImageCount > 0 &&
      image_count > swapchain_support.capabilities.maxImageCount) {
    image_count = swapchain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapchain_ci{};
  swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_ci.surface = surface_;

  swapchain_ci.minImageCount = image_count;
  swapchain_ci.imageFormat = surface_format.format;
  swapchain_ci.imageColorSpace = surface_format.colorSpace;
  swapchain_ci.imageExtent = extent;
  swapchain_ci.imageArrayLayers = 1;
  swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queue_family_indices[] = {
      queue_family_indices_.graphics_queue_index_.value(),
      queue_family_indices_.present_queue_index_.value()};

  if (queue_family_indices_.graphics_queue_index_ !=
      queue_family_indices_.graphics_queue_index_) {
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_ci.queueFamilyIndexCount = 2;
    swapchain_ci.pQueueFamilyIndices = queue_family_indices;
  } else {
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = nullptr;
  }

  swapchain_ci.preTransform = swapchain_support.capabilities.currentTransform;
  swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_ci.presentMode = present_mode;
  swapchain_ci.clipped = VK_TRUE;

  VkResult create_result =
      vkCreateSwapchainKHR(device_, &swapchain_ci, nullptr, &swapchain_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create swap chain!");

  vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
  swapchain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(device_, swapchain_, &image_count,
                          swapchain_images_.data());

  swapchain_image_format_ = surface_format.format;
  swapchain_extent_ = extent;

  swapchain_image_views_.clear();
  for (VkImage swapchain_image : swapchain_images_) {
    VkImageViewCreateInfo image_view_ci{};
    image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_ci.image = swapchain_image;
    image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_ci.format = swapchain_image_format_;
    image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 1;
    VkImageView image_view;
    VkResult create_result =
        vkCreateImageView(device_, &image_view_ci, nullptr, &image_view);
    ASSERT(create_result == VK_SUCCESS,
           "Failed to create swapchain image view!");
    swapchain_image_views_.push_back(image_view);
  }
  frame_count_ = static_cast<uint32_t>(swapchain_images_.size());
}
VkFormat Application::Renderer::SelectFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling req_tiling,
    VkFormatFeatureFlags req_flags) {
  for (VkFormat format : candidates) {
    VkFormatProperties format_props{};
    vkGetPhysicalDeviceFormatProperties(physical_device_,format, &format_props);
    VkFormatFeatureFlags format_features = 0;
    switch (req_tiling) {
      case VK_IMAGE_TILING_LINEAR:
        format_features = format_props.linearTilingFeatures;
        break;
      case VK_IMAGE_TILING_OPTIMAL:
        format_features = format_props.optimalTilingFeatures;
        break;
      default:
        ASSERT(false, "Unhandled image tiling scheme!");
        break;
    }
    if ((req_flags & format_features) == req_flags) return format;
  }
  ASSERT(false, "Could not find suitable format!");
  return candidates[0];
}
VkFormat Application::Renderer::SelectDepthFormat() {
  return SelectFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
}
void Application::Renderer::CreateDepthResources() {
  depth_format_ = SelectDepthFormat();
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    VkImage depth_image;
    VkImageCreateInfo image_ci{};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.pNext = nullptr;
    image_ci.flags = 0;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = depth_format_;
    image_ci.extent.width = swapchain_extent_.width;
    image_ci.extent.height = swapchain_extent_.height;
    image_ci.extent.depth = 1;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkResult create_result =
        vkCreateImage(device_, &image_ci, nullptr, &depth_image);
    ASSERT(create_result == VK_SUCCESS, "Failed to create depth image!");
    depth_images_.push_back(depth_image);

    VkDeviceMemory depth_mem;
    VkMemoryRequirements image_reqs;
    vkGetImageMemoryRequirements(device_, depth_image, &image_reqs);
    uint32_t mem_type = SelectMemoryType(image_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkMemoryAllocateInfo image_ai{};
    image_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    image_ai.pNext = nullptr;
    image_ai.memoryTypeIndex = mem_type;
    image_ai.allocationSize = image_reqs.size;
    VkResult alloc_result =
        vkAllocateMemory(device_, &image_ai, nullptr, &depth_mem);
    ASSERT(alloc_result == VK_SUCCESS, "Failed to allocate depth memory!");
    depth_memory_.push_back(depth_mem);
    VkResult bind_result =
        vkBindImageMemory(device_, depth_image, depth_mem, 0);
    ASSERT(bind_result == VK_SUCCESS, "Failed to bind depth memory!");

    VkImageView depth_iv;
    VkImageViewCreateInfo view_ci{};
    view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_ci.pNext = nullptr;
    view_ci.flags = 0;
    view_ci.image = depth_image;
    view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_ci.format = depth_format_;
    view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depth_format_ == VK_FORMAT_D24_UNORM_S8_UINT)
      view_ci.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    view_ci.subresourceRange.baseMipLevel = 0;
    view_ci.subresourceRange.levelCount = 1;
    view_ci.subresourceRange.baseArrayLayer = 0;
    view_ci.subresourceRange.layerCount = 1;
    create_result = vkCreateImageView(device_, &view_ci, nullptr, &depth_iv);
    ASSERT(create_result == VK_SUCCESS, "Failed to create depth image view!");
    depth_image_views_.push_back(depth_iv);
  }
}
void Application::Renderer::CreateDirectionalShadowmapResources() {
  shadowmap_images_.resize(frame_count_);
  shadowmap_image_views_.resize(frame_count_);
  shadowmap_memory_.resize(frame_count_);
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    for (uint32_t shadowmap_i = 0; shadowmap_i < Scene::kMaxDirectionalLights;
         shadowmap_i++) {
      VkImage depth_image;
      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.pNext = nullptr;
      image_ci.flags = 0;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = depth_format_;
      image_ci.extent.width = Scene::kMaxShadowmapResolution;
      image_ci.extent.height = Scene::kMaxShadowmapResolution;
      image_ci.extent.depth = 1;
      image_ci.mipLevels = 1;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                       VK_IMAGE_USAGE_SAMPLED_BIT;
      image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      VkResult create_result =
          vkCreateImage(device_, &image_ci, nullptr, &depth_image);
      ASSERT(create_result == VK_SUCCESS, "Failed to create depth image!");
      shadowmap_images_[frame_i].push_back(depth_image);

      VkDeviceMemory depth_mem;
      VkMemoryRequirements image_reqs;
      vkGetImageMemoryRequirements(device_, depth_image, &image_reqs);
      uint32_t mem_type =
          SelectMemoryType(image_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VkMemoryAllocateInfo image_ai{};
      image_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      image_ai.pNext = nullptr;
      image_ai.memoryTypeIndex = mem_type;
      image_ai.allocationSize = image_reqs.size;
      VkResult alloc_result =
          vkAllocateMemory(device_, &image_ai, nullptr, &depth_mem);
      ASSERT(alloc_result == VK_SUCCESS, "Failed to allocate depth memory!");
      shadowmap_memory_[frame_i].push_back(depth_mem);
      VkResult bind_result =
          vkBindImageMemory(device_, depth_image, depth_mem, 0);
      ASSERT(bind_result == VK_SUCCESS, "Failed to bind depth memory!");

      VkImageView depth_iv;
      VkImageViewCreateInfo view_ci{};
      view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_ci.pNext = nullptr;
      view_ci.flags = 0;
      view_ci.image = depth_image;
      view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_ci.format = depth_format_;
      view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      if (depth_format_ == VK_FORMAT_D24_UNORM_S8_UINT)
        view_ci.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      view_ci.subresourceRange.baseMipLevel = 0;
      view_ci.subresourceRange.levelCount = 1;
      view_ci.subresourceRange.baseArrayLayer = 0;
      view_ci.subresourceRange.layerCount = 1;
      create_result = vkCreateImageView(device_, &view_ci, nullptr, &depth_iv);
      ASSERT(create_result == VK_SUCCESS, "Failed to create depth image view!");
      shadowmap_image_views_[frame_i].push_back(depth_iv);
    }
  }
  // Transition shadowmaps
  VkCommandBuffer cmd;
  BeginSingleUseCommandBuffer(cmd);
  std::vector<VkImageMemoryBarrier> image_barriers;
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    for (uint32_t shadow_i = 0; shadow_i < Scene::kMaxDirectionalLights;
         shadow_i++) {
      VkImageMemoryBarrier image_barrier{};
      image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      image_barrier.pNext = nullptr;
      image_barrier.srcAccessMask = 0;
      image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      image_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      image_barrier.srcQueueFamilyIndex =
          queue_family_indices_.graphics_queue_index_.value();
      image_barrier.dstQueueFamilyIndex =
          queue_family_indices_.graphics_queue_index_.value();
      image_barrier.image = shadowmap_images_[frame_i][shadow_i];
      image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      image_barrier.subresourceRange.baseArrayLayer = 0;
      image_barrier.subresourceRange.baseMipLevel = 0;
      image_barrier.subresourceRange.layerCount = 1;
      image_barrier.subresourceRange.levelCount = 1;
      image_barriers.push_back(image_barrier);
    }
  }
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, static_cast<uint32_t>(image_barriers.size()),
                       image_barriers.data());
  EndSingleUseCommandBuffer(cmd);
}
void Application::Renderer::DestroySwapchain() {
  vkDeviceWaitIdle(device_);

  // Destroy framebuffers
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++)
    vkDestroyFramebuffer(device_, framebuffers_[frame_i], nullptr);
  framebuffers_.clear();

  // Destroy pipelines and render passes
  vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
  vkDestroyPipeline(device_, debugdraw_pipeline_, nullptr);
  vkDestroyPipeline(device_, debugdraw_lines_pipeline_, nullptr);
  vkDestroyRenderPass(device_, render_pass_, nullptr);

  // Destroy resources
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    vkDestroyImageView(device_, depth_image_views_[frame_i], nullptr);
    vkFreeMemory(device_, depth_memory_[frame_i], nullptr);
    vkDestroyImage(device_, depth_images_[frame_i], nullptr);
    vkDestroyImageView(device_, swapchain_image_views_[frame_i], nullptr);
  }
  depth_image_views_.clear();
  depth_memory_.clear();
  depth_images_.clear();

  vkDestroySwapchainKHR(device_, swapchain_, nullptr);
  swapchain_ = VK_NULL_HANDLE;
}
void Application::Renderer::RecreateSwapchain() {
  DestroySwapchain();
  CreateSwapchain();
  CreateDepthResources();
  CreateRenderPasses(false);
  CreatePipelines(false);
  CreateFramebuffers(false);
}
void Application::Renderer::CreatePipelineCache() {
  VkPipelineCacheCreateInfo pipeline_cache_ci{};
  pipeline_cache_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  pipeline_cache_ci.pNext = nullptr;
  pipeline_cache_ci.flags = 0;
  pipeline_cache_ci.initialDataSize = 0;
  pipeline_cache_ci.pInitialData = nullptr;
  VkResult create_result = vkCreatePipelineCache(device_, &pipeline_cache_ci,
                                                  nullptr, &pipeline_cache_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create pipeline cache!");
}
void Application::Renderer::CreatePipelines(bool include_fixed_size) {
  CreateGraphicsPipeline();
  CreateDebugDrawPipeline();
  CreateDebugDrawLinesPipeline();
  if (include_fixed_size) CreateDepthmapPipeline();
}
void Application::Renderer::CreateRenderPasses(bool include_fixed_size) {
  CreateGraphicsRenderPass();
  if(include_fixed_size) CreateDepthmapRenderPass();
}
void Application::Renderer::CreateFramebuffers(bool include_fixed_size) {
  CreateGraphicsFramebuffers();
  if (include_fixed_size) CreateDirectionalShadowmapFramebuffers();
}
std::vector<char> Application::Renderer::ReadFile(const std::string& path) {

    std::ifstream file(path, std::ios::ate | std::ios::binary);

    ASSERT(file.is_open(), "Could not open file: " + path + "!");

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule Application::Renderer::CreateShaderModule(
  const std::vector<char>& buffer) {
  size_t file_size = buffer.size();
  VkShaderModuleCreateInfo shader_ci{};
  shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_ci.pNext = nullptr;
  shader_ci.flags = 0;
  shader_ci.codeSize = file_size;
  shader_ci.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
  VkShaderModule shader;
  VkResult create_result =
      vkCreateShaderModule(device_, &shader_ci, nullptr, &shader);
  return shader;
}

void Application::Renderer::CreateSampler() {
  VkSamplerCreateInfo sampler_ci{};
  sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_ci.pNext = nullptr;
  sampler_ci.flags = 0;
  sampler_ci.magFilter = VK_FILTER_LINEAR;
  sampler_ci.minFilter = VK_FILTER_LINEAR;
  sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler_ci.mipLodBias = 0.0;
  sampler_ci.anisotropyEnable = VK_TRUE;
  sampler_ci.maxAnisotropy = 1.0f;
  sampler_ci.minLod = 0.0f;
  sampler_ci.maxLod = 1.0f;
  sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  sampler_ci.unnormalizedCoordinates = VK_FALSE;
  VkResult create_result =
      vkCreateSampler(device_, &sampler_ci, nullptr, &sampler_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create sampler!");
}

void Application::Renderer::CreateCommandPool() {
  VkCommandPoolCreateInfo command_pool_ci{};
  command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_ci.queueFamilyIndex =
      queue_family_indices_.graphics_queue_index_.value();

  VkResult create_result =
      vkCreateCommandPool(device_, &command_pool_ci, nullptr, &command_pool_);
  ASSERT(create_result == VK_SUCCESS,
         "Failed to create graphics command pool!");
}
void Application::Renderer::CreateCommandBuffers() {
  command_buffers_.resize(frame_count_);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = command_pool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)command_buffers_.size();

  VkResult alloc_result =
      vkAllocateCommandBuffers(device_, &allocInfo, command_buffers_.data());
  ASSERT(alloc_result == VK_SUCCESS, "Failed to allocated command buffers!");
}
void Application::Renderer::CreateSyncObjects() {
  image_acquired_semaphores_.resize(frame_count_);
  image_presented_semaphores_.resize(frame_count_);
  in_flight_fences_.resize(frame_count_);

  VkSemaphoreCreateInfo sem_ci{};
  sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  sem_ci.pNext = nullptr;
  sem_ci.flags = 0;
  VkFenceCreateInfo fence_ci{};
  fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_ci.pNext = nullptr;
  fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for (uint32_t frame_i = 0; frame_i < frame_count_; frame_i++) {
    vkCreateSemaphore(device_, &sem_ci, nullptr,
                      &image_acquired_semaphores_[frame_i]);
    vkCreateSemaphore(device_, &sem_ci, nullptr, &image_presented_semaphores_[frame_i]);
    vkCreateFence(device_, &fence_ci, nullptr, &in_flight_fences_[frame_i]);
  }
}
void Application::Renderer::CreateGraphicsRenderPass() {
  VkAttachmentDescription color_attachment{};
  color_attachment.format = swapchain_image_format_;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depth_attachment{};
  depth_attachment.format = depth_format_;
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  std::array<VkAttachmentDescription, 2> attachments = {color_attachment,depth_attachment};
  VkRenderPassCreateInfo render_pass_ci{};
  render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
  render_pass_ci.pAttachments = attachments.data();
  render_pass_ci.subpassCount = 1;
  render_pass_ci.pSubpasses = &subpass;
  render_pass_ci.dependencyCount = 0;
  render_pass_ci.pDependencies = nullptr;

  VkResult create_result = vkCreateRenderPass(
      device_, &render_pass_ci, nullptr, &render_pass_);

  ASSERT(create_result == VK_SUCCESS, "Failed to create render pass!");
}
void Application::Renderer::CreateGraphicsPipeline() {
  const std::vector<char> vert_shader_code =
      ReadFile("../assets/shaders/phong.vert.spv");
  const std::vector<char> frag_shader_code =
      ReadFile("../assets/shaders/phong.frag.spv");

  VkShaderModule vert_shader =
      CreateShaderModule(vert_shader_code);
  VkShaderModule frag_shader =
      CreateShaderModule(frag_shader_code);

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
  vertex_bd.stride = sizeof(Vertex);

  VkVertexInputAttributeDescription vertex_pos_ad{};
  vertex_pos_ad.binding = 0;
  vertex_pos_ad.format = VK_FORMAT_R32G32B32_SFLOAT;
  vertex_pos_ad.location = 0;
  vertex_pos_ad.offset = 0;

  VkVertexInputAttributeDescription vertex_norm_ad{};
  vertex_norm_ad.binding = 0;
  vertex_norm_ad.format = VK_FORMAT_R32G32B32_SFLOAT;
  vertex_norm_ad.location = 1;
  vertex_norm_ad.offset = offsetof(Vertex, normal);

  VkVertexInputAttributeDescription vertex_uv_ad{};
  vertex_uv_ad.binding = 0;
  vertex_uv_ad.format = VK_FORMAT_R32G32_SFLOAT;
  vertex_uv_ad.location = 2;
  vertex_uv_ad.offset = offsetof(Vertex, uv);

  VkVertexInputAttributeDescription vertex_ads[] = {vertex_pos_ad,vertex_norm_ad,vertex_uv_ad};

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount = 3;
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
  multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
  depth_stencil_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_state.depthTestEnable = VK_TRUE;
  depth_stencil_state.depthWriteEnable = VK_TRUE;
  depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
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

  VkPipelineLayoutCreateInfo graphics_pipeline_layout_ci{};
  graphics_pipeline_layout_ci.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  graphics_pipeline_layout_ci.setLayoutCount = 1;
  graphics_pipeline_layout_ci.pSetLayouts = &descriptor_set_layout_;
  graphics_pipeline_layout_ci.pushConstantRangeCount = 1;
  graphics_pipeline_layout_ci.pPushConstantRanges = &vertex_push;

  VkResult create_result = vkCreatePipelineLayout(
      device_, &graphics_pipeline_layout_ci, nullptr,
      &graphics_pipeline_layout_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create pipeline layout!");

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

  create_result = vkCreateGraphicsPipelines(
      device_, pipeline_cache_, 1, &pipeline_ci,
      nullptr, &graphics_pipeline_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create graphics pipeline!");

  vkDestroyShaderModule(device_, vert_shader, nullptr);
  vkDestroyShaderModule(device_, frag_shader, nullptr);
}
void Application::Renderer::CreateGraphicsFramebuffers() {
  framebuffers_.resize(frame_count_);

  for (size_t i = 0; i < frame_count_; i++) {
    std::array<VkImageView, 2> attachments = {swapchain_image_views_[i], depth_image_views_[i]};

    VkFramebufferCreateInfo framebuffer_ci{};
    framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.renderPass = render_pass_;
    framebuffer_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebuffer_ci.pAttachments = attachments.data();
    framebuffer_ci.width = swapchain_extent_.width;
    framebuffer_ci.height = swapchain_extent_.height;
    framebuffer_ci.layers = 1;

    VkResult create_result = vkCreateFramebuffer(device_, &framebuffer_ci,
                                                 nullptr, &framebuffers_[i]);
    ASSERT(create_result == VK_SUCCESS, "Failed to create framebuffer!");
  }
}
void Application::Renderer::BeginGraphicsRenderPass(VkCommandBuffer& cmd, uint32_t swapchain_image_i) {
  VkClearValue color_clear;
  color_clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  VkClearValue depth_clear;
  depth_clear.depthStencil.depth = 1.0f;
  depth_clear.depthStencil.stencil = 0;
  VkClearValue clear_values[] = {color_clear, depth_clear};
  VkRenderPassBeginInfo render_pass_bi{};
  render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_bi.pNext = nullptr;
  render_pass_bi.renderPass = render_pass_;
  render_pass_bi.framebuffer = framebuffers_[swapchain_image_i];
  render_pass_bi.renderArea.extent = swapchain_extent_;
  render_pass_bi.renderArea.offset.x = 0;
  render_pass_bi.renderArea.offset.y = 0;
  render_pass_bi.clearValueCount = 2;
  render_pass_bi.pClearValues = clear_values;
  vkCmdBeginRenderPass(cmd, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
}
void Application::Renderer::DrawFrame() {
  static uint32_t frame_i = 0;
  uint32_t frame_count = static_cast<uint32_t>(framebuffers_.size());
  vkWaitForFences(device_, 1,
                  &in_flight_fences_[frame_i], VK_TRUE,
                  UINT64_MAX);
  vkResetFences(device_, 1,
                &in_flight_fences_[frame_i]);
  uint32_t image_i = 0;
  VkResult acquire_result = vkAcquireNextImageKHR(
      device_, swapchain_, UINT64_MAX,
      image_acquired_semaphores_[frame_i], VK_NULL_HANDLE,
      &image_i);
  if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
    RecreateSwapchain();
    return;
  }
  ASSERT(acquire_result == VK_SUCCESS || acquire_result == VK_SUBOPTIMAL_KHR,
         "Failed to acquire swapchain image!");

  VkCommandBuffer& cmd = command_buffers_[frame_i];
  vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

  VkCommandBufferBeginInfo cmd_bi{};
  cmd_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_bi.pNext = nullptr;
  cmd_bi.flags = 0;
  cmd_bi.pInheritanceInfo = nullptr;
  VkResult begin_result = vkBeginCommandBuffer(cmd, &cmd_bi);
  ASSERT(begin_result == VK_SUCCESS,
         "Failed to begin recording command buffer!");

  DrawScene(frame_i,image_i);
  VkResult end_result = vkEndCommandBuffer(cmd);
  ASSERT(end_result == VK_SUCCESS,
         "Failed to finish recording command buffer!");

  VkPipelineStageFlags semaphore_wait_stages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo cmd_si{};
  cmd_si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  cmd_si.pNext = nullptr;
  cmd_si.waitSemaphoreCount = 1;
  cmd_si.pWaitSemaphores = &image_acquired_semaphores_[frame_i];
  cmd_si.pWaitDstStageMask = &semaphore_wait_stages;
  cmd_si.commandBufferCount = 1;
  cmd_si.pCommandBuffers = &cmd;
  cmd_si.signalSemaphoreCount = 1;
  cmd_si.pSignalSemaphores =
      &image_presented_semaphores_[frame_i];
  vkQueueSubmit(graphics_queue_, 1, &cmd_si,
                in_flight_fences_[frame_i]);

  VkResult present_result = VK_ERROR_UNKNOWN;
  VkPresentInfoKHR frame_pi{};
  frame_pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  frame_pi.pNext = nullptr;
  frame_pi.waitSemaphoreCount = 1;
  frame_pi.pWaitSemaphores =
      &image_presented_semaphores_[frame_i];
  frame_pi.swapchainCount = 1;
  frame_pi.pSwapchains = &swapchain_;
  frame_pi.pImageIndices = &image_i;
  frame_pi.pResults = &present_result;
  vkQueuePresentKHR(present_queue_, &frame_pi);
  if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
      present_result == VK_SUBOPTIMAL_KHR) {
    RecreateSwapchain();
  } else {
    ASSERT(present_result == VK_SUCCESS, "Could not present frame!");
  }

  frame_i = (frame_i + 1) % frame_count;
}
}  // namespace catalyst