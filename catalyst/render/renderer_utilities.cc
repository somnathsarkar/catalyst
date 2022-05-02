#include <catalyst/render/renderer.h>

#include <fstream>

namespace catalyst {
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
uint32_t Application::Renderer::SelectMemoryType(
    const VkMemoryRequirements& mem_reqs,
    const VkMemoryPropertyFlags& req_props) {
  bool mem_found = false;
  uint32_t mem_index = -1;
  for (uint32_t mem_i = 0; mem_i < mem_props_.memoryTypeCount; mem_i++) {
    if ((mem_reqs.memoryTypeBits >> mem_i) & 1 &&
        (mem_props_.memoryTypes[mem_i].propertyFlags & req_props) ==
            req_props) {
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
  VkResult alloc_result =
      vkAllocateMemory(device_, &memory_ai, nullptr, &memory);
  ASSERT(alloc_result == VK_SUCCESS, "Failed to allocate memory!");

  VkResult bind_result = vkBindBufferMemory(device_, buffer, memory, 0);
  ASSERT(bind_result == VK_SUCCESS, "Failed to bind memory to buffer!");
}

void Application::Renderer::CreateImage(VkImage& image, VkDeviceMemory& memory,
                                        const VkFormat format,
                                        const VkExtent3D extent,
                                        const uint32_t mip_levels,
                                        const VkImageUsageFlags usage,
                                        const VkMemoryPropertyFlags req_props) {
  VkImageCreateInfo image_ci{};
  image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_ci.pNext = nullptr;
  image_ci.flags = 0;
  image_ci.imageType = VK_IMAGE_TYPE_2D;
  image_ci.format = format;
  image_ci.extent = extent;
  image_ci.mipLevels = mip_levels;
  image_ci.arrayLayers = 1;
  image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
  image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_ci.usage = usage;
  image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_ci.queueFamilyIndexCount = 1;
  image_ci.pQueueFamilyIndices = nullptr;
  image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkResult create_result = vkCreateImage(device_, &image_ci, nullptr, &image);
  ASSERT(create_result == VK_SUCCESS, "Failed to create image!");

  VkMemoryRequirements mem_reqs{};
  vkGetImageMemoryRequirements(device_, image, &mem_reqs);

  VkMemoryAllocateInfo image_ai{};
  image_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  image_ai.pNext = nullptr;
  image_ai.memoryTypeIndex = SelectMemoryType(mem_reqs, req_props);
  image_ai.allocationSize = mem_reqs.size;
  VkResult alloc_result =
      vkAllocateMemory(device_, &image_ai, nullptr, &memory);
  ASSERT(alloc_result == VK_SUCCESS, "Failed to allocate image memory!");
  VkResult bind_result = vkBindImageMemory(device_, image, memory, 0);
  ASSERT(bind_result == VK_SUCCESS, "Failed to bind image memory!");
}

void Application::Renderer::CreateImageView(
    VkImageView& image_view, VkImage& image, const VkFormat format,
    const VkImageAspectFlags aspect_flags) {
  VkImageViewCreateInfo view_ci{};
  view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_ci.pNext = nullptr;
  view_ci.flags = 0;
  view_ci.image = image;
  view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_ci.format = format;
  view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_ci.subresourceRange.aspectMask = aspect_flags;
  view_ci.subresourceRange.baseMipLevel = 0;
  view_ci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
  view_ci.subresourceRange.baseArrayLayer = 0;
  view_ci.subresourceRange.layerCount = 1;
  VkResult create_result =
      vkCreateImageView(device_, &view_ci, nullptr, &image_view);
  ASSERT(create_result == VK_SUCCESS, "Failed to create image view!");
}

void Application::Renderer::TransitionImageLayout(
    VkImage& image, const VkImageAspectFlagBits image_aspect,
    const VkImageLayout initial_layout, const VkImageLayout final_layout,
    const VkPipelineStageFlagBits src_scope,
    const VkPipelineStageFlagBits dst_scope,
    const VkAccessFlags src_access_mask, const VkAccessFlags dst_access_mask) {
  VkCommandBuffer cmd;
  BeginSingleUseCommandBuffer(cmd);
  VkImageMemoryBarrier image_barrier{};
  image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  image_barrier.pNext = nullptr;
  image_barrier.srcAccessMask = src_access_mask;
  image_barrier.dstAccessMask = dst_access_mask;
  image_barrier.oldLayout = initial_layout;
  image_barrier.newLayout = final_layout;
  image_barrier.srcQueueFamilyIndex =
      queue_family_indices_.graphics_queue_index_.value();
  image_barrier.dstQueueFamilyIndex =
      queue_family_indices_.graphics_queue_index_.value();
  image_barrier.image = image;
  image_barrier.subresourceRange.aspectMask = image_aspect;
  image_barrier.subresourceRange.baseArrayLayer = 0;
  image_barrier.subresourceRange.baseMipLevel = 0;
  image_barrier.subresourceRange.layerCount = 1;
  image_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
  vkCmdPipelineBarrier(cmd, src_scope, dst_scope, 0, 0, nullptr, 0, nullptr, 1,
                       &image_barrier);
  EndSingleUseCommandBuffer(cmd);
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
  ASSERT(begin_result == VK_SUCCESS,
         "Failed to begin recording single use command buffer!");
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
}