/**
 * @brief アプリに関するクラスを扱います。
 */

#ifndef APP_H
#define APP_H

// ********************************************************************************
// Including files
// ********************************************************************************

#include <GLFW/glfw3.h>

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <iostream>

#include "Common.h"

#include "HID/KeyInput.h"
#include "Window.h"

// ********************************************************************************
// Class
// ********************************************************************************

class App : private boost::noncopyable {
public:
  App(const char *appName, int w = 1280, int h = 720, int samples = 0) {
    if (glfwInit() == GL_FALSE) {
      BOOST_ASSERT_MSG(false, "glfw Initialization failed!");
      return;
    }

    window_ = Window::Create(w, h, appName, samples);
    glfwGetFramebufferSize(window_, &width_, &height_);
  }

  ~App() {
    Window::Destroy(window_);
    glfwTerminate();
  }

  template <typename Initialize, typename Update, typename Render,
            typename Destroy>
  int Run(Initialize OnInit, Update OnUpdate, Render OnRender,
          Destroy OnDestroy) {
    if (window_ == nullptr) {
      return EXIT_FAILURE;
    }

    OnInit(width_, height_);

    while (!glfwWindowShouldClose(window_) &&
           !glfwGetKey(window_, GLFW_KEY_ESCAPE)) {

      OnPreUpdate(window_);
      OnUpdate(static_cast<float>(glfwGetTime()));
      OnRender();

      glfwSwapBuffers(window_);
      glfwPollEvents();
    }

    OnDestroy();
    return EXIT_SUCCESS;
  }

private:
  GLFWwindow *window_ = nullptr;
  int width_;
  int height_;

  static void OnPreUpdate(GLFWwindow *hwd) {
    if (hwd == nullptr) {
      return;
    }

    // キー入力更新を行いますが、キー入力初期化は各々に任せます。
    if (KeyInput::IsExist()) {
      KeyInput::Get().OnUpdate(hwd);
    }
  }
};

#endif // APP_HPP
