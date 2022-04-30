#include <catalyst/render/renderer.h>
#include <catalyst/dev/dev.h>

namespace catalyst {
void Application::Renderer::SelectPhysicalDevice() {
  uint32_t physical_device_count;
  std::vector<VkPhysicalDevice> physical_devices;
  vkEnumeratePhysicalDevices(instance_, &physical_device_count, nullptr);
  physical_devices.resize(physical_device_count);
  vkEnumeratePhysicalDevices(instance_, &physical_device_count,
                             physical_devices.data());

  // Choose Physical Device here
  bool physical_device_found = false;
  for (VkPhysicalDevice physical_device : physical_devices) {
    if (CheckPhysicalDeviceSuitability(physical_device)) {
      physical_device_found = true;
      physical_device_ = physical_device;
      break;
    }
  }
  ASSERT(physical_device_found, "Could not find suitable physical device!");

  // Find physical device memory properties
  vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_props_);
}
bool Application::Renderer::CheckPhysicalDeviceSuitability(
    VkPhysicalDevice physical_device) {
  QueueFamilyIndexCollection indices = FindQueueFamilyIndices(physical_device);

  bool extensions_supported =
      CheckPhysicalDeviceExtensionSupport(physical_device);

  bool swapchain_supported = false;
  if (extensions_supported) {
    SwapchainSupportDetails swapChainSupport = CheckSwapchainSupport(physical_device);
    swapchain_supported = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.present_modes.empty();
  }

  VkPhysicalDeviceFeatures supported_features;
  vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

  return indices.IsComplete() && extensions_supported &&
         supported_features.samplerAnisotropy &&
         supported_features.fillModeNonSolid && supported_features.wideLines;
}

bool Application::Renderer::CheckPhysicalDeviceExtensionSupport(VkPhysicalDevice physical_device) {
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                       &extension_count,
                                       nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &extension_count, available_extensions.data());

  bool extension_missing = false;
  std::vector<std::string> required_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  for (const std::string& required_extension : required_extensions) {
    bool extension_found = false;
    for (const VkExtensionProperties& extension : available_extensions) {
      if (extension.extensionName == required_extension) {
        extension_found = true;
        break;
      }
    }
    if (!extension_found) {
      extension_missing = true;
      break;
    }
  }

  return !extension_missing;
}
Application::Renderer::QueueFamilyIndexCollection
Application::Renderer::FindQueueFamilyIndices(VkPhysicalDevice physical_device) {
  QueueFamilyIndexCollection indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           queue_families.data());

  int queue_i = 0;
  for (const auto& queue_family : queue_families) {
    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphics_queue_index_ = queue_i;
    if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
      indices.compute_queue_index_ = queue_i;
    if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)
      indices.transfer_queue_index_ = queue_i;
    VkBool32 present_support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_i, surface_,
                                         &present_support);
    if (present_support) indices.present_queue_index_ = queue_i;
    queue_i++;
  }

  return indices;
}
void Application::Renderer::CreateLogicalDevice() {
  queue_family_indices_ = FindQueueFamilyIndices(physical_device_);

  std::vector<VkDeviceQueueCreateInfo> queue_cis;
  std::vector<uint32_t> unique_queue_family_indices =
      queue_family_indices_.GetUniqueIndices();
  float queue_priority = 1.0f;
  for (uint32_t queue_family_index : unique_queue_family_indices) {
    VkDeviceQueueCreateInfo queue_ci{};
    queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci.queueFamilyIndex = queue_family_index;
    queue_ci.queueCount = 1;
    queue_ci.pQueuePriorities = &queue_priority;
    queue_cis.push_back(queue_ci);
  }

  VkPhysicalDeviceFeatures device_features{};
  device_features.samplerAnisotropy = VK_TRUE;
  device_features.fillModeNonSolid = VK_TRUE;
  device_features.wideLines = VK_TRUE;

  VkDeviceCreateInfo device_ci{};
  device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  device_ci.queueCreateInfoCount = static_cast<uint32_t>(queue_cis.size());
  device_ci.pQueueCreateInfos = queue_cis.data();

  device_ci.pEnabledFeatures = &device_features;

  std::vector<const char*> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  device_ci.enabledExtensionCount =
      static_cast<uint32_t>(device_extensions.size());
  device_ci.ppEnabledExtensionNames = device_extensions.data();

  // Device Layers are deprecated
  device_ci.enabledLayerCount = 0;
  device_ci.ppEnabledLayerNames = nullptr;

  VkResult create_result =
      vkCreateDevice(physical_device_, &device_ci, nullptr, &device_);
  ASSERT(create_result == VK_SUCCESS, "Failed to create logical device!");

  vkGetDeviceQueue(device_, queue_family_indices_.graphics_queue_index_.value(), 0,
                   &graphics_queue_);
  vkGetDeviceQueue(device_, queue_family_indices_.compute_queue_index_.value(),
                   0, &compute_queue_);
  vkGetDeviceQueue(device_, queue_family_indices_.transfer_queue_index_.value(),
                   0, &transfer_queue_);
  vkGetDeviceQueue(device_, queue_family_indices_.present_queue_index_.value(), 0,
                   &present_queue_);
}
void Application::Renderer::CreateDevice() { static bool device_created = false;
  if (!device_created) {
    SelectPhysicalDevice();
    CreateLogicalDevice();
    device_created = true;
  }
}
};