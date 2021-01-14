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
  explicit App(const nlohmann::json &config) {
    if (glfwInit() == GL_FALSE) {
      BOOST_ASSERT_MSG(false, "glfw Initialization failed!");
    }

    const auto appName = config["AppName"].get<std::string>();
    width_ = config["Width"].get<int>();
    height_ = config["Height"].get<int>();
    const auto samples = config["Samples"].get<int>();

    window_ = Window::Create(width_, height_, appName.c_str(), samples);

    glfwSetWindowUserPointer(window_, &vkImpl_);
    glfwSetFramebufferSizeCallback(window_, VkApp::OnResized);

    vkImpl_.OnCreate(config, window_);
  }

  ~App() {
    vkImpl_.OnDestroy();

    Window::Destroy(window_);
    glfwTerminate();
  }

  template <typename Initialize, typename Update, typename Destroy>
  int Run(Initialize OnInit, Update OnUpdate, Destroy OnDestroy) {
    if (window_ == nullptr) {
      return EXIT_FAILURE;
    }

    OnInit();

    while (!glfwWindowShouldClose(window_) &&
           !glfwGetKey(window_, GLFW_KEY_ESCAPE)) {
      glfwPollEvents();

      OnUpdate(static_cast<float>(glfwGetTime()));
      vkImpl_.OnRender();
    }
    vkImpl_.WaitIdle();

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
