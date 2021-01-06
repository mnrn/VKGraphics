/**
 * @brief  Framework Window
 * @date   2017/03/19
 */

#ifndef WINDOW_H
#define WINDOW_H

// ********************************************************************************
// Including files
// ********************************************************************************

#include <GLFW/glfw3.h>
#include <boost/assert.hpp>
#include <thread>

// ********************************************************************************
// Namespace
// ********************************************************************************

namespace Window {

static inline GLFWwindow *Create(int w, int h, const char *title, int samples,
                                 GLFWmonitor *monitor = nullptr,
                                 GLFWwindow *share = nullptr) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  if (samples > 0) {
    glfwWindowHint(GLFW_SAMPLES, samples);
  }

  // create window handle
  GLFWwindow *handle = glfwCreateWindow(w, h, title, monitor, share);
  if (handle == nullptr) {
    glfwTerminate();
    BOOST_ASSERT_MSG(false, "Failed to create window!");
    return nullptr;
  }

  glfwMakeContextCurrent(handle);
  return handle;
}

static inline GLFWwindow *Destroy(GLFWwindow *handle) {
  if (handle != nullptr) {
    glfwDestroyWindow(handle);
  }
  return nullptr;
}

} // namespace Window

#endif // WINDOW_H
