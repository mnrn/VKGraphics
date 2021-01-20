/**
 * @brief Vulkan Application
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */

//*-----------------------------------------------------------------------------
// Including files
//*-----------------------------------------------------------------------------

#include "VkBase.h"

#include <boost/assert.hpp>
#include <map>
#include <spdlog/spdlog.h>

#include "VK/Common.h"
#include "VK/Initializer.h"
#include "VK/Utils.h"

//*-----------------------------------------------------------------------------
// Init & Deinit
//*-----------------------------------------------------------------------------

void VkBase::OnInit(const nlohmann::json &conf, GLFWwindow *hwnd) {
  config = conf;
  window = hwnd;

  const auto appName = config["AppName"].get<std::string>();

  CreateInstance(appName.c_str());
#if !defined(NDEBUG)
  debug_.Setup(instance.Get());
#endif
  CreateSurface();
  SelectPhysicalDevice();
  CreateLogicalDevice();
  CreateSemaphores();

  OnPostInit();
}

void VkBase::OnPostInit() {
  const auto width = config["Width"].get<int>();
  const auto height = config["Height"].get<int>();

  CreateSwapchain(width, height);

  CreateCommandPool();
  CreateCommandBuffers();
  CreateFence();
  SetupDepthStencil();
  SetupRenderPass();
  CreatePipelineCache();
  SetupFramebuffers();
}

void VkBase::OnPreDestroy() {}

void VkBase::OnDestroy() {
  OnPreDestroy();

  swapchain.Destroy(instance);
  if (descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(instance.device, descriptorPool, nullptr);
  }
  DestroyCommandBuffers();
  vkDestroyRenderPass(instance.device, renderPass, nullptr);
  for (auto &framebuffer : framebuffers) {
    vkDestroyFramebuffer(instance.device, framebuffer, nullptr);
  }

  depthStencil.Destroy(instance);

  vkDestroyPipelineCache(instance.device, pipelineCache, nullptr);
  vkDestroyCommandPool(instance.device, instance.pool, nullptr);
  DestroySyncObjects();

#if !defined(NDEBUG)
  debug_.Cleanup(instance.Get());
#endif
  instance.Destroy();
}

//*-----------------------------------------------------------------------------
// Update
//*-----------------------------------------------------------------------------

void VkBase::OnUpdate(float) {}

//*-----------------------------------------------------------------------------
// Render
//*-----------------------------------------------------------------------------

void VkBase::OnRender() { VkBase::RenderFrame(); }

void VkBase::RenderFrame() {
  VkBase::PrepareFrame();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
  VK_CHECK_RESULT(
      vkQueueSubmit(instance.queues.graphics, 1, &submitInfo, VK_NULL_HANDLE));
  VkBase::SubmitFrame();
}

void VkBase::PrepareFrame() {
  // スワップチェーンの次の画像を取得します。(バック/フロントバッファ)
  VkResult result = vkAcquireNextImageKHR(
      instance.device, swapchain.Get(), std::numeric_limits<uint64_t>::max(),
      semaphores.presentComplete, VK_NULL_HANDLE, &currentBuffer);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    ResizeWindow();
    return;
  } else {
    VK_CHECK_RESULT(result);
  }
}

void VkBase::SubmitFrame() {
  VkPresentInfoKHR present{};
  present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present.swapchainCount = 1;
  present.pSwapchains = &swapchain.Get();
  present.pImageIndices = &currentBuffer;
  if (semaphores.renderComplete) {
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &semaphores.renderComplete;
  }

  // 現在のバッファをスワップチェーンに提示します。
  // コマンドバッファ送信によって通知されたセマフォを送信情報からスワップチェーンプレゼンテーションの待機セマフォとして渡します。
  // これにより、すべてのコマンドが送信されるまで、画像がウィンドウシステムに表示されないようになります。
  VkResult result = vkQueuePresentKHR(instance.queues.presentation, &present);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      isFramebufferResized_) {
    isFramebufferResized_ = false;
    ResizeWindow();
    return;
  } else {
    VK_CHECK_RESULT(result);
  }
  VK_CHECK_RESULT(vkQueueWaitIdle(instance.queues.presentation));
}

void VkBase::WaitIdle() const { vkDeviceWaitIdle(instance.device); }

//*-----------------------------------------------------------------------------
// Resize window
//*-----------------------------------------------------------------------------

void VkBase::OnResized(GLFWwindow *window, int, int) {
  auto app = reinterpret_cast<VkBase *>(glfwGetWindowUserPointer(window));
  app->isFramebufferResized_ = true;
}

void VkBase::ResizeWindow() {
  // Windowが最小化されている場合framebufferのresizeが行われるまで待ちます。
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  // リソースを破棄する前にDeviceがすべての作業を終わらせている必要があります。
  vkDeviceWaitIdle(instance.device);

  // Swap chain の再生成を行います。
  swapchain.Create(instance, width, height);

  // Frame buffers の再生成を行います。
  depthStencil.Destroy(instance);
  depthStencil.Create(instance, swapchain);
  for (auto &framebuffer : framebuffers) {
    vkDestroyFramebuffer(instance.device, framebuffer, nullptr);
  }
  SetupFramebuffers();

  // Frame buffersの再生成後にCommand buffersも再生成する必要があります。
  DestroyCommandBuffers();
  CreateCommandBuffers();
  BuildCommandBuffers();

  vkDeviceWaitIdle(instance.device);

  ViewChanged();
}

