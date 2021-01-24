#include "VK/Device.h"

#include <boost/assert.hpp>

#include "VK/Common.h"
#include "VK/Initializer.h"

void Device::Init(VkPhysicalDevice selectedDevice) {
  physicalDevice = selectedDevice;

  // 後で使用できるように物理デバイスのプロパティ機能などを記憶しておきます。

  // デバイスプロパティにはLimitsプロパティやSparseプロパティが含まれます。
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);

  // Features(機能)は使用する前に確認する必要があります。
  vkGetPhysicalDeviceFeatures(physicalDevice, &features);

  // メモリプロパティはあらゆる種類のバッファを生成するために定期的に使用されます。
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

  // キューファミリーのプロパティ。デバイスの生成時に要求されたキューを設定するために使用します。
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  BOOST_ASSERT(queueFamilyCount > 0);
  queueFamilyProperties.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilyProperties.data());

  // サポートされている拡張機能のリストを取得します。
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                       nullptr);
  if (extensionCount > 0) {
    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                             &extensionCount,
                                             extensions.data()) == VK_SUCCESS) {
      for (auto &extension : extensions) {
        supportExtensions.emplace_back(extension.extensionName);
      }
    }
  }
}

void Device::Destroy() const {
  if (commandPool) {
    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
  }
  if (logicalDevice) {
    vkDestroyDevice(logicalDevice, nullptr);
  }
}

/**
 * @brief
 * この関数は、要求するすべてのプロパティフラグをサポートするデバイスメモリタイプを要求するために使用されます。(e.g.
 * device local, host visible)<br>
 * 成功すると、要求されたメモリプロパティに適合するメモリタイプのインデックスが返されます。<br>
 * 実装が異なるメモリプロパティを持つ任意の数のメモリタイプを提供するために必要になります。
 * @ref http://vulkan.gpuinfo.org/
 */
uint32_t Device::FindMemoryType(uint32_t typeBits,
                                VkMemoryPropertyFlags flags) const {
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if (typeBits & (1 << i) &&
        (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
      return i;
    }
  }
  BOOST_ASSERT_MSG(false, "Failed to find suitable memory type!");
  return 0;
}

/**
 * @brief
 * 要求されたキューフラグをサポートするキューファミリーインデックスを返します。
 * @param queueFlagBits キューファミリーインデックスを検索するためのフラグ
 * @return キューフラグに一致するキューファミリーインデックス
 */
uint32_t Device::FindQueueFamilyIndex(VkQueueFlagBits queueFlagBits) const {
  const auto queueFamilyPropertySize =
      static_cast<uint32_t>(queueFamilyProperties.size());

  // グラフィックスではなくコンピューティングをサポートするキューファミリーインデックスを検索します。
  if (queueFlagBits & VK_QUEUE_COMPUTE_BIT) {
    for (uint32_t i = 0; i < queueFamilyPropertySize; i++) {
      if ((queueFamilyProperties[i].queueFlags & queueFlagBits) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0)) {
        return i;
      }
    }
  }

  // 転送をサポートするが、グラフィックスとコンピューティングをサポートしないキューファミリーインデックスを検索します。
  if (queueFlagBits & VK_QUEUE_TRANSFER_BIT) {
    for (uint32_t i = 0; i < queueFamilyPropertySize; i++) {
      if ((queueFamilyProperties[i].queueFlags & queueFlagBits) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
        return i;
      }
    }
  }

  // 他のキュータイプの場合、または個別のコンピュートキューが存在しない場合は、要求されたフラグをサポートするために最初のキューを返します。
  for (uint32_t i = 0; i < queueFamilyPropertySize; i++) {
    if (queueFamilyProperties[i].queueFlags & queueFlagBits) {
      return i;
    }
  }
  BOOST_ASSERT_MSG(false, "Failed to find suitable queue family index");
  return 0;
}

