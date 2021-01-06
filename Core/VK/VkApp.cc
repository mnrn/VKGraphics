/**
 * @brief Vulkan Application
 */

//*--------------------------------------------------------------------------------
// Including files
//*--------------------------------------------------------------------------------

#include "VkApp.h"
#include <vulkan/vulkan.h>

#include <boost/assert.hpp>

#include "VK/Debug.h"

//*--------------------------------------------------------------------------------
// Constant expressions
//*--------------------------------------------------------------------------------

//*--------------------------------------------------------------------------------
// Init & Deinit
//*--------------------------------------------------------------------------------

void VkApp::OnCreate(const char *appName) {
  CreateInstance(appName);
  if (isEnableValidationLayers_) {
    debug_.Setup(instance_.Get());
  }
}

void VkApp::OnDestroy() {
  if (isEnableValidationLayers_) {
    debug_.Cleanup(instance_.Get());
  }
  instance_.Cleanup();
}

//*--------------------------------------------------------------------------------
// Create & Destroy
//*--------------------------------------------------------------------------------

void VkApp::CreateInstance(const char *appName) {
  VkApplicationInfo info{};
  info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  info.pApplicationName = appName;
  info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  info.pEngineName = "";
  info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create.pApplicationInfo = &info;

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);
  if (isEnableValidationLayers_) {
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  create.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create.ppEnabledExtensionNames = extensions.data();

  if (isEnableValidationLayers_) {
    create.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
    create.ppEnabledLayerNames = validationLayers_.data();
  } else {
    create.enabledLayerCount = 0;
  }

  if (vkCreateInstance(&create, nullptr, instance_.Set()) != VK_SUCCESS) {
    BOOST_ASSERT_MSG(false, "Failed to create instance!");
  }
}
