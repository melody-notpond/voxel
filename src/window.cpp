#include "window.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>

using namespace vx;

void Window::fb_resize_cb(GLFWwindow *window, int width, int height) {
  Window::get(window).fb_resized = true;
}

Window::Window(int width, int height, const char *title) {
  assert(width > 0 && height > 0);

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, fb_resize_cb);

  last_time = glfwGetTime();
  last_second = last_time;
  fps_ = 0.;
}

float Window::delta_time() {
  float current_time = glfwGetTime();
  float dt = current_time - last_time;
  last_time = current_time;
  if (current_time - last_second >= 1.) {
    fps_ = frame_count;
    frame_count = 0;
    last_second = current_time;

    // HACK: this should be moved into rendering code
    std::cout << "fps: " << fps_ << std::endl;
  } else frame_count += 1;
  return dt;
}

Window::~Window() {
  glfwDestroyWindow(window_);
  glfwTerminate();
}
