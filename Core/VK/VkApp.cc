/**
 * @brief Vulkan Application
 */

//*--------------------------------------------------------------------------------
// Including files
//*--------------------------------------------------------------------------------

#include "VkApp.h"
#include <vulkan/vulkan.h>

#include <boost/assert.hpp>
#include <map>
#include <spdlog/spdlog.h>

#include "VK/Debug.h"

//*--------------------------------------------------------------------------------
// Constant expressions
//*--------------------------------------------------------------------------------

//*--------------------------------------------------------------------------------
// Init & Deinit
//*--------------------------------------------------------------------------------

void VkApp::OnCreate(const char *appName, int width, int height,
                     GLFWwindow *window) {
  CreateInstance(appName);
#if !defined(NDEBUG)
  debug_.Setup(instance_.Get());
#endif
  CreateSurface(window);
  SelectPhysicalDevice();
  CreateLogicalDevice();
  swapchain_.Create(instance_, width, height, false);
}

void VkApp::OnDestroy() {
  CleanupSwapchain();
#if !defined(NDEBUG)
  debug_.Cleanup(instance_.Get());
#endif
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
    spdlog::info("Required extensions:");
    for (const auto extension : extensions) {
      spdlog::info(" - {}", extension);
    }
  }
  create.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create.ppEnabledExtensionNames = extensions.data();

  if (isEnableValidationLayers_) {
    spdlog::info("Enabling valiadation");
    create.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
    create.ppEnabledLayerNames = validationLayers_.data();
  } else {
    create.enabledLayerCount = 0;
  }

  if (vkCreateInstance(&create, nullptr, instance_.Set()) != VK_SUCCESS) {
    BOOST_ASSERT_MSG(false, "Failed to create instance!");
  }
}

void VkApp::CreateSurface(GLFWwindow *window) {
  if (glfwCreateWindowSurface(instance_.Get(), window, nullptr,
                              &instance_.surface)) {
    BOOST_ASSERT_MSG(false, "Failed to create window surface!");
  }
}

void VkApp::SelectPhysicalDevice() {
  uint32_t size = 0;
  vkEnumeratePhysicalDevices(instance_.Get(), &size, nullptr);
  BOOST_ASSERT_MSG(size != 0, "Failed to find any physical device!");

  std::vector<VkPhysicalDevice> devices(size);
  vkEnumeratePhysicalDevices(instance_.Get(), &size, devices.data());
#if !defined(NDEBUG)
  spdlog::info("Found {} physical devices", size);
#endif
  std::multimap<float, VkPhysicalDevice> scores;
  for (const auto &device : devices) {
    scores.emplace(CalcDeviceScore(device), device);
  }
  BOOST_ASSERT_MSG(scores.rbegin()->first >= 0.0000001f,
                   "Failed to find suitable physical device");

  instance_.physicalDevice = scores.rbegin()->second;
  vkGetPhysicalDeviceProperties(instance_.physicalDevice,
                                &instance_.properties);
}

float VkApp::CalcDeviceScore(VkPhysicalDevice device) const {
  VkPhysicalDeviceProperties prop{};
  vkGetPhysicalDeviceProperties(device, &prop);
  VkPhysicalDeviceFeatures feat{};
  vkGetPhysicalDeviceFeatures(device, &feat);

  std::map<VkPhysicalDeviceType, float> scores = {
      {VK_PHYSICAL_DEVICE_TYPE_CPU, 0.0f},
      {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 1000.0f}};
  float score = 0.0f;
  if (scores.count(prop.deviceType) > 0) {
    score = scores[prop.deviceType];
  } else {
    score = 1.0f;
  }

  const auto families = QueueFamilies::Find(device, instance_.surface);
  if (!families.IsComplete()) {
#if !defined(NDEBUG)
    spdlog::warn("Missing suitable queue family");
#endif
    score = 0;
  }

  uint32_t size = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &size, nullptr);
  std::vector<VkExtensionProperties> extensions(size);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &size,
                                       extensions.data());

  std::set<std::string> missing(deviceExtensions_.begin(),
                                deviceExtensions_.end());
  for (const auto &extension : extensions) {
    missing.erase(extension.extensionName);
  }
  if (!missing.empty()) {
    score = 0;
#if !defined(NDEBUG)
    spdlog::warn("Missing device extensions");
#endif
  } else {
    size = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, instance_.surface, &size,
                                         nullptr);
    std::vector<VkSurfaceFormatKHR> formats(size);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, instance_.surface, &size,
                                         formats.data());
    if (formats.empty()) {
      score = 0;
#if !defined(NDEBUG)
      spdlog::warn("Missing surface formats");
#endif
    }

    size = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, instance_.surface, &size,
                                              nullptr);
    std::vector<VkPresentModeKHR> modes(size);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, instance_.surface, &size,
                                              modes.data());
    if (modes.empty()) {
      score = 0;
#if !defined(NDEBUG)
      spdlog::warn("Missing present modes");
#endif
    }
  }

  if (!feat.samplerAnisotropy) {
    score = 0;
  }

  uint32_t major = VK_VERSION_MAJOR(VK_API_VERSION_1_0);
  uint32_t minor = VK_VERSION_MINOR(VK_API_VERSION_1_0);
  uint32_t version = VK_MAKE_VERSION(major, minor, VK_HEADER_VERSION);

  if (prop.apiVersion >= version) {
    score *= 1.1f;
  }
#if !defined(NDEBUG)
  spdlog::info(" - score {}", score);
#endif

  return score;
}

void VkApp::CreateLogicalDevice() {
  QueueFamilies families = QueueFamilies::Find(instance_);

  std::vector<VkDeviceQueueCreateInfo> createQueues;
  float prio = 1.0f;
  for (int family : families.UniqueFamilies()) {
    VkDeviceQueueCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create.queueFamilyIndex = static_cast<uint32_t>(family);
    create.queueCount = 1;
    create.pQueuePriorities = &prio;
    createQueues.emplace_back(create);
  }

  VkPhysicalDeviceFeatures features{};
  features.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create.queueCreateInfoCount = static_cast<uint32_t>(createQueues.size());
  create.pQueueCreateInfos = createQueues.data();
  create.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions_.size());
  create.ppEnabledExtensionNames = deviceExtensions_.data();
  create.pEnabledFeatures = &features;
  if (isEnableValidationLayers_) {
    create.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
    create.ppEnabledLayerNames = validationLayers_.data();
  } else {
    create.enabledLayerCount = 0;
  }

  if (vkCreateDevice(instance_.physicalDevice, &create, nullptr,
                     &instance_.device) != VK_SUCCESS) {
    BOOST_ASSERT_MSG(false, "Failed to create logical device");
  }

  vkGetDeviceQueue(instance_.device, static_cast<uint32_t>(families.graphics),
                   0, &instance_.queues.graphics);
  vkGetDeviceQueue(instance_.device,
                   static_cast<uint32_t>(families.presentation), 0,
                   &instance_.queues.presentation);
}

void VkApp::CleanupSwapchain() { swapchain_.Cleanup(instance_); }
