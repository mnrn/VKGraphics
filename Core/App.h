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
#include <memory>

#include "VK/VkBase.h"
#include "Window.h"

// ********************************************************************************
// Class
// ********************************************************************************

class App : private boost::noncopyable {
public:
  explicit App(const nlohmann::json &config) {
    if (glfwInit() == GLFW_FALSE) {
      BOOST_ASSERT_MSG(false, "glfw Initialization failed!");
    }

    const auto appName = config["AppName"].get<std::string>();
    const auto width = config["Width"].get<int>();
    const auto height = config["Height"].get<int>();
    const auto samples = config["Samples"].get<int>();

    window_ = Window::Create(width, height, appName.c_str(), samples);
    config_ = config;
  }

  ~App() {
    Window::Destroy(window_);
    glfwTerminate();
  }

  int Run(std::unique_ptr<VkBase> app) {
    if (window_ == nullptr) {
      return EXIT_FAILURE;
    }

    glfwSetWindowUserPointer(window_, app.get());
    glfwSetFramebufferSizeCallback(window_, VkBase::OnResized);
    app->OnInit(config_, window_);

    while (!glfwWindowShouldClose(window_) &&
           !glfwGetKey(window_, GLFW_KEY_ESCAPE)) {
      glfwPollEvents();

      app->OnUpdate(static_cast<float>(glfwGetTime()));
      app->OnRender();
    }
    app->WaitIdle();

    app->OnDestroy();
    return EXIT_SUCCESS;
  }

protected:
  GLFWwindow *window_ = nullptr;
  nlohmann::json config_{};
};

#endif // APP_HPP
