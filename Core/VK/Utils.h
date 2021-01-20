#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Utils {
float CalcDeviceScore(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions);
}