VkFormat Device::FindSupportedDepthFormat(bool checkSamplingSupport) const {
  // すべての深度フォーマットはオプションである可能性があるため、使用する適切な深度フォーマットを探す必要があります。
  std::vector<VkFormat> depthFormats{
      VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM};
  for (auto &format : depthFormats) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format,
                                        &formatProperties);
    // フォーマットは、適切なタイリングのために深度ステンシルアタッチメントをサポートする必要があります。
    if (formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (checkSamplingSupport) {
        if (!(formatProperties.optimalTilingFeatures &
              VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
          continue;
        }
      }
      return format;
    }
  }
  BOOST_ASSERT_MSG(false, "Failed to suitable depth format!");
  return VK_FORMAT_D32_SFLOAT;
}

/**
 * @brief 拡張機能が物理デバイスでサポートされているか確認します。
 * @param extension チェックする拡張機能の名前
 * @return サポートされていれば true を返します。
 */
bool Device::IsSupportedExtension(const std::string &extension) const {
  return std::find(std::begin(supportExtensions), std::end(supportExtensions),
                   extension) != std::end(supportExtensions);
}

/**
 * @brief
 * 割り当てられた物理デバイスに基づいて論理デバイスを生成し、デフォルトのキューファミリーインデックスも取得します。
 * @param requestedFeatures
 * デバイス生成時に特定の機能を有効にするために使用できます。
 * @param requestedExtensions
 * デバイス生成時に特定の拡張機能を有効にするために使用できます。
 * @param requestedQueueTypes
 * デバイスからリクエストされるキュータイプを指定するビットフラグ
 * @param useSwapchain
 * ヘッドレスレンダリングの場合はfalseを指定し、スワップチェーンデバイスの拡張機能を省略します。
 * @param pNextChain 拡張構造へのポインタのオプションのチェーン
 * @return デバイス生成呼び出しのVkResult
 */
VkResult
Device::CreateLogicalDevice(VkPhysicalDeviceFeatures requestedFeatures,
                            std::vector<const char *> requestedExtensions,
                            VkQueueFlags requestedQueueTypes, bool useSwapchain,
                            void *pNextChain) {
  enabledFeatures = requestedFeatures;

  // 論理デバイスの生成時に必要なキューをリクエストする必要があります。
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

  // リクエストされたキューファミリータイプのキューファミリーインデックスを取得します。
  // 実装によってはインデックスが重複する場合があることに注意してください。
  const float defaultQueuePriority = 1.0f;

  // グラフィックスキュー
  if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
    queueFamilyIndices.graphics = FindQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
    VkDeviceQueueCreateInfo queueCreateInfo =
        Initializer::DeviceQueueCreateInfo(queueFamilyIndices.graphics, 1,
                                           &defaultQueuePriority);
    queueCreateInfos.emplace_back(queueCreateInfo);
  } else {
    queueFamilyIndices.graphics = VK_NULL_HANDLE;
  }

  // コンピュート専用キュー
  if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
    queueFamilyIndices.compute = FindQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
    if (queueFamilyIndices.compute != queueFamilyIndices.graphics) {
      // コンピューティングファミリーのインデックスが異なる場合は、コンピューティングキューの追加のキュー生成情報が必要です。
      VkDeviceQueueCreateInfo queueCreateInfo =
          Initializer::DeviceQueueCreateInfo(queueFamilyIndices.compute, 1,
                                             &defaultQueuePriority);
      queueCreateInfos.emplace_back(queueCreateInfo);
    }
  } else {
    // そうでなかった場合は同じグラフィックスキューと同じキューを使用します。
    queueFamilyIndices.compute = queueFamilyIndices.graphics;
  }

  // 転送専用キュー
  if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
    queueFamilyIndices.transfer = FindQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
    if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) &&
        (queueFamilyIndices.transfer != queueFamilyIndices.compute)) {
      // 転送ファミリーのインデックスが異なる場合は、転送キューの追加のキュー生成情報が必要です。
      VkDeviceQueueCreateInfo queueCreateInfo =
          Initializer::DeviceQueueCreateInfo(queueFamilyIndices.transfer, 1,
                                             &defaultQueuePriority);
      queueCreateInfos.emplace_back(queueCreateInfo);
    }
  } else {
    // そうでなかった場合は同じグラフィックスキューと同じキューを使用します。
    queueFamilyIndices.transfer = queueFamilyIndices.graphics;
  }

  // 論理デバイスを生成します。
  std::vector<const char *> deviceExtensions(std::move(requestedExtensions));
  if (useSwapchain) {
    deviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  }

  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.pEnabledFeatures = &requestedFeatures;

  // pNextChainが渡された場合、それをデバイス生成情報に追加する必要があります。
  VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
  if (pNextChain) {
    physicalDeviceFeatures2.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.features = requestedFeatures;
    physicalDeviceFeatures2.pNext = pNextChain;
    deviceCreateInfo.pEnabledFeatures = nullptr;
    deviceCreateInfo.pNext = &physicalDeviceFeatures2;
  }

  if (!deviceExtensions.empty()) {
    for (const auto &enableExtension : deviceExtensions) {
      if (!IsSupportedExtension(enableExtension)) {
        std::cerr << "Enabled device extension \"" << enableExtension
                  << "\" is not present at device level!" << std::endl;
      }
    }
    deviceCreateInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
  }

  VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr,
                                   &logicalDevice);
  if (result != VK_SUCCESS) {
    return result;
  }

  // グラフィックコマンドバッファのデフォルトのコマンドプールを生成します。
  commandPool = CreateCommandPool(queueFamilyIndices.graphics);

  return result;
}

