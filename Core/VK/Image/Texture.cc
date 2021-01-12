/**
 * @brief Texture
 */

#include "Texture.h"

#include <boost/assert.hpp>
#include <cstdlib>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "VK/Buffer/Buffer.h"
#include "VK/Image/Image.h"
#include "VK/Instance.h"
#include "VK/Pipeline/Pipelines.h"

namespace Pixels {
static stbi_uc *Load(const std::string &path, int &w, int &h,
                     bool flip = true) {
  int bytesPerPix;
  stbi_set_flip_vertically_on_load(flip);
  return stbi_load(path.c_str(), &w, &h, &bytesPerPix, 4);
}

static void Free(unsigned char *data) { stbi_image_free(data); }
} // namespace Pixels

void Texture::Load(const Instance &instance, const std::string &filepath) {
  int w, h;
  stbi_uc *pixels = Pixels::Load(filepath, w, h);
  if (pixels == nullptr) {
    std::cerr << "Failed to load " << filepath << std::endl;
    BOOST_ASSERT_MSG(false, "Failed to load texture!");
  }

  VkDeviceSize size = static_cast<size_t>(w * h * 4);
  VkBuffer staging;
  VkDeviceMemory stagingMemory;
  Buffer::Create(instance, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMemory);

  void *data = nullptr;
  vkMapMemory(instance.device, stagingMemory, 0, size, 0, &data);
  std::memcpy(data, pixels, size);
  vkUnmapMemory(instance.device, stagingMemory);

  Pixels::Free(pixels);
}

void Texture::CreateDescriptorSet(const Instance &, const Pipelines &) {
}

void Texture::Cleanup(const Instance& instance) const {
  vkDestroyImageView(instance.device, view, nullptr);
  vkDestroyImage(instance.device, image, nullptr);
  vkFreeMemory(instance.device, memory, nullptr);
}
