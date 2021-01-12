/**
 * @brief アプリに関するクラスを扱います。
 */

#ifndef APP_H
#define APP_H

// ********************************************************************************
// Including files
// ********************************************************************************

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Common.h"
#include "VK/VkApp.h"
#include "Window.h"

// ********************************************************************************
// Class
// ********************************************************************************

class App : private boost::noncopyable {
public:
  explicit App(const nlohmann::json& config) {
    if (glfwInit() == GL_FALSE) {
      BOOST_ASSERT_MSG(false, "glfw Initialization failed!");
    }
    const auto appName = config["AppName"].get<std::string>();
    width_ = config["Width"].get<int>();
    height_ = config["Height"].get<int>();
    const auto samples = config["Samples"].get<int>();
    window_ = Window::Create(width_, height_, appName.c_str(), samples);
    glfwGetFramebufferSize(window_, &width_, &height_);

    vkImpl_.OnCreate(config, window_);
  }

  ~App() {
    vkImpl_.OnDestroy();

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

      OnUpdate(static_cast<float>(glfwGetTime()));
      OnRender();

      glfwSwapBuffers(window_);
      glfwPollEvents();
    }

    OnDestroy();
    return EXIT_SUCCESS;
  }

protected:
  GLFWwindow *window_ = nullptr;
  int width_;
  int height_;

  VkApp vkImpl_;
};

#endif // APP_HPP
