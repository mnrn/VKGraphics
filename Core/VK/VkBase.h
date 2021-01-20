/**
 * @brief Vulkan Application
 */

#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <boost/noncopyable.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

#include <GLFW/glfw3.h>

#include "VK/Buffer/Buffer.h"
#include "VK/Buffer/DepthStencil.h"
#include "VK/Debug.h"
#include "VK/Instance.h"
#include "VK/Swapchain.h"

class VkBase : private boost::noncopyable {
public:
  VkBase() = default;
  virtual ~VkBase() = default;

  virtual void OnInit(const nlohmann::json &config, GLFWwindow *window);
  virtual void OnDestroy();
  virtual void OnUpdate(float);
  virtual void OnRender();

  void WaitIdle() const;

  static void OnResized(GLFWwindow *window, int width, int height);

protected:
  void CreateInstance(const char *appName);
  void CreateSurface();
  void SelectPhysicalDevice();
  void CreateLogicalDevice();

  virtual void OnPostInit();
  virtual void OnPreDestroy();

  void CreateSwapchain(int width, int height);
  void CreatePipelineCache();
  void CreateCommandPool();
  void CreateCommandBuffers();
  void DestroyCommandBuffers();

  virtual void SetupRenderPass();
  virtual void SetupDepthStencil();
  virtual void SetupFramebuffers();
  virtual void BuildCommandBuffers();

  void CreateSemaphores();
  void CreateFence();
  void DestroySyncObjects();

  void PrepareFrame();
  void RenderFrame();
  void SubmitFrame();

  virtual void ResizeWindow();
  virtual void ViewChanged();

  // Vulkan Instance
  Instance instance{};
  // Swap chain to present images (framebuffers) to the windowing system
  Swapchain swapchain{};
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  // フレームバッファに書き込むグローバルレンダーパス
  VkRenderPass renderPass = VK_NULL_HANDLE;
  // レンダリングに使用されるコマンドバッファ
  std::vector<VkCommandBuffer> drawCmdBuffers{};
  // 使用可能なフレームバッファのリスト(スワップチェーンイメージの数と同じになります。)
  std::vector<VkFramebuffer> framebuffers{};
  // 現在使用しているフレームバッファのインデックス
  uint32_t currentBuffer = 0;
  // 同期セマフォ
  struct {
    // swap chain imagesの表示を交換します。
    VkSemaphore presentComplete = VK_NULL_HANDLE;
    // コマンドバッファの送信と実行に用います。
    VkSemaphore renderComplete = VK_NULL_HANDLE;
  } semaphores{};
  std::vector<VkFence> waitFences{};
  // キューに提示されるコマンドバッファとセマフォが含まれます。
  VkSubmitInfo submitInfo;
  // グラフィックキューの送信を待機するために使用されるパイプラインステージ
  VkPipelineStageFlags submitPipelineStages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  // Pipeline cache object
  VkPipelineCache pipelineCache = VK_NULL_HANDLE;
  // Depth stencil object
  DepthStencil depthStencil;

  GLFWwindow *window = nullptr;
  nlohmann::json config{};
  bool isFramebufferResized_ = false;

#if !defined(NDEBUG)
  DebugMessenger debug_{};
#endif
  std::vector<const char *> validationLayers_ = {
#ifdef __APPLE__
      "VK_LAYER_KHRONOS_validation",
#else
#endif
  };
  std::vector<const char *> deviceExtensions_ = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };
#if defined(NDEBUG)
  const bool isEnableValidationLayers_ = false;
#else
  const bool isEnableValidationLayers_ = true;
#endif
};
