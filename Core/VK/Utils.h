#pragma once

#include <vulkan/vulkan.h>

#include <set>
#include <vector>

float CalcDeviceScore(VkPhysicalDevice physicalDevice,
                      const std::vector<const char *> &deviceExtensions);
