/**
 * #brief 物理デバイスと論理デバイスのプレゼンテーションをカプセル化を行います。
 */

#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <string>
#include <vector>

struct Device {
public:
  void Init(VkPhysicalDevice physicalDevice);
  void Destroy() const;

  [[nodiscard]] VkResult
  CreateLogicalDevice(VkPhysicalDeviceFeatures requestedFeatures,
                      std::vector<const char *> requestedExtensions,
                      VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT,
                      bool useSwapchain = true, void *pNextChain = nullptr);
  [[nodiscard]] VkCommandPool
  CreateCommandPool(uint32_t queueFamilyIndex,
                    VkCommandPoolCreateFlags createFlags =
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) const;

  [[nodiscard]] VkResult CreateBuffer(VkBufferUsageFlags bufferUsageFlags,
                                      VkMemoryPropertyFlags memoryPropertyFlags,
                                      VkDeviceSize size, VkBuffer *buffer,
                                      VkDeviceMemory *memory, const void *data = nullptr) const;

  [[nodiscard]] VkCommandBuffer CreateCommandBuffer(
      VkCommandPool pool,
      VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      bool begin = true) const;
  [[nodiscard]] VkCommandBuffer CreateCommandBuffer(
      VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      bool begin = true) const;
  void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue,
                          VkCommandPool pool, bool free = true) const;
  void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue,
                          bool free = true) const;

  [[nodiscard]] uint32_t FindMemoryType(uint32_t memoryType,
                                        VkMemoryPropertyFlags flags) const;
  [[nodiscard]] uint32_t
  FindQueueFamilyIndex(VkQueueFlagBits queueFlagBits) const;
  [[nodiscard]] VkFormat
  FindSupportedDepthFormat(bool checkSamplingSupport = false) const;
  [[nodiscard]] bool IsSupportedExtension(const std::string &extension) const;

  operator VkDevice() const noexcept { return logicalDevice; }

  /** @brief 物理デバイス */
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  /** @brief 論理デバイス */
  VkDevice logicalDevice = VK_NULL_HANDLE;
  /** @brief アプリケーションがチェックできる制限を含む物理デバイスのプロパティ
   */
  VkPhysicalDeviceProperties properties{};
  /** @brief
   * アプリケーションがfeaturesがサポートされているかどうかを確認するために使用できる物理デバイスの機能(features)
   */
  VkPhysicalDeviceFeatures features{};
  /** @brief 物理デバイスでの使用が有効になっている機能 */
  VkPhysicalDeviceFeatures enabledFeatures{};
  /** @brief 物理デバイスのメモリタイプとヒープ */
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  /** @brief 物理デバイスのキューファミリプロパティ */
  std::vector<VkQueueFamilyProperties> queueFamilyProperties{};
  /** @brief デバイスでサポートされている拡張機能のリスト */
  std::vector<std::string> supportExtensions{};
  /** @brief
   * グラフィックキューファミリーインデックスのデフォルトのコマンドプール */
  VkCommandPool commandPool = VK_NULL_HANDLE;
  /** @brief キューファミリーインデックス */
  struct {
    uint32_t graphics;
    uint32_t compute;
    uint32_t transfer;
  } queueFamilyIndices{};
};
