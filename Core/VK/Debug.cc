/**
 * @brief Debug for Vulkan
 */

#include "VK/Debug.h"

#include <boost/assert.hpp>
#include <iostream>

namespace Debug {
static VKAPI_ATTR VkBool32 VKAPI_CALL Callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
  std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}
} // namespace Debug

//*--------------------------------------------------------------------------------
// Create & Destroy
//*--------------------------------------------------------------------------------

VkResult
DebugMessenger::Create(VkInstance instance,
                       const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                       const VkAllocationCallbacks *pAllocator,
                       VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DebugMessenger::Destroy(VkInstance instance,
                             VkDebugUtilsMessengerEXT debugMessenger,
                             const VkAllocationCallbacks *pAllocator) {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

//*--------------------------------------------------------------------------------
// Setup & Cleanup
//*--------------------------------------------------------------------------------

void DebugMessenger::Setup(VkInstance instance) {
  VkDebugUtilsMessengerCreateInfoEXT info = ExtractCreateInfo();
  if (Create(instance, &info, nullptr, &msg) != VK_SUCCESS) {
    BOOST_ASSERT_MSG(false, "Failed to set up debug messenger");
  }
}

void DebugMessenger::Cleanup(VkInstance instance) {
  Destroy(instance, msg, nullptr);
}

VkDebugUtilsMessengerCreateInfoEXT DebugMessenger::ExtractCreateInfo() {
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = Debug::Callback;
  return createInfo;
}
