/**
 * @brief Vulkan Application
 */

//*-----------------------------------------------------------------------------
// Including files
//*-----------------------------------------------------------------------------

#include "VkApp.h"
#include <vulkan/vulkan.h>

#include <boost/assert.hpp>
#include <map>
#include <spdlog/spdlog.h>

#include "VK/Buffer/DepthStencil.h"
#include "VK/Pipeline/Shader.h"

//*-----------------------------------------------------------------------------
// Constant expressions
//*-----------------------------------------------------------------------------

//*-----------------------------------------------------------------------------
// Init & Deinit
//*-----------------------------------------------------------------------------

void VkApp::OnInit(const nlohmann::json &config, GLFWwindow *window) {
  config_ = config;
  window_ = window;

  const auto appName = config_["AppName"].get<std::string>();
  const auto width = config_["Width"].get<int>();
  const auto height = config_["Height"].get<int>();

  CreateInstance(appName.c_str());
#if !defined(NDEBUG)
  debug_.Setup(instance_.Get());
#endif
  CreateSurface();
  SelectPhysicalDevice();
  CreateLogicalDevice();
  CreateSwapchain(width, height);
  CreateRenderPass();
  CreatePipelines();
  CreateCommandPool();
  CreateFramebuffers();
  CreateDrawCommandBuffers();
  CreateSyncObjects();
}

void VkApp::OnDestroy() {
  CleanupSwapchain();
  pipelines_.Destroy(instance_);
  syncs_.Destroy(instance_, kMaxFramesInFlight);
  vkDestroyCommandPool(instance_.device, instance_.pool, nullptr);
#if !defined(NDEBUG)
  debug_.Cleanup(instance_.Get());
#endif
  instance_.Destroy();
}

void VkApp::OnUpdate(float) {}

void VkApp::OnRender() {
  const size_t id = syncs_.currentFrame;
  vkWaitForFences(instance_.device, 1, &syncs_.fences.inFlight[id], VK_TRUE,
                  std::numeric_limits<uint64_t>::max());
  uint32_t image;
  VkResult result = vkAcquireNextImageKHR(
      instance_.device, swapchain_.Get(), std::numeric_limits<uint64_t>::max(),
      syncs_.semaphores.imageAvailable[id], VK_NULL_HANDLE, &image);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    RecreateSwapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    BOOST_ASSERT_MSG(false, "Failed to acquire swap chain image!");
  }

  if (syncs_.fences.imagesInFlight[image] != VK_NULL_HANDLE) {
    vkWaitForFences(instance_.device, 1, &syncs_.fences.imagesInFlight[image],
                    VK_TRUE, std::numeric_limits<uint64_t>::max());
  }
  syncs_.fences.imagesInFlight[image] = syncs_.fences.inFlight[id];

  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSems[] = {syncs_.semaphores.imageAvailable[id]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = waitSems;
  submit.pWaitDstStageMask = waitStages;

  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commandBuffers_.draw[image];

  VkSemaphore signalSems[] = {syncs_.semaphores.renderFinished[id]};
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = signalSems;

  vkResetFences(instance_.device, 1, &syncs_.fences.inFlight[id]);

  if (vkQueueSubmit(instance_.queues.graphics, 1, &submit,
                    syncs_.fences.inFlight[id])) {
    BOOST_ASSERT_MSG(false, "Failed to submit draw command buffer!");
  }

  VkPresentInfoKHR present{};
  present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = signalSems;
  present.swapchainCount = 1;
  present.pSwapchains = &swapchain_.Get();
  present.pImageIndices = &image;

  result = vkQueuePresentKHR(instance_.queues.presentation, &present);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      isFramebufferResized_) {
    isFramebufferResized_ = false;
    RecreateSwapchain();
  } else if (result != VK_SUCCESS) {
    BOOST_ASSERT_MSG(false, "Failed to present swap chain image!");
  }

  syncs_.currentFrame = (id + 1) % kMaxFramesInFlight;
}


void VkApp::WaitIdle() const { vkDeviceWaitIdle(instance_.device); }

void VkApp::OnResized(GLFWwindow *window, int, int) {
  auto app = reinterpret_cast<VkApp *>(glfwGetWindowUserPointer(window));
  app->isFramebufferResized_ = true;
}

//*-----------------------------------------------------------------------------
// Create & Destroy
//*-----------------------------------------------------------------------------

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
    spdlog::info("Enabling validation");
    create.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
    create.ppEnabledLayerNames = validationLayers_.data();
  } else {
    create.enabledLayerCount = 0;
  }

  if (vkCreateInstance(&create, nullptr, instance_.Set())) {
    BOOST_ASSERT_MSG(false, "Failed to create instance!");
  }
}

void VkApp::CreateSurface() {
  if (glfwCreateWindowSurface(instance_.Get(), window_, nullptr,
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
                     &instance_.device)) {
    BOOST_ASSERT_MSG(false, "Failed to create logical device");
  }

  vkGetDeviceQueue(instance_.device, static_cast<uint32_t>(families.graphics),
                   0, &instance_.queues.graphics);
  vkGetDeviceQueue(instance_.device,
                   static_cast<uint32_t>(families.presentation), 0,
                   &instance_.queues.presentation);
}

void VkApp::CreateSwapchain(int w, int h) {
  swapchain_.Create(instance_, w, h);
}

void VkApp::CreateCommandPool() {
  QueueFamilies families = QueueFamilies::Find(instance_);

  VkCommandPoolCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create.queueFamilyIndex = static_cast<uint32_t>(families.graphics);
  create.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (vkCreateCommandPool(instance_.device, &create, nullptr,
                          &instance_.pool)) {
    BOOST_ASSERT_MSG(false, "Failed to create command pool!");
  }
}

void VkApp::CreateDrawCommandBuffers() {
  commandBuffers_.draw.resize(framebuffers_.size());

  VkCommandBufferAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc.commandPool = instance_.pool;
  alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc.commandBufferCount = static_cast<uint32_t>(commandBuffers_.draw.size());
  if (vkAllocateCommandBuffers(instance_.device, &alloc,
                               commandBuffers_.draw.data())) {
    BOOST_ASSERT_MSG(false, "Failed to alloc draw command buffers!");
  }

  RecordDrawCommands();
}

void VkApp::CreateSyncObjects() {
  syncs_.Create(instance_, swapchain_, kMaxFramesInFlight);
}

//*-----------------------------------------------------------------------------
// Swapchain
//*-----------------------------------------------------------------------------

void VkApp::RecreateSwapchain() {
  // Windowが最小化されている場合framebufferのresizeが行われるまで待ちます。
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window_, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window_, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(instance_.device);

  CleanupSwapchain();

  CreateSwapchain(width, height);
  CreateRenderPass();
  CreatePipelines();
  CreateFramebuffers();
  CreateDrawCommandBuffers();
}

void VkApp::CleanupSwapchain() {
  for (auto &framebuffer : framebuffers_) {
    vkDestroyFramebuffer(instance_.device, framebuffer, nullptr);
  }
  vkFreeCommandBuffers(instance_.device, instance_.pool,
                       static_cast<uint32_t>(commandBuffers_.draw.size()),
                       commandBuffers_.draw.data());
  pipelines_.Cleanup(instance_);
  vkDestroyRenderPass(instance_.device, renderPass_, nullptr);
  swapchain_.Destroy(instance_);
}

//*-----------------------------------------------------------------------------
// Subroutine
//*-----------------------------------------------------------------------------

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
