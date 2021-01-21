#include "VK/Utils.h"

#include <map>
#include <spdlog/spdlog.h>

float CalcDeviceScore(VkPhysicalDevice device,
                      const std::vector<const char *> &deviceExtensions) {
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

  uint32_t size = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &size, nullptr);
  std::vector<VkExtensionProperties> extensions(size);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &size,
                                       extensions.data());

  std::set<std::string> missing(deviceExtensions.begin(),
                                deviceExtensions.end());
  for (const auto &extension : extensions) {
    missing.erase(extension.extensionName);
  }
  if (!missing.empty()) {
    score = 0;
#if !defined(NDEBUG)
    spdlog::warn("Missing device extensions");
#endif
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