/**
 * @brief デバイス上にバッファを生成します。
 * @param device Vulkanデバイス
 * @param bufferUsageFlags 使用フラグビットマスク(Vertex, Index, Uniformなど)
 * @param memoryPropertyFlags
 * メモリプロパティ(DeviceLocal, HostVisible, ÒCoherentなど)
 * @param size バイト単位のバッファサイズ
 * @param buffer バッファハンドルへのポインタ
 * @param memory メモリハンドルへのポインタ
 * @param data 生成後にバッファをコピーする必要があるデータへのポインタ
 * @return バッファハンドルとメモリが生成された場合、VK_SUCCESSを返します。
 */
VkResult Device::CreateBuffer(VkBufferUsageFlags bufferUsageFlags,
                              VkMemoryPropertyFlags memoryPropertyFlags,
                              VkDeviceSize size, VkBuffer *buffer,
                              VkDeviceMemory *memory, const void *data) const {
  // バッファハンドルを生成します。
  VkBufferCreateInfo bufferCreateInfo =
      Initializer::BufferCreateInfo(bufferUsageFlags, size);
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VK_CHECK_RESULT(
      vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));

  // バッファハンドルをバックアップするメモリを生成します。
  VkMemoryRequirements memoryRequirements{};
  vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memoryRequirements);
  VkMemoryAllocateInfo memoryAllocateInfo = Initializer::MemoryAllocateInfo();
  memoryAllocateInfo.allocationSize = memoryRequirements.size;

  // バッファのプロパティに適合するメモリタイプのインデックスを見つけます。
  memoryAllocateInfo.memoryTypeIndex =
      FindMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags);

  // バッファにVK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BITが設定されている場合は、メモリ割り当て中に適切なフラグも有効にする必要があります。
  VkMemoryAllocateFlagsInfoKHR allocateFlagsInfo{};
  if (bufferUsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
    allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    memoryAllocateInfo.pNext = &allocateFlagsInfo;
  }
  VK_CHECK_RESULT(
      vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, memory));

  // バッファデータへのポインタが渡された場合は、バッファをマップしてデータをコピーします。
  if (data != nullptr) {
    void *mapped;
    VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
    std::memcpy(mapped, data, size);

    // ホストの一貫性(Coherency)がリクエストされていない場合は、手動でフラッシュして書き込みを表示します。
    if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
      VkMappedMemoryRange mappedMemoryRange = Initializer::MappedMemoryRange();
      mappedMemoryRange.memory = *memory;
      mappedMemoryRange.offset = 0;
      mappedMemoryRange.size = size;
      VK_CHECK_RESULT(
          vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedMemoryRange));
    }
    vkUnmapMemory(logicalDevice, *memory);
  }
  // メモリをバッファオブジェクトにアタッチします。
  VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));
  return VK_SUCCESS;
}

