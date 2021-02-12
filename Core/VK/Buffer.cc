/**
 * @brief デバイスメモリによってバックアップされたVulkan
 * Bufferへのアクセスをカプセル化します。
 */

#include "VK/Buffer.h"

#include <boost/assert.hpp>

#include "VK/Device.h"
#include "VK/Initializer.h"

/**
 * @brief
 * このバッファのメモリ範囲をマップします。成功した場合、マップされたポイントは指定されたバッファ範囲を指します。
 * @param size (オプションです。)
 * マップするメモリ範囲のサイズ。VK_WHOLE_SIZEを渡すと、完全なバッファ範囲をマップします。
 * @param offset (オプションです。) 先頭からのバイトオフセット
 */
VkResult Buffer::Map(const Device &device, VkDeviceSize size,
                     VkDeviceSize offset) {
  return vkMapMemory(device, memory, offset, size, 0, &mapped);
}

/**
 * @brief マップされたメモリ範囲のマップを解除します。
 */
void Buffer::Unmap(const Device &device) {
  if (mapped != nullptr) {
    vkUnmapMemory(device, memory);
    mapped = nullptr;
  }
}

/**
 * @brief 割り当てられたメモリブロックをバッファにアタッチします。
 * @param offset (オプションです。) バインドするメモリのバイトオフセット
 * @return vkBindBufferMemory呼び出しのVkResult
 */
VkResult Buffer::Bind(const Device &device, VkDeviceSize offset) const {
  return vkBindBufferMemory(device, buffer, memory, offset);
}

/**
 * @brief このバッファのデフォルトの記述子を設定します。
 * @param size (オプションです。) 記述子のメモリ範囲のサイズ
 * @param offset (オプションです。) 先頭からのバイトオフセット
 */
void Buffer::SetupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
  descriptor.offset = offset;
  descriptor.buffer = buffer;
  descriptor.range = size;
}

/**
 * @brief 指定されたデータをバッファにコピーします。
 * @param data コピーするデータへのポインタ
 * @param size コピーするデータのサイズ
 */
void Buffer::Copy(void *data, VkDeviceSize size) const {
  BOOST_ASSERT(mapped != nullptr);
  std::memcpy(mapped, data, size);
}

/**
 * @brief バッファのメモリ範囲をフラッシュして、デバイスから見えるようにします。
 * @param size (オプションです。)
 * フラッシュするメモリ範囲のサイズ。VK_WHOLE_SIZEを渡すと、バッファ範囲全体をフラッシュします。
 * @param offset (オプションです。) 先頭からのバイトオフセット
 * @return vkFlushMappedMemoryRangesのVkResult
 */
VkResult Buffer::Flush(const Device &device, VkDeviceSize size,
                       VkDeviceSize offset) const {
  VkMappedMemoryRange mappedMemoryRange = Initializer::MappedMemoryRange();
  mappedMemoryRange.memory = memory;
  mappedMemoryRange.offset = offset;
  mappedMemoryRange.size = size;
  return vkFlushMappedMemoryRanges(device, 1, &mappedMemoryRange);
}

/**
 * @brief バッファのメモリ範囲を無効にして、ホストから見えるようします。
 * @param size (オプションです。)
 * 無効にするメモリ範囲のサイズ。VK_WHOLE_SIZEを渡すと、バッファ範囲全体を無効にします。
 * @param offset (オプションです。) 先頭からのバイトオフセット
 * @return vkInvalidateMappedMemoryRangesのVkResult
 */
VkResult Buffer::Invalidate(const Device &device, const VkDeviceSize size,
                            VkDeviceSize offset) const {
  VkMappedMemoryRange mappedMemoryRange = Initializer::MappedMemoryRange();
  mappedMemoryRange.memory = memory;
  mappedMemoryRange.offset = offset;
  mappedMemoryRange.size = size;
  return vkInvalidateMappedMemoryRanges(device, 1, &mappedMemoryRange);
}

/**
 * @brief デバイス上にバッファを生成します。
 * @param device Vulkanデバイス
 * @param bufferUsageFlags 使用フラグビットマスク(Vertex, Index, Uniformなど)
 * @param memoryPropertyFlags
 * メモリプロパティ(DeviceLocal,HostVisible,Coherentなど)
 * @param size バイト単位のバッファサイズ
 * @param data 生成後にバッファをコピーする必要があるデータへのポインタ
 * @return バッファハンドルとメモリが生成された場合、VK_SUCCESSを返します。
 */
VkResult Buffer::Create(const Device &device,
                        VkBufferUsageFlags bufferUsageFlags,
                        VkMemoryPropertyFlags memoryPropertyFlags,
                        VkDeviceSize size, void *data) {
  VkResult result = device.CreateBuffer(bufferUsageFlags, memoryPropertyFlags,
                                        data, size, buffer, memory);
  SetupDescriptor(size);
  return result;
}

/**
 * @brief デバイス上にバッファを生成します。
 * @param device Vulkanデバイス
 * @param bufferUsageFlags 使用フラグビットマスク(Vertex, Index, Uniformなど)
 * @param memoryPropertyFlags
 * メモリプロパティ(DeviceLocal,HostVisible,Coherentなど)
 * @param size バイト単位のバッファサイズ
 * @return バッファハンドルとメモリが生成された場合、VK_SUCCESSを返します。
 */
VkResult Buffer::Create(const Device &device,
                        VkBufferUsageFlags bufferUsageFlags,
                        VkMemoryPropertyFlags memoryPropertyFlags,
                        VkDeviceSize size) {
  return Create(device, bufferUsageFlags, memoryPropertyFlags, size, nullptr);
}

/**
 * @brief バッファが持っているリソースを解放します。
 */
void Buffer::Destroy(const Device &device) const {
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(device, memory, nullptr);
  }
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, buffer, nullptr);
  }
}
