/**
 * @brief Vulkan Application
 */

#pragma once

#include <array>
#include <boost/noncopyable.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VK/Buffer/Buffer.h"
#include "VK/Debug.h"
#include "VK/Instance.h"
#include "VK/Swapchain.h"
#include "VK/Sync/SyncObjects.h"

class VkBase : private boost::noncopyable {
public:
  VkBase() = default;
  virtual ~VkBase() = default;

  virtual void OnInit(const nlohmann::json &config, GLFWwindow *window);
  virtual void OnDestroy();
  virtual void OnUpdate(float t);
  virtual void OnRender();

  void WaitIdle() const;

  static void OnResized(GLFWwindow *window, int width, int height);

protected:
  void CreateInstance(const char *appName);
  void CreateSurface();
  void SelectPhysicalDevice();
  void CreateLogicalDevice();
  float CalcDeviceScore(VkPhysicalDevice physicalDevice) const;

  void CreateSwapchain(int width, int height);
  void CreateCommandPool();

  virtual void CreateRenderPass() = 0;
  virtual void CreateDescriptorSetLayouts() = 0;
  virtual void DestroyDescriptorSetLayouts() = 0;
  virtual void CreatePipelines() = 0;
  virtual void DestroyPipelines() = 0;
  virtual void CreateDepthStencil() {}
  virtual void DestroyDepthStencil() {}
  virtual void CreateFramebuffers() = 0;
  virtual void SetupAssets() {}
  virtual void CleanupAssets() {}
  virtual void CreateVertexBuffer() = 0;
  virtual void CreateIndexBuffer() = 0;
  virtual void CreateUniformBuffers() = 0;
  virtual void DestroyUniformBuffers() = 0;
  virtual void CreateDescriptorPool() = 0;
  virtual void CreateDescriptorSets() = 0;
  virtual void CreateDrawCommandBuffers() = 0;

  void CreateSyncObjects();

  virtual void CleanupSwapchain();
  virtual void RecreateSwapchain();
  virtual void UpdateUniformBuffers(uint32_t currentImage) {
    static_cast<void>(currentImage);
  }

  static constexpr size_t kMaxFramesInFlight = 2;

  Instance instance_{};
  Swapchain swapchain_{};
  VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  struct CommandBuffers {
    std::vector<VkCommandBuffer> draw;
  } commandBuffers_{};
  std::vector<VkFramebuffer> framebuffers_{};
  Buffer vertex_{};
  Buffer index_{};
  SyncObjects syncs_{};

  GLFWwindow *window_ = nullptr;
  nlohmann::json config_{};
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