/**
 * @brief アロケートコマンドバッファ用のコマンドプールを生成します。
 * @param queueFamilyIndex
 * コマンドプールを生成するためのキューのファミリーインデックス
 * @param createFlags(オプションです。) コマンド生成フラグ
 * @note
 * 生成されたプールからアロケートされたコマンドバッファは同じファミリーインデックスを持つキューにのみ送信できます。
 * @return 生成されたコマンドプールのハンドル
 */
VkCommandPool
Device::CreateCommandPool(uint32_t queueFamilyIndex,
                          VkCommandPoolCreateFlags createFlags) const {
  VkCommandPoolCreateInfo commandPoolCreateInfo =
      Initializer::CommandPoolCreateInfo();
  commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
  commandPoolCreateInfo.flags = createFlags;
  VkCommandPool pool;
  VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo,
                                      nullptr, &pool));
  return pool;
}

/**
 * @brief コマンドプールからコマンドバッファを割り当てます。
 * @param level 新しいコマンドバッファのレベル(プライマリまたはセカンダリ)
 * @param pool コマンドバッファが割り当てられるコマンドプール
 * @param begin trueの場合、新しいコマンドバッファへの記録が開始されます。
 * @return 割り当てられたコマンドバッファのハンドル
 */
VkCommandBuffer Device::CreateCommandBuffer(VkCommandPool pool,
                                            VkCommandBufferLevel level,

                                            bool begin) const {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo =
      Initializer::CommandBufferAllocateInfo(pool, level, 1);
  VkCommandBuffer commandBuffer;
  VK_CHECK_RESULT(vkAllocateCommandBuffers(
      logicalDevice, &commandBufferAllocateInfo, &commandBuffer));

  if (begin) {
    VkCommandBufferBeginInfo commandBufferBeginInfo =
        Initializer::CommandBufferBeginInfo();
    VK_CHECK_RESULT(
        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
  }
  return commandBuffer;
}

VkCommandBuffer Device::CreateCommandBuffer(VkCommandBufferLevel level,
                                            bool begin) const {
  return CreateCommandBuffer(commandPool, level, begin);
}

/**
 * @brief コマンドバッファの記録を終了し、キューに送信します。
 * @param commandBuffer フラッシュ対象のコマンドバッファ
 * @param queue コマンドバッファを送信するキュー
 * @param pool コマンドバッファが生成されたコマンドプール
 * @param free(オプションです。)
 * trueの場合、送信後にコマンドバッファを開放します。
 *
 * @note
 * コマンドバッファが送信されるキューは、アロケート元のプールと同じファミリーインデックスからのものである必要があります。
 * @note フェンスを使用して、コマンドバッファの実行が終了したことを確認します。
 */
void Device::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue,
                                VkCommandPool pool, bool free) const {
  if (commandBuffer == VK_NULL_HANDLE) {
    return;
  }

  VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo = Initializer::SubmitInfo();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // フェンスを生成して、コマンドバッファの実行が終了したことを確認します。
  VkFenceCreateInfo fenceCreateInfo = Initializer::FenceCreateInfo();
  VkFence fence;
  VK_CHECK_RESULT(
      vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence));

  // キューに送信します。
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
  // コマンドバッファの実行が終了したことをフェンスが通知するのを待ちます。
  VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE,
                                  std::numeric_limits<uint64_t>::max()));

  vkDestroyFence(logicalDevice, fence, nullptr);
  if (free) {
    vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
  }
}

void Device::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue,
                                bool free) const {
  return FlushCommandBuffer(commandBuffer, queue, commandPool, free);
}
