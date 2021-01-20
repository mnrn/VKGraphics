/**
 * @brief Common Definitions.
 */

#pragma once

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#include <cassert>
#include <iostream>
#include <vulkan/vulkan.h>

inline std::string ErrorString(VkResult errorCode) {
  switch (errorCode) {
#define STR(r)                                                                 \
  case VK_##r:                                                                 \
    return #r
    STR(NOT_READY);
    STR(TIMEOUT);
    STR(EVENT_SET);
    STR(EVENT_RESET);
    STR(INCOMPLETE);
    STR(ERROR_OUT_OF_HOST_MEMORY);
    STR(ERROR_OUT_OF_DEVICE_MEMORY);
    STR(ERROR_INITIALIZATION_FAILED);
    STR(ERROR_DEVICE_LOST);
    STR(ERROR_MEMORY_MAP_FAILED);
    STR(ERROR_LAYER_NOT_PRESENT);
    STR(ERROR_EXTENSION_NOT_PRESENT);
    STR(ERROR_FEATURE_NOT_PRESENT);
    STR(ERROR_INCOMPATIBLE_DRIVER);
    STR(ERROR_TOO_MANY_OBJECTS);
    STR(ERROR_FORMAT_NOT_SUPPORTED);
    STR(ERROR_SURFACE_LOST_KHR);
    STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
    STR(SUBOPTIMAL_KHR);
    STR(ERROR_OUT_OF_DATE_KHR);
    STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    STR(ERROR_VALIDATION_FAILED_EXT);
    STR(ERROR_INVALID_SHADER_NV);
#undef STR
  default:
    return "UNKNOWN_ERROR";
  }
}

#define VK_CHECK_RESULT(f)                                                     \
  do {                                                                         \
    VkResult res = (f);                                                        \
    if (res != VK_SUCCESS) {                                                   \
      std::cerr << "Fatal : VkResult is \"" << ErrorString(res) << "\" in "    \
                << __FILE__ << " at line " << __LINE__ << "\n";                \
      assert(res == VK_SUCCESS);                                               \
    }                                                                          \
  } while (false)

#ifdef __clang__
#   pragma clang diagnostic pop
#endif
