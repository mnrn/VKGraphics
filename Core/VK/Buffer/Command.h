#pragma once

#include <vulkan/vulkan.h>

struct Instance;

namespace Command {
VkCommandBuffer Get(const Instance &instance);
void Flush(const Instance &instance, VkCommandBuffer command);
} // namespace Command
