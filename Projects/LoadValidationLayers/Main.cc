#include <iostream>
#include <vulkan/vulkan.h>

int main() {
  uint32_t instanceLayerCount;
  VkResult res =
      vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
  std::cout << instanceLayerCount << " layers found!" << std::endl;
  if (instanceLayerCount > 0) {
    std::unique_ptr<VkLayerProperties[]> instanceLayers =
        std::make_unique<VkLayerProperties[]>(instanceLayerCount);
    res = vkEnumerateInstanceLayerProperties(&instanceLayerCount,
                                             instanceLayers.get());
    for (uint32_t i = 0; i < instanceLayerCount; i++) {
      std::cout << instanceLayers[i].layerName << std::endl;
    }
  }

  const char *names[] = {"VK_LAYER_KHRONOS_validation"};
  VkInstanceCreateInfo info{};
  info.enabledLayerCount = 1;
  info.ppEnabledLayerNames = names;

  VkInstance inst;
  res = vkCreateInstance(&info, nullptr, &inst);
  std::cout << "vkCreateInstance result: " << res << std::endl;

  vkDestroyInstance(inst, nullptr);
  return 0;
}