//*-----------------------------------------------------------------------------
// Vulkan Instance
//*-----------------------------------------------------------------------------

void VkBase::CreateInstance(const char *appName) {
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

  if (vkCreateInstance(&create, nullptr, instance.Set())) {
    BOOST_ASSERT_MSG(false, "Failed to create instance!");
  }
}

void VkBase::CreateSurface() {
  if (glfwCreateWindowSurface(instance.Get(), window, nullptr,
                              &instance.surface)) {
    BOOST_ASSERT_MSG(false, "Failed to create window surface!");
  }
}

void VkBase::SelectPhysicalDevice() {
  uint32_t size = 0;
  vkEnumeratePhysicalDevices(instance.Get(), &size, nullptr);
  BOOST_ASSERT_MSG(size != 0, "Failed to find any physical device!");

  std::vector<VkPhysicalDevice> devices(size);
  vkEnumeratePhysicalDevices(instance.Get(), &size, devices.data());
#if !defined(NDEBUG)
  spdlog::info("Found {} physical devices", size);
#endif
  std::multimap<float, VkPhysicalDevice> scores;
  for (const auto &device : devices) {
    scores.emplace(
        Utils::CalcDeviceScore(device, instance.surface, deviceExtensions_),
        device);
  }
  BOOST_ASSERT_MSG(scores.rbegin()->first >= 0.0000001f,
                   "Failed to find suitable physical device");

  instance.physicalDevice = scores.rbegin()->second;
  vkGetPhysicalDeviceProperties(instance.physicalDevice, &instance.properties);
}

