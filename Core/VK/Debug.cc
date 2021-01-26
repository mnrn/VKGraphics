/**
 * @brief Debug for Vulkan
 */

#include "VK/Debug.h"

#include <boost/assert.hpp>
#include <iostream>
#include <sstream>

namespace Debug {
static VKAPI_ATTR VkBool32 VKAPI_CALL
Callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
         VkDebugUtilsMessageTypeFlagsEXT,
         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
  // コールバックに渡されるフラグに応じてプレフィックスを選択します。
  std::string prefix;
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    prefix = "VERBOSE: ";
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    prefix = "INFO: ";
  } else if (messageSeverity &
             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    prefix = "WARNING: ";
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    prefix = "ERROR: ";
  }

  // メッセージをデフォルト出力に表示します。
  std::stringstream debugMessage;
  debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "]["
               << pCallbackData->pMessageIdName
               << "] : " << pCallbackData->pMessage;
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    std::cerr << debugMessage.str() << std::endl;
  } else {
    std::cout << debugMessage.str() << std::endl;
  }
  fflush(stdout);

  // このコールバックの戻り値は検証メッセージの原因となったVulkan呼び出しを中止するかどうかを制御します。
  // ここでは、検証メッセージを中止させるVulkan呼び出しを望まないため、VK_FALSEを返します。
  // 代わりに呼び出しを中止する場合は、VK_TRUEを渡すと、関数はVK_ERROR_VALIDATION_FAILED_EXTを返します。
  return VK_FALSE;
}
} // namespace Debug


//*-----------------------------------------------------------------------------
// Create & Destroy
//*-----------------------------------------------------------------------------

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

//*-----------------------------------------------------------------------------
// Setup & Cleanup
//*-----------------------------------------------------------------------------

void DebugMessenger::Setup(VkInstance instance) {
  VkDebugUtilsMessengerCreateInfoEXT info = ExtractCreateInfo();
  VkResult result = Create(instance, &info, nullptr, &msg);
  BOOST_ASSERT_MSG(result == VK_SUCCESS, "Failed to set up debug messenger");
}

void DebugMessenger::Cleanup(VkInstance instance) const {
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
