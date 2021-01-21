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

#include "VK/Debug.h"
#include "VK/Device.h"
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

  void DestroyDepthStencil();

  virtual void ResizeWindow();
  virtual void ViewChanged();
  [[nodiscard]] virtual VkPhysicalDeviceFeatures GetEnabledFeatures() const;
  [[nodiscard]] virtual std::vector<const char *>
  GetEnabledDeviceExtensions() const;
  [[nodiscard]] virtual VkPhysicalDevice SelectPhysicalDevice() const;

  VkInstance instance = VK_NULL_HANDLE;
  Device device{};
  VkQueue queue = VK_NULL_HANDLE;

  /** @brief Swap chain to present images (framebuffers) to the windowing system
   */
  Swapchain swapchain{};
  /** @brief コマンドバッファプール */
  VkCommandPool cmdPool = VK_NULL_HANDLE;
  /** @brief 記述子セットプール */
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  /** @brief フレームバッファに書き込むグローバルレンダーパス */
  VkRenderPass renderPass = VK_NULL_HANDLE;
  /** @brief レンダリングに使用されるコマンドバッファ */
  std::vector<VkCommandBuffer> drawCmdBuffers{};
  /** @brief 使用可能なフレームバッファのリスト */
  std::vector<VkFramebuffer> framebuffers{};
  /** @brief 現在使用しているフレームバッファのインデックス */
  uint32_t currentBuffer = 0;
  /** @brief  同期セマフォ */
  struct {
    /** @brief swap chain image presentation */
    VkSemaphore presentComplete = VK_NULL_HANDLE;
    /** @brief コマンドバッファの送信と実行に用います。 */
    VkSemaphore renderComplete = VK_NULL_HANDLE;
  } semaphores{};
  std::vector<VkFence> waitFences{};
  /** @brief キューに提示されるコマンドバッファとセマフォが含まれます。*/
  VkSubmitInfo submitInfo{};
  /** @brief
   * グラフィックキューの送信を待機するために使用されるパイプラインステージ */
  VkPipelineStageFlags submitPipelineStages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  /** @brief Pipeline cache object */
  VkPipelineCache pipelineCache = VK_NULL_HANDLE;
  /** @brief Depth stencil object */
  struct {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
  } depthStencil;

  GLFWwindow *window = nullptr;
  nlohmann::json config{};
  bool isFramebufferResized = false;

  std::vector<const char *> validationLayers_ = {
      "VK_LAYER_KHRONOS_validation",
  };

#if !defined(NDEBUG)
  DebugMessenger debugMessenger{};
#endif

#if defined(NDEBUG)
  const bool isEnableValidationLayers_ = false;
#else
  const bool isEnableValidationLayers_ = true;
#endif
};
