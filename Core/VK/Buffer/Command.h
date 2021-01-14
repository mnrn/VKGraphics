#pragma once

#include <vulkan/vulkan.h>

struct Instance;

namespace Command {
VkCommandBuffer BeginSingleTime(const Instance &instance);
void EndSingleTime(const Instance &instance, VkCommandBuffer command);
} // namespace Command
