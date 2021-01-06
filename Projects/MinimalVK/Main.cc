#include <vulkan/vulkan.h>

#include <iostream>

int main() {
  VkInstance inst;
  VkInstanceCreateInfo info{};

  VkResult res = vkCreateInstance(&info, nullptr, &inst);
  std::cout << "vkCreateInstance result: " << res << std::endl;

  vkDestroyInstance(inst, nullptr);
  return 0;
}
