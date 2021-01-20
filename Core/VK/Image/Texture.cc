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
#include "VK/Image/ImageView.h"
#include "VK/Instance.h"

namespace Pixels {
static stbi_uc *Load(const std::string &path, int &w, int &h,
                     bool flip = true) {
  int bytesPerPix;
  stbi_set_flip_vertically_on_load(flip);
  return stbi_load(path.c_str(), &w, &h, &bytesPerPix, 4);
}

static void Free(unsigned char *data) { stbi_image_free(data); }
} // namespace Pixels

void Texture::Create(const Instance &instance, const std::string &filepath) {
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

  Image::Create(instance, w, h, 0, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

  Image::TransitionImageLayout(instance, image, VK_FORMAT_R8G8B8A8_UNORM,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  Buffer::CopyToImage(instance, staging, image, w, h);
  Image::TransitionImageLayout(instance, image, VK_FORMAT_R8G8B8A8_UNORM,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(instance.device, staging, nullptr);
  vkFreeMemory(instance.device, stagingMemory, nullptr);

  view = ImageView::Create(instance, image, VK_IMAGE_VIEW_TYPE_2D,
                           VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Texture::Destroy(const Instance &instance) const {
  if (sampler) {
    vkDestroySampler(instance.device, sampler, nullptr);
  }
  vkDestroyImageView(instance.device, view, nullptr);
  vkDestroyImage(instance.device, image, nullptr);
  vkFreeMemory(instance.device, memory, nullptr);
}