void VkBase::CreateLogicalDevice() {
  QueueFamilies families = QueueFamilies::Find(instance);

  std::vector<VkDeviceQueueCreateInfo> createQueues;
  float priority = 1.0f;
  for (int family : families.UniqueFamilies()) {
    VkDeviceQueueCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create.queueFamilyIndex = static_cast<uint32_t>(family);
    create.queueCount = 1;
    create.pQueuePriorities = &priority;
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

  if (vkCreateDevice(instance.physicalDevice, &create, nullptr,
                     &instance.device)) {
    BOOST_ASSERT_MSG(false, "Failed to create logical device");
  }

  vkGetDeviceQueue(instance.device, static_cast<uint32_t>(families.graphics), 0,
                   &instance.queues.graphics);
  vkGetDeviceQueue(instance.device,
                   static_cast<uint32_t>(families.presentation), 0,
                   &instance.queues.presentation);
}

//*-----------------------------------------------------------------------------
// Vulkan Fixed functions
//*-----------------------------------------------------------------------------

void VkBase::CreateSwapchain(int w, int h) { swapchain.Create(instance, w, h); }

void VkBase::CreatePipelineCache() {
  VkPipelineCacheCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(
      vkCreatePipelineCache(instance.device, &create, nullptr, &pipelineCache));
}

void VkBase::CreateCommandPool() {
  QueueFamilies families = QueueFamilies::Find(instance);

  VkCommandPoolCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create.queueFamilyIndex = static_cast<uint32_t>(families.graphics);
  create.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(
      vkCreateCommandPool(instance.device, &create, nullptr, &instance.pool));
}

void VkBase::CreateCommandBuffers() {
  drawCmdBuffers.resize(swapchain.images.size());

  VkCommandBufferAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc.commandPool = instance.pool;
  alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc.commandBufferCount = static_cast<uint32_t>(drawCmdBuffers.size());
  VK_CHECK_RESULT(
      vkAllocateCommandBuffers(instance.device, &alloc, drawCmdBuffers.data()));
}

void VkBase::DestroyCommandBuffers() {
  vkFreeCommandBuffers(instance.device, instance.pool,
                       static_cast<uint32_t>(drawCmdBuffers.size()),
                       drawCmdBuffers.data());
}

void VkBase::CreateSemaphores() {
  VkSemaphoreCreateInfo semaphoreCreateInfo =
      Initializer::SemaphoreCreateInfo();
  VK_CHECK_RESULT(vkCreateSemaphore(instance.device, &semaphoreCreateInfo,
                                    nullptr, &semaphores.presentComplete));
  VK_CHECK_RESULT(vkCreateSemaphore(instance.device, &semaphoreCreateInfo,
                                    nullptr, &semaphores.renderComplete));

  submitInfo = Initializer::SubmitInfo();
  submitInfo.pWaitDstStageMask = &submitPipelineStages;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &semaphores.presentComplete;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &semaphores.renderComplete;
}

void VkBase::CreateFence() {
  VkFenceCreateInfo create =
      Initializer::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
  waitFences.resize(drawCmdBuffers.size());
  for (auto &fence : waitFences) {
    VK_CHECK_RESULT(vkCreateFence(instance.device, &create, nullptr, &fence));
  }
}

void VkBase::DestroySyncObjects() {
  for (auto &fence : waitFences) {
    vkDestroyFence(instance.device, fence, nullptr);
  }
  vkDestroySemaphore(instance.device, semaphores.renderComplete, nullptr);
  vkDestroySemaphore(instance.device, semaphores.presentComplete, nullptr);
}

//*-----------------------------------------------------------------------------
// Vulkan virtual functions
//*-----------------------------------------------------------------------------

/**
 * @brief フレームバッファで使用される深度(ステンシル)バッファを生成します。
 */
void VkBase::SetupDepthStencil() { depthStencil.Create(instance, swapchain); }

/**
 * @brief スワップチェーンのイメージごとにフレームバッファを生成します。
 */
void VkBase::SetupFramebuffers() {
  std::array<VkImageView, 2> attachments{VK_NULL_HANDLE, depthStencil.view};

  // Depth/Stencil attachmentをすべてのframebufferに適用します。
  VkFramebufferCreateInfo create = Initializer::FramebufferCreateInfo();

  // すべてのフレームバッファは同じのレンダーパス設定を使用します。
  create.renderPass = renderPass;
  create.attachmentCount = static_cast<uint32_t>(attachments.size());
  create.pAttachments = attachments.data();
  create.width = swapchain.extent.width;
  create.height = swapchain.extent.height;
  create.layers = 1;

  // スワップチェーン内のすべてのイメージのフレームバッファを生成します。
  framebuffers.resize(swapchain.views.size());
  for (size_t i = 0; i < framebuffers.size(); i++) {
    attachments[0] = swapchain.views[i];
    VK_CHECK_RESULT(vkCreateFramebuffer(instance.device, &create, nullptr,
                                        &framebuffers[i]));
  }
}

/**
 * @brief
 * レンダリングパスの設定(ここでは1つのサブパスを持つ単一のレンダーパスを使用します。)
 * @note
 * レンダリングパスはVulkanの新しい概念です。それらはレンダリング中に使用されるアタッチメントを記述し、アタッチメントの依存関係を持つ複数のサブパスを含む場合があります。<br>
 * これにより、ドライバーはレンダリングがどのように見えるかを事前に知ることができ、特にタイルベースのレンダラー(複数のサブパスを使用)は最適化する良い機会です。<br>
 * サブパスの依存関係を使用すると使用するアタッチメントの暗黙的なレイアウト遷移も追加されるため、明示的な画像のメモリバリアを追加する必要はありません。
 */
void VkBase::SetupRenderPass() {

  // カラーアタッチメント
  VkAttachmentDescription color{};
  color.format = swapchain.format;
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // デプスアタッチメント
  VkAttachmentDescription depth{};
  if (const auto format =
          DepthStencil::FindDepthFormat(instance.physicalDevice)) {
    depth.format = format.value();
  } else {
    BOOST_ASSERT_MSG(format, "Failed to find suitable depth format!");
  }
  depth.samples = VK_SAMPLE_COUNT_1_BIT;
  depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // アタッチメントの参照を設定します。
  VkAttachmentReference colorRef{};
  colorRef.attachment = 0;
  colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthRef{};
  depthRef.attachment = 1;
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // Setup a single subpass reference
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = nullptr;
  subpass.pResolveAttachments = nullptr;

  // サブパスの依存関係を設定します。
  // これらはアタッチメントの記述子で指定された暗黙のアタッチメントレイアウト遷移を追加します。
  // 実際の使用するレイアウトは、アタッチメントリファレンスで指定されたレイアウトを通じて保持されます。
  // 各サブパスの依存関係はsrcStageMask、dstStageMask、srcAccessMask、dstAccessMask(およびdependencyFlagsが設定されている)
  // によって記述されている入力サブパスと出力サブパスの間にメモリと実行の依存関係を導入します。
  // NOTE:VK_SUBPASS_EXTERNALは、実際のレンダーパスの外部で実行されるすべてのコマンドを参照する特別な定数です。
  std::array<VkSubpassDependency, 2> dependencies{};

  // レンダーパスの開始時の依存関係(First dependency)
  // 最終レイアウトから開始レイアウトへの移行を行います。
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // レンダーパスの終了時の依存関係(Second dependency)
  // 開始レイアウトから終了レイアウトへの移行を行います。
  // これは、暗黙的なサブパスの依存関係と同じですが、ここで明示的に行います。
  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  std::vector<VkAttachmentDescription> attachments{color, depth};

  // 実際のレンダーパスを作成します。
  VkRenderPassCreateInfo create = Initializer::RenderPassCreateInfo();
  create.attachmentCount = static_cast<uint32_t>(attachments.size());
  create.pAttachments = attachments.data();
  create.subpassCount = 1;
  create.pSubpasses = &subpass;
  create.dependencyCount = static_cast<uint32_t>(dependencies.size());
  create.pDependencies = dependencies.data();

  VK_CHECK_RESULT(
      vkCreateRenderPass(instance.device, &create, nullptr, &renderPass));
}

void VkBase::BuildCommandBuffers() {}

void VkBase::ViewChanged() {}
